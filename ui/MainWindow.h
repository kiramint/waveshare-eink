//
// Created by kira on 25-4-6.
//

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <iostream>

#include <QMainWindow>
#include <QPainter>
#include <QtTest/QTest>

#include "MainWidget.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

    ~MainWindow();

    void renderToImage(QImage& image);

    void clickAt(int x, int y);
signals:

private:
    QVBoxLayout* layout;
    MainWidget* mainWidget;

protected:
    void mousePressEvent(QMouseEvent* event) override;
};


#endif //MAINWINDOW_H
