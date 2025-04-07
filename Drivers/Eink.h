//
// Created by kira on 25-4-6.
//

#ifndef EINK_H
#define EINK_H

#include <bitset>
#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <opencv2/opencv.hpp>
#include <signal.h>
#include <thread>
#include <vector>

#include "DEV_Config.h"
#include "EPD_2in13_V4.h"
#include "GT1151.h"

using namespace std;

class Eink
{
public:
    /**
     * Display update mode.
     */
    enum Update
    {
        UpdateFull = EPD_2IN13_V4_FULL,
        UpdateFast = EPD_2IN13_V4_Fast,
        UpdatePart = EPD_2IN13_V4_PART,
    };

    Eink();

    void destroy();

    struct EinkTouchPoint
    {
        u_int16_t xDirect;
        u_int16_t yDirect;
        u_int16_t size;
    };

    class Touch
    {
    public:
        Touch();

        void destroy();

        void registerCallback(const function<void(vector<EinkTouchPoint>)>& callback);

    private:
        GT1151_Dev *Dev_Now, *Dev_Old;
        bool cancellationToken = false;
        shared_ptr<thread> scanThread;
        function<void(vector<EinkTouchPoint>)> _callback;

        void scanLoop() const;
    };

    class Display
    {
    public:
        Display();

        void destroy();

        /**
         * Update Image
         * Size: 250x122
         */
        void updateImage(const cv::Mat& sourceImage, Update mode);

        void updateImageAuto(const cv::Mat& sourceImage);

    private:
        const int sleepInterval = 15;
        const int screenWidth = 250;
        const int screenHigh = 122;

        bool cancellationToken = false;
        bool epdSleep = true;
        Update currentMode = UpdatePart;
        int64_t lastActivate;
        cv::Mat lastImage;
        int fullUpdateDiffRate = 30;
        shared_ptr<std::thread> sleepMonitorThread;

        void updateActivate();

        void sleepMonitor();

        /**
         * Diff rate of two images
         */
        int imageDiffs(const cv::Mat& thisImage) const;

        u_int8_t* convertImage(cv::Mat sourceImage) const;
    };

    shared_ptr<Touch> getTouch();
    shared_ptr<Display> getDisplay();

private:
    shared_ptr<Touch> _touch;
    shared_ptr<Display> _display;
};


#endif //EINK_H
