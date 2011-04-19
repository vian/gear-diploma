#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "glwidget.h"

#include <QPainter>
#include <QLabel>
#include <QMouseEvent>

const QRect closeButtonRect = QRect(167, 61, 49, 48);
const QSize windowSize = QSize(934, 715);

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent, Qt::FramelessWindowHint),
    ui(new Ui::MainWindow)
{
    this->setAttribute(Qt::WA_TranslucentBackground);
    ui->setupUi(this);

    //ui->label->setAttribute(Qt::WA_TranslucentBackground);

    glWidget = new GLWidget(this);
    ui->verticalLayout->addWidget(glWidget);

    connect(ui->zoomIn, SIGNAL(clicked()), glWidget, SLOT(zoomIn()));
    connect(ui->zoomOut, SIGNAL(clicked()), glWidget, SLOT(zoomOut()));
    connect(ui->restoreView, SIGNAL(clicked()), glWidget, SLOT(restoreView()));
    connect(ui->tooth, SIGNAL(clicked()), glWidget, SLOT(toggleTooth()));
    connect(ui->toggleWireframe, SIGNAL(clicked()), glWidget, SLOT(toggleWireframe()));
    connect(ui->toggleBgWireframe, SIGNAL(clicked()), glWidget, SLOT(toggleBgWireframe()));
    connect(ui->zoomTooth, SIGNAL(clicked()), glWidget, SLOT(zoomToTooth()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
 {
     if (event->button() == Qt::LeftButton) {

         if (closeButtonRect.contains(event->pos())) {
             this->close();
         }

         dragPosition = event->globalPos() - frameGeometry().topLeft();
         event->accept();
     }
 }

 void MainWindow::mouseMoveEvent(QMouseEvent *event)
 {
     if (event->buttons() & Qt::LeftButton) {
         move(event->globalPos() - dragPosition);
         event->accept();
     }
 }

void MainWindow::paintEvent(QPaintEvent *e)
{
}
