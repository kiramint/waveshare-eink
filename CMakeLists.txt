cmake_minimum_required(VERSION 3.21.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 17)

project(E-Paper VERSION 1.0)

# Waveshare
add_subdirectory(lib/waveshare_eink_2in13_v4_opi3b)

# OpenCV
find_package(OpenCV REQUIRED)
include_directories(
        ${OpenCV_INCLUDE_DIRS}
)

add_executable(${PROJECT_NAME}
        Program.cpp
        driver/Eink.cpp
        driver/Eink.h
        driver/X11Redirect.cpp
        driver/X11Redirect.h
)

target_link_libraries(${PROJECT_NAME}
        WaveshareDrivers
        ${OpenCV_LIBS}
        X11
        Xtst
)
