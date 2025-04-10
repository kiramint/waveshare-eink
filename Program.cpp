#include <bitset>
#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <csignal>
#include <thread>
#include <vector>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <opencv2/opencv.hpp>

#include <fcntl.h>
#include <sys/wait.h>
#include <semaphore.h>

#include "driver/Eink.h"
#include "driver/X11Redirect.h"

using namespace std;

Eink* eInkInstance;
X11Redirect* redirect;
pid_t xvfbPid, appPid, parentPid;
int signalCalled = 0;

int updateRateMs = (1 / 10); // FPS
long lastPressed = 0;
auto startTime = std::chrono::system_clock::now();

atomic<bool> cancellationToken(false);
atomic<int> xTouch(0);
atomic<int> yTouch(0);
atomic<bool> isTouch(false);


sem_t* xvfbRun;
sem_t* mainLoopRun;

// Quit handler
void signal_handler(int signum)
{
    if (signalCalled == 0)
    {
        signalCalled = true;
        std::cout << endl
            << "Signal: SIGINT received, terminating..." << std::endl;

        cout << "Stopping event loop" << endl;
        cancellationToken = true;

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        try
        {
            cout << "Killing Eink driver" << endl;;
            if (eInkInstance != nullptr)
            {
                eInkInstance->destroy();
                delete eInkInstance;
            }
        }
        catch (exception& e)
        {
            cerr << "Exception caught:" << e.what() << endl;
        }

        cout << "Killing xvfb" << endl;
        kill(xvfbPid, SIGINT);
        cout << "Killing x11app" << endl;
        kill(appPid, SIGINT);

        int status;
        pid_t wpid;

        cout << "Waiting for xvfb and x11app die..." << endl;
        while ((wpid = wait(&status)) > 0)
        {
            std::cout << "Parent: child " << wpid << " exited with status " << status << "\n";
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));

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

// Touch Callback
void scrCallBack(vector<Eink::EinkTouchPoint> touchData)
{
    long thisPress = std::chrono::duration_cast<std::chrono::milliseconds>
        (chrono::system_clock::now() - startTime).count();

    // Anti vibration
    if (thisPress - lastPressed > 200)
    {
        auto touch = touchData[0];
        xTouch.store(touch.xDirect);
        yTouch.store(touch.yDirect);
        isTouch.store(touch.pressed);

        lastPressed = std::chrono::duration_cast<std::chrono::milliseconds>
            (chrono::system_clock::now() - startTime).count();
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <x11-Apps> [args...]\n";
        return 1;
    }

    cout << "App params: " << argv[1] << " " << &argv[1] << endl;

    system("killall Xvfb");
    system((std::string("killall ") + std::string(argv[1])).c_str());

    sem_unlink("/eink_xvfb_run");
    sem_unlink("/eink_loop_run");

    xvfbRun = sem_open("/eink_xvfb_run",O_CREAT | O_EXCL, 0644, 0);
    mainLoopRun = sem_open("/eink_loop_run",O_CREAT | O_EXCL, 0644, 0);

    if (xvfbRun == nullptr || mainLoopRun == nullptr)
    {
        std::cerr << "sem init error" << endl;
        std::cerr << "Error: " << strerror(errno) << std::endl;
        return -1;
    }


    // Handel quit
    signal(SIGINT, signal_handler);

    parentPid = getpid();

    setenv("DISPLAY", ":99", true);

    pid_t pid = fork();
    if (pid == 0) // Handle Xvfp
    {
        const char* xvfbArgs[] = {"Xvfb", ":99", "-screen", "0", "250x122x24", nullptr};
        sem_post(xvfbRun);
        cout << "Xvfb start" << endl;
        execvp("Xvfb", const_cast<char* const*>(xvfbArgs));
        kill(parentPid, SIGINT);
        cout << "Xvfb killed" << endl;
        exit(0);
    }
    else if (pid > 0) // Handle Main
    {
        sem_wait(xvfbRun);

        appPid = fork();
        if (appPid == 0)
        {
            sem_wait(mainLoopRun);
            cout << "Running app: " << argv[1] << " " << &argv[1] << endl;
            execvp(argv[1], &argv[1]);
            kill(parentPid, SIGINT);
            exit(0);
        }

        cout << "App run at pid: " << appPid << endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));

        cout << "X11Redirect initializing..." << endl;
        redirect = new X11Redirect(":0");

        cout << "Eink driver initializing..." << endl;
        eInkInstance = new Eink();

        auto ink = eInkInstance->getDisplay();
        auto touch = eInkInstance->getTouch();

        touch->registerCallback(scrCallBack);

        sem_post(mainLoopRun);

        cout << "Entering event loop" << endl;

        while (!cancellationToken)
        {
            try
            {
                int status;
                pid_t result;

                result = waitpid(appPid, &status, WNOHANG);
                if (result != 0)
                {
                    if (WIFEXITED(status))
                    {
                        printf("Child exited with code %d\n", WEXITSTATUS(status));
                        signal_handler(SIGINT);
                        break;
                    }
                }

                auto frame = redirect->xCapture();
                cv::imwrite("Debug.jpg", frame);
                ink->updateImageAuto(frame);
                if (isTouch)
                {
                    redirect->xClick(xTouch.load(), yTouch.load());
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(updateRateMs));
            }
            catch (exception& e)
            {
                cout << "Exception: " << e.what() << "\n";
                signal_handler(SIGINT);
            }
        }

        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }
    }
    return 0;
};
