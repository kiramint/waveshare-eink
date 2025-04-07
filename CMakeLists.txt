cmake_minimum_required(VERSION 3.21.0)

project(E-Paper VERSION 1.0)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Waveshare
add_subdirectory(lib/waveshare_eink_2in13_v4_opi3b)

# OpenCV
find_package(OpenCV REQUIRED)
include_directories(
        ${OpenCV_INCLUDE_DIRS}
)

# Qt6
set(QT6_DIR "/home/kira/Qt/6.9.0")
set(CMAKE_PREFIX_PATH "/home/kira/Qt/6.9.0/gcc_64;${CMAKE_PREFIX_PATH}")

### Let cmake handle Qt MOC preprocessor
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Test)

qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION
        Program.cpp
        drivers/Eink.cpp
        drivers/Eink.h
        ui/MainWindow.cpp
        ui/MainWindow.h
        ui/AppEntry.cpp
        ui/AppEntry.h
        ui/MainWidget.cpp
        ui/MainWidget.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        WaveshareDrivers
        ${OpenCV_LIBS}
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
)

# Qt6 Finalize
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(${PROJECT_NAME})
