//
// Created by kira on 25-4-6.
//

#ifndef APPENTRY_H
#define APPENTRY_H

#include <iostream>
#include <thread>
#include <signal.h>
#include <opencv2/opencv.hpp>
#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>

#include "MainWindow.h"

using namespace std;


void signal_handler(int signum);

class AppEntry : public QObject
{
    Q_OBJECT

public:
    enum StartupMode
    {
        WindowOnly,
        EinkOnly,
        WindowAndEink,
    };

    AppEntry();

    int exec(int argc,char* argv[]);

    void destroy();

    void manualClick();


private:
    bool _isEinkInit = false;
    bool cancellationToken = false;
    const int frameRate = 1000/10;
    StartupMode _mode;

    QSize _screenSize;

    cv::Mat cvFrame;

    shared_ptr<QApplication> app;
    shared_ptr<MainWindow> window;
    shared_ptr<QImage> frame;
    shared_ptr<std::thread> qtRenderThread;
    shared_ptr<QTimer> frameTimer;

    shared_ptr<QTimer> clickTimer;

    void QtRender();
signals:
    void CallRendering(QImage& image);
    void clickAt(int x, int y);
};

#endif //APPENTRY_H
