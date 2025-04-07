//
// Created by kira on 25-4-6.
//

#include "MainWindow.h"

MainWindow::MainWindow()
{
    this->setFixedSize(250, 122);
    this->setWindowTitle("E-ink debug view [MainWindow]");
    mainWidget = new MainWidget();
    this->setCentralWidget(mainWidget);
}

MainWindow::~MainWindow()
{
}

void MainWindow::renderToImage(QImage& image)
{
    QPainter painter(&image);
    render(&painter);
    painter.end();
}

void MainWindow::clickAt(int x, int y)
{
    QTest::mouseClick(this, Qt::LeftButton, Qt::NoModifier, QPoint(x, y));

    std::cout<<"Called"<<std::endl;
    // QMouseEvent* event = new QMouseEvent(
    //     QEvent::MouseButtonPress,
    //     QPoint(x, y),
    //     Qt::LeftButton,
    //     Qt::LeftButton,
    //     Qt::NoModifier
    // );
    //
    // QApplication::postEvent(this, event);
    //
    // QMouseEvent* releaseEvent = new QMouseEvent(
    //     QEvent::MouseButtonRelease,
    //     QPoint(x, y),
    //     Qt::LeftButton,
    //     Qt::LeftButton,
    //     Qt::NoModifier);
    // QApplication::postEvent(this, releaseEvent);
}

void MainWindow::mousePressEvent(QMouseEvent* event)
{
    std::cout<<"pressed"<<std::endl;
    QMainWindow::mousePressEvent(event);
}
