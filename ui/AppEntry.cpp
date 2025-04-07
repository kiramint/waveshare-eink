//
// Created by kira on 25-4-6.
//

#include "AppEntry.h"

#include <QOpenGLContext>

AppEntry* _appInstance;

int signalCalled = 0;

AppEntry::AppEntry()
{
    _mode = WindowOnly;
    _screenSize = QSize(250, 122);
    _appInstance = this;
}

int AppEntry::exec(int argc, char* argv[])
{
    app = make_shared<QApplication>(argc, argv);
    window = make_shared<MainWindow>();

    QCommandLineParser parser;
    parser.setApplicationDescription("Kira E-Paper App 1.0");
    parser.addHelpOption();

    QCommandLineOption useEink(
        QStringList() << "e" << "useEink",
        "Use eink to display.");
    QCommandLineOption useEinkAndWindow(
        QStringList() << "w" << "useEinkAndWindow",
        "Use both eink and window to display");

    parser.addOption(useEink);
    parser.addOption(useEinkAndWindow);

    parser.process(*app);

    if (parser.isSet(useEink))
    {
        _mode == StartupMode::EinkOnly;
        signal(SIGINT, signal_handler);
        _isEinkInit = true;

        window->setVisible(true);
        // window->setAttribute(Qt::WA_DontShowOnScreen, true);

        // Qt render
        frame = make_shared<QImage>(_screenSize, QImage::Format_RGB888);
        frame->fill(Qt::white);

        frameTimer = make_shared<QTimer>(this);
        connect(&*frameTimer, &QTimer::timeout, this, &AppEntry::QtRender);
        connect(&*this,&AppEntry::CallRendering,&*window,&MainWindow::renderToImage);
        frameTimer->start(frameRate);

        clickTimer = make_shared<QTimer>(this);
        connect(&*clickTimer,&QTimer::timeout,this,&AppEntry::manualClick);
        connect(this,&AppEntry::clickAt,&*window,&MainWindow::clickAt);
        clickTimer-> start(5000);

    }
    else if (parser.isSet(useEinkAndWindow))
    {
        _mode == StartupMode::WindowAndEink;
        signal(SIGINT, signal_handler);
        _isEinkInit = true;
        window->show();
    }
    else
    {
        _mode == StartupMode::WindowOnly;
        window->show();
    }

    return QApplication::exec();
}

void AppEntry::manualClick()
{
    emit clickAt(250/2,122/2);
}

void AppEntry::destroy()
{
    cancellationToken = true;
    qtRenderThread->join();
}

void AppEntry::QtRender()
{
    try
    {
        emit CallRendering(*frame);
        cv::Mat convertMat(frame->height(),
                           frame->width(),
                           CV_8UC3,
                           frame->bits(),
                           frame->bytesPerLine()
        );
        cvFrame = convertMat.clone();
        imwrite("frameDebug.jpg", cvFrame);
    }
    catch (exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
    }
}


void signal_handler(int signum)
{
    if (signalCalled == 0)
    {
        signalCalled = true;
        std::cout << endl
            << "Signal: SIGINT received, terminating..." << std::endl;
        _appInstance->destroy();
        delete _appInstance;
        exit(0);
    }
    if (signalCalled > 5)
    {
        cerr << endl
            << "Terminated." << endl;
        exit(0);
    }
    signalCalled++;
    cout << endl
        << "Closing... be patient (•́ω•̀ o) (Ctrl+C 5 times to terminate)."
        << endl;
}
