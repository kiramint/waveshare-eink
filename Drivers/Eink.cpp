//
// Created by kira on 25-4-6.
//

#include "Eink.h"

using namespace std;

/**
 * Init touch and display.
 * Use `getTouch()` and `getDisplay()` to get touch and display object
 */
Eink::Eink()
{
    DEV_SetIicAddress(0X14);
    DEV_ModuleInit();
    _display = make_shared<Display>();
    _touch = make_shared<Touch>();
}

/**
 * A MUST-CALL function before exit the program
 */
void Eink::destroy()
{
    _touch->destroy();
    _display->destroy();
    _touch.reset();
    _display.reset();
    DEV_ModuleExit();
}

Eink::Touch::Touch()
{
    GT_Init();
    Dev_Now = GT_GetDev_Now();
    Dev_Old = GT_GetDev_Old();
    scanThread = make_shared<std::thread>(&Touch::scanLoop, this);
}

void Eink::Touch::destroy()
{
    cancellationToken = true;
    GT_Reset();
    scanThread->join();
    cout << "Touch off" << endl;
}

void Eink::Touch::registerCallback(const function<void(vector<EinkTouchPoint>)>& callback)
{
    _callback = callback;
}

void Eink::Touch::scanLoop() const
{
    while (!cancellationToken)
    {
        try
        {
            this_thread::sleep_for(chrono::milliseconds(1));
            // Hardware touch signals
            if (DEV_Digital_Read(INT) == 0)
            {
                // Waveshare's fault
                Dev_Now->Touch = 1;

                // Software request
                if (GT_Scan() == 1 || (Dev_Now->X[0] == Dev_Old->X[0] &&
                    Dev_Now->Y[0] == Dev_Old->Y[0]))
                {
                    // No new touch
                    continue;
                }

                // Handle result
                if (Dev_Now->TouchpointFlag)
                {
                    Dev_Now->TouchpointFlag = 0;

                    // Get touch point
                    vector<EinkTouchPoint> touchData;
                    for (int i = 0; i < Dev_Now->TouchCount; i++)
                    {
                        // XY have adjust to QT Opencv mode
                        EinkTouchPoint touchPoint{
                            .xDirect = static_cast<u_int16_t>(255 - Dev_Now->Y[i]),
                            .yDirect = Dev_Now->X[i],
                            .size = Dev_Now->S[i]
                        };
                        touchData.emplace_back(touchPoint);
                    }

                    // TODO: Callback
                    if (_callback)
                    {
                        _callback(touchData);
                    }
                }
            }
            else
            {
                Dev_Now->Touch = 0;
            }
        }
        catch (exception& e)
        {
            cerr << "Exception: " << e.what() << endl;
        }
    }
}

Eink::Display::Display()
{
    EPD_2in13_V4_Init(EPD_2IN13_V4_FULL);
    EPD_2in13_V4_Clear();
    this_thread::sleep_for(chrono::seconds(1));
    EPD_2in13_V4_Sleep();
    sleepMonitorThread = make_shared<std::thread>(&Display::sleepMonitor, this);
}

void Eink::Display::destroy()
{
    cancellationToken = true;
    sleepMonitorThread->join();
    EPD_2in13_V4_Sleep();
    cout << "Display off" << endl;
}

void Eink::Display::updateImage(const cv::Mat& sourceImage, Update mode)
{
    // Update last image
    lastImage = sourceImage;

    // Convert image
    auto result = convertImage(sourceImage);

    // Check status
    if (epdSleep)
    {
        EPD_2in13_V4_Init(mode);
        updateActivate();
    }
    else if (currentMode != mode)
    {
        EPD_2in13_V4_Sleep();
        EPD_2in13_V4_Init(mode);
        currentMode = mode;
        updateActivate();
    }

    // Update image
    switch (mode)
    {
    case UpdateFull:
        EPD_2in13_V4_Display(result);
        break;
    case UpdateFast:
        EPD_2in13_V4_Display_Fast(result);
        break;
    case UpdatePart:
        EPD_2in13_V4_Display_Partial_Wait(result);
        break;
    default:
        break;
    }

    updateActivate();
}

