//
// Created by kira on 25-4-7.
//

#include "MainWidget.h"

MainWidget::MainWidget(QWidget* parent)
{
    title = new QLabel();
    button = new QPushButton("Click me!");
    button->setFixedSize(QSize(250, 122));

    connect(button,&QPushButton::clicked,this,&MainWidget::onPushBtnClick);

    title->setAlignment(Qt::AlignCenter);
    title->setText("Hello Eink");
    layout = new QVBoxLayout();
    // layout->addWidget(title);
    layout->addWidget(button);
    this->setLayout(layout);
}

MainWidget::~MainWidget()
{
}

void MainWidget::onPushBtnClick()
{
    qDebug() << "Clicked !!";
}


