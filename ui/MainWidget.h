//
// Created by kira on 25-4-7.
//

#ifndef MAINWIDGET_H
#define MAINWIDGET_H
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>


class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(QWidget* parent = 0);
    ~MainWidget();

private:
    QVBoxLayout* layout;
    QLabel* title;
    QPushButton* button;
    void onPushBtnClick();
    void manualClick();
};


#endif //MAINWIDGET_H