void Eink::Display::updateImageAuto(const cv::Mat& sourceImage)
{
    if (lastImage.empty())
    {
        updateImage(sourceImage, UpdatePart);
        return;
    }

    auto diff_rate = imageDiffs(sourceImage);

    if (diff_rate > fullUpdateDiffRate)
    {
        updateImage(sourceImage, UpdateFast);
    }
    else if (diff_rate != 0)
    {
        updateImage(sourceImage, UpdateFast);
    }
}

void Eink::Display::updateActivate()
{
    using namespace chrono;
    lastActivate =
        duration_cast<seconds>(system_clock::now().time_since_epoch())
        .count();
    epdSleep = false;
}

void Eink::Display::sleepMonitor()
{
    while (!cancellationToken)
    {
        try
        {
            using namespace chrono;
            auto timeNow =
                duration_cast<seconds>(system_clock::now().time_since_epoch())
                .count();
            auto interval = timeNow - lastActivate;

            if (interval > sleepInterval && epdSleep == false)
            {
                EPD_2in13_V4_Sleep();
                epdSleep = true;
                cout << "Sending sleep to E-ink. (Time outdated)" << endl;
            }
            this_thread::sleep_for(seconds(1));
        }
        catch (exception& e)
        {
            cerr << "Exception: " << e.what() << endl;
        }
    }
}

int Eink::Display::imageDiffs(const cv::Mat& thisImage) const
{
    if (thisImage.empty() || lastImage.empty())
    {
        return 100;
    }
    if (thisImage.size() != lastImage.size() ||
        thisImage.type() != lastImage.type())
    {
        return 100;
    }

    cv::Mat diff;
    cv::absdiff(thisImage, lastImage, diff);

    auto nonZero = cv::countNonZero(diff.reshape(1));

    if (nonZero == 0)
    {
        return 0;
    }
    else
    {
        return static_cast<int>(nonZero / thisImage.total());
    }
}

u_int8_t* Eink::Display::convertImage(cv::Mat sourceImage) const
{
    // Resize image
    int imgWidth = sourceImage.cols;
    int imgHeight = sourceImage.rows;

    double scale = std::min(static_cast<double>(screenWidth) / imgWidth,
                            static_cast<double>(screenHigh) / imgHeight);

    cv::Mat resizedImage;
    cv::resize(sourceImage, resizedImage, cv::Size(), scale, scale,
               cv::INTER_LINEAR);

    cv::Mat fitImage =
        cv::Mat::zeros(screenHigh, screenWidth, sourceImage.type());
    fitImage.setTo(cv::Scalar(255, 255, 255));

    int offset_x = (screenWidth - resizedImage.cols) / 2;
    int offset_y = (screenHigh - resizedImage.rows) / 2;

    cv::Mat roi = fitImage(
        cv::Rect(offset_x, offset_y, resizedImage.cols, resizedImage.rows));
    resizedImage.copyTo(roi);

    cv::Mat rotatedImage;
    cv::rotate(fitImage, rotatedImage, cv::ROTATE_90_CLOCKWISE);

    // BGR2GRAT
    cv::Mat grayImage;
    cv::cvtColor(rotatedImage, grayImage, cv::COLOR_BGR2GRAY);
    cv::threshold(grayImage, grayImage, 128, 255, cv::THRESH_BINARY);

    // To EPD Image
    int rows = grayImage.rows;
    int cols = grayImage.cols;

    std::vector<uint8_t> outputArray;
    outputArray.clear();
    for (int i = 0; i < rows; ++i)
    {
        std::bitset<8> bitsetByte;
        for (int j = 0; j < cols; ++j)
        {
            uint8_t pixel = grayImage.at<uint8_t>(i, j);
            bitsetByte[7 - (j % 8)] = (pixel == 255);
            if (j % 8 == 7 || j == cols - 1)
            {
                outputArray.push_back(static_cast<uint8_t>(bitsetByte.to_ulong()));
                bitsetByte.reset();
            }
        }
    }

    u_int8_t* result = new u_int8_t[outputArray.size()];
    memcpy(result, outputArray.data(), sizeof(u_int8_t) * outputArray.size());

    return result;
}

shared_ptr<Eink::Touch> Eink::getTouch()
{
    return _touch;
}

shared_ptr<Eink::Display> Eink::getDisplay()
{
    return _display;
}
