//
// Created by kira on 25-4-8.
//

#include "X11Redirect.h"

#include <X11/extensions/XTest.h>

X11RedirectException::X11RedirectException(std::string message): message(std::move(message))
{
}

const char* X11RedirectException::what() const noexcept
{
    return message.c_str();
}

X11Redirect::X11Redirect(std::string DISPLAY): DISPLAY(std::move(DISPLAY))
{
    display = XOpenDisplay(DISPLAY.c_str());
    if (display == nullptr) {
        std::cerr << "Cannot connect to X server: " << DISPLAY << std::endl;
        throw X11RedirectException("Cannot connect to X server: " + DISPLAY);
    }
    screen = DefaultScreenOfDisplay(display);
    _screenWidth = DisplayWidth(display, DefaultScreen(display));
    _screenHeight = DisplayHeight(display, DefaultScreen(display));
}

X11Redirect::~X11Redirect(){}

void X11Redirect::destroy()
{
    if (display != nullptr) {
        XCloseDisplay(display);
        delete screen;
    }
}

void X11Redirect::xClick(int x, int y)
{
    if (display != nullptr) {
        XTestFakeMotionEvent(display, -1, x, y, CurrentTime);
        XTestFakeButtonEvent(display, 1, True, CurrentTime);
        XTestFakeButtonEvent(display, 1, False, CurrentTime);
        XFlush(display);
    } else {
        std::cerr << "Cannot connect to X server: " << DISPLAY << std::endl;
        throw X11RedirectException("Cannot connect to X server: " + DISPLAY);
    }
}

cv::Mat X11Redirect::xCapture()
{
    image = XGetImage(display, RootWindow(display, DefaultScreen(display)),
                      0, 0, _screenWidth, _screenHeight, AllPlanes, ZPixmap);
    if (image == nullptr) {
        std::cerr << "Failed to capture screen image" << std::endl;
        throw X11RedirectException("Failed to capture screen image");
    }
    cv::Mat frame = XImageToMat(image).clone();
    delete image;
    return frame;
}

cv::Mat X11Redirect::XImageToMat(XImage* image)
{
    int width = image->width;
    int height = image->height;
    int bpp = image->bits_per_pixel;

    if (bpp != 32) {
        std::cerr << "Unsupported bits_per_pixel: " << bpp << std::endl;
        throw X11RedirectException("Unsupported bits_per_pixel: " + bpp);
    }
    cv::Mat mat(height, width, CV_8UC4);
    for (int y = 0; y < height; ++y) {
        memcpy(mat.ptr(y), image->data + y * image->bytes_per_line, width * 4);
    }
    cv::Mat matBGR;
    cv::cvtColor(mat, matBGR, cv::COLOR_BGRA2BGR);
    return matBGR;
}
