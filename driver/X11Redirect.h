//
// Created by kira on 25-4-8.
//

#ifndef X11REDIRECT_H
#define X11REDIRECT_H

#include <string>

#include <opencv2/opencv.hpp>

#include <X11/Xlib.h>

class X11RedirectException : std::exception {
public:
    explicit X11RedirectException(std::string message);

    [[nodiscard]] const char *what() const noexcept override;

private:
    std::string message;
};

class X11Redirect {
public:
    X11Redirect(std::string DISPLAY);

    ~X11Redirect();

    void destroy();

    void xClick(int x, int y);

    cv::Mat xCapture();

private:
    int _screenWidth{};
    int _screenHeight{};
    std::string DISPLAY;

    Display *display = nullptr;
    Screen *screen = nullptr;
    XImage *image = nullptr;

    static cv::Mat XImageToMat(XImage *image);
};



#endif //X11REDIRECT_H
