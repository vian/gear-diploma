#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QGLWidget>
#include <QtOpenGL>

class GLWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GLWidget(QWidget *parent = 0);
    virtual ~GLWidget();

signals:

public slots:
    void zoomIn();
    void zoomOut();
    void restoreView();
    void toggleTooth();
    void toggleWireframe();
    void toggleBgWireframe();
    void zoomToTooth();

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();
    void updateGL();

    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
private:
    void drawAxes();
    void drawTooth();
    void zoomTo(int toothNum);

    QPoint prevPos;
    bool lbut;
    bool rbut;
    bool waitForToothSelection;
    bool pickingMode;
    GLfloat zoom;
    int toothSelected;

    GLfloat rotAccum[16];
    GLfloat tx, ty, rx, ry;

    QGLContext *context;
    QGLFramebufferObject *fbo;

    GLfloat *toothData;
    GLushort *toothIndices;
};

#endif // GLWIDGET_H
