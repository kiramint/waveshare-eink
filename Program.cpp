#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <bitset>
#include <vector>
#include <exception>
#include <signal.h>
#include <opencv2/opencv.hpp>

#include "DEV_Config.h"
#include "EPD_2in13_V4.h"
#include "GT1151.h"

using namespace std;

class Eink
{
public:
    enum Update
    {
        UpdateFull = EPD_2IN13_V4_FULL,
        UpdateFast = EPD_2IN13_V4_Fast,
        UpdatePart = EPD_2IN13_V4_PART,
    };

    Eink()
    {
        DEV_SetIicAddress(0X14);
        DEV_ModuleInit();
        _display = make_shared<Display>();
        _touch = make_shared<Touch>();
    }

    void destroy()
    {
        _touch->destroy();
        _display->destroy();
        _touch.reset();
        _display.reset();
        DEV_ModuleExit();
    }

    struct EinkTouchPoint
    {
        u_int16_t xDirect;
        u_int16_t yDirect;
        u_int16_t size;
    };

    class Touch
    {
    public:
        Touch()
        {
            GT_Init();
            Dev_Now = GT_GetDev_Now();
            Dev_Old = GT_GetDev_Old();
            scanThread = make_shared<std::thread>(&Touch::scanLoop, this);
        }

        void destroy()
        {
            bool cancellationToken = true;
            GT_Reset();
            scanThread->join();
            cout << "Touch off" << endl;
        }

        void registerCallback(function<void(vector<EinkTouchPoint>)> callback)
        {
            _callback = callback;
        }

    private:
        GT1151_Dev *Dev_Now, *Dev_Old;
        bool cancellationToken = false;
        shared_ptr<thread> scanThread;
        function<void(vector<EinkTouchPoint>)> _callback;

        void scanLoop()
        {
            while (!cancellationToken)
            {
                try
                {
                    this_thread::sleep_for(chrono::milliseconds(1));
                    // Hardward touch signals
                    if (DEV_Digital_Read(INT) == 0)
                    {
                        // Waveshare's fault
                        Dev_Now->Touch = 1;

                        // Software request
                        if (GT_Scan() == 1 || (Dev_Now->X[0] == Dev_Old->X[0] && Dev_Now->Y[0] == Dev_Old->Y[0]))
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
                                    .xDirect = (u_int16_t)255 - Dev_Now->Y[i],
                                    .yDirect = Dev_Now->X[i],
                                    .size = Dev_Now->S[i]};
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
                catch (exception &e)
                {
                    cerr << "Exception: " << e.what() << endl;
                }
            }
        }
    };

    class Display
    {
    public:
        Display()
        {
            EPD_2in13_V4_Init(EPD_2IN13_V4_FULL);
            EPD_2in13_V4_Clear();
            this_thread::sleep_for(chrono::seconds(1));
            EPD_2in13_V4_Sleep();
            sleepMonitorThread = make_shared<std::thread>(&Display::sleepMonitor, this);
        }

        void destroy()
        {
            cancellationToken = true;
            sleepMonitorThread->join();
            EPD_2in13_V4_Sleep();
            cout << "Display off" << endl;
        }

