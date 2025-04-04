#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <bitset>
#include <vector>
#include <signal.h>
#include <opencv2/opencv.hpp>

#include "EPD_2in13_V4.h"

using namespace std;

class Eink
{
public:
    Eink()
    {
        sleepMonitorThread = make_shared<std::thread>(&Eink::sleepMonitor, this);
        DEV_ModuleInit();
        EPD_2in13_V4_Sleep();
    }

    ~Eink()
    {
        cancellationToken = true;
        sleepMonitorThread->join();
        EPD_2in13_V4_Sleep();
        DEV_ModuleExit();
    }

    /**
     * Update Image
     * Size: 250x122
     */
    void updateImage(cv::Mat sourceImage)
    {
        // Conver image

        auto result = convertImage(sourceImage);

        if (epdSleep)
        {
            EPD_2in13_V4_Init(EPD_2IN13_V4_FULL);
            updateActivate();
        }
        EPD_2in13_V4_Display(result);
    }

    void sleepMonitor()
    {
        while (!cancellationToken)
        {
            using namespace chrono;
            auto timeNow = duration_cast<seconds>(
                               system_clock::now().time_since_epoch())
                               .count();
            auto interval = timeNow - lastActivate;

            if (interval > 10 && epdSleep == false)
            {
                EPD_2in13_V4_Sleep();
                epdSleep = true;
                cout << "Sending sleep to E-ink. (Time outdated)" << endl;
            }
            if (interval > antiBurnInterval && interval % antiBurnInterval == 0)
            {
                EPD_2in13_V4_Sleep();
                epdSleep = true;
                cout << "Sending sleep to E-ink. (Anti burned)" << endl;
            }
            this_thread::sleep_for(seconds(1));
        }
    }

private:
    const int antiBurnInterval = 15;
    const int screenWidth = 250;
    const int screenHight = 122;

    bool cancellationToken = false;
    bool epdSleep = true;
    int64_t lastActivate;
    shared_ptr<std::thread> sleepMonitorThread;

    void updateActivate()
    {
        cout << "Eink init" << endl;
        using namespace chrono;
        lastActivate = duration_cast<seconds>(
                           system_clock::now().time_since_epoch())
                           .count();
        epdSleep = false;
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

Eink *eInkInstance;

void signal_handler(int signum)
{
    std::cout << "Signal " << signum << " received!" << std::endl;
    delete eInkInstance;
    exit(0);
}

int main()
{
    signal(SIGINT, signal_handler);
    auto ink = new Eink();
    eInkInstance = ink;
    cv::Mat Image = cv::imread("/opt/project/app/PAIMON.jpeg");
    ink->updateImage(Image);
    while (1)
    {
        this_thread::sleep_for(chrono::seconds(1));
    }
    return 0;
};