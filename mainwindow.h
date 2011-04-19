#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>

class GLWidget;

namespace Ui {
    class MainWindow;
}

class QLabel;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *e);

private:
    Ui::MainWindow *ui;
    GLWidget *glWidget;
    QPoint dragPosition;
};

#endif // MAINWINDOW_H