        /**
         * Update Image
         * Size: 250x122
         */
        void updateImage(cv::Mat sourceImage, Eink::Update mode)
        {
            // Update last image
            lastImage = sourceImage;

            // Convet image
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

        void updateImageAuto(cv::Mat sourceImage)
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

    private:
        const int sleepInterval = 15;
        const int screenWidth = 250;
        const int screenHight = 122;

        bool cancellationToken = false;
        bool epdSleep = true;
        Eink::Update currentMode = Eink::UpdatePart;
        int64_t lastActivate;
        cv::Mat lastImage;
        int fullUpdateDiffRate = 30;
        shared_ptr<std::thread> sleepMonitorThread;

        void updateActivate()
        {
            // cout << "Display init" << endl;
            using namespace chrono;
            lastActivate = duration_cast<seconds>(
                               system_clock::now().time_since_epoch())
                               .count();
            epdSleep = false;
        }

        void sleepMonitor()
        {

            while (!cancellationToken)
            {
                try
                {
                    using namespace chrono;
                    auto timeNow = duration_cast<seconds>(
                                       system_clock::now().time_since_epoch())
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
                catch (exception &e)
                {
                    cerr << "Exception: " << e.what() << endl;
                }
            }
        }

        /**
         * Diff rate of two images
         */
        int imageDiffs(cv::Mat thisImage)
        {
            if (thisImage.empty() || lastImage.empty())
            {
                return 100;
            }
            if (thisImage.size() != lastImage.size() || thisImage.type() != lastImage.type())
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

        u_int8_t *convertImage(cv::Mat sourceImage)
        {
            // Resize image
            int imgWidth = sourceImage.cols;
            int imgHeight = sourceImage.rows;

            double scale = std::min(
                static_cast<double>(screenWidth) / imgWidth,
                static_cast<double>(screenHight) / imgHeight);

            cv::Mat resizedImage;
            cv::resize(sourceImage, resizedImage, cv::Size(), scale, scale, cv::INTER_LINEAR);

            cv::Mat fitedImage = cv::Mat::zeros(screenHight, screenWidth, sourceImage.type());
            fitedImage.setTo(cv::Scalar(255, 255, 255));

            int offset_x = (screenWidth - resizedImage.cols) / 2;
            int offset_y = (screenHight - resizedImage.rows) / 2;

            cv::Mat roi = fitedImage(cv::Rect(offset_x, offset_y, resizedImage.cols, resizedImage.rows));
            resizedImage.copyTo(roi);

            cv::Mat rotatedImage;
            cv::rotate(fitedImage, rotatedImage, cv::ROTATE_90_CLOCKWISE);

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

            u_int8_t *result = new u_int8_t[outputArray.size()];
            memcpy(result, outputArray.data(), sizeof(u_int8_t) * outputArray.size());

            return result;
        }
    };

    shared_ptr<Touch> getTouch()
    {
        return _touch;
    }
    shared_ptr<Display> getDisplay()
    {
        return _display;
    }

private:
    shared_ptr<Touch> _touch;
    shared_ptr<Display> _display;
};

Eink *eInkInstance;

int signalCalled = 0;

void signal_handler(int signum)
{
    if (signalCalled == 0)
    {
        signalCalled = true;
        std::cout << endl
                  << "Signal: SIGINT received, terminating..." << std::endl;
        eInkInstance->destroy();
        delete eInkInstance;
        exit(0);
    }
    else if (signalCalled > 5)
    {
        cerr << endl
             << "Terminated." << endl;
        exit(0);
    }
    else
    {
        signalCalled++;
        cout << endl
             << "Closing... be patient (•́ω•̀ o) (Ctrl+C 5 times to terminate)." << endl;
    }
}

void scrCallBack(vector<Eink::EinkTouchPoint> touchData)
{
    cout << "Touch Points:" << endl;
    for (auto it : touchData)
    {
        cout << "\txDirect: " << it.xDirect << ", yDirect: " << it.yDirect << ", size: " << it.size << endl;
    }
}

int main()
{
    cout << "Compile: " << __DATE__ << " at " << __TIME__ << endl;
    cout << "GCC: " << __VERSION__ << endl;
    cout << "OpenCV: " << CV_VERSION << endl;
    cout << "\nKira Eink" << endl;

    signal(SIGINT, signal_handler);

    eInkInstance = new Eink();

    auto ink = eInkInstance->getDisplay();
    auto touch = eInkInstance->getTouch();

    touch->registerCallback(scrCallBack);

    cv::VideoCapture cap("/opt/project/app/Test_Neko.mp4");
    cv::Mat frame;

    while (1)
    {
        cap >> frame;
        if (frame.empty())
        {
            cap.set(cv::CAP_PROP_POS_FRAMES, 0);
            continue;
        }

        ink->updateImage(frame, Eink::UpdatePart);
    }

    return 0;
};