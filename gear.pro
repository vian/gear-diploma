#-------------------------------------------------
#
# Project created by QtCreator 2011-03-14T19:59:46
#
#-------------------------------------------------

QT       += core gui opengl

TARGET = gear
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    glwidget.cpp \
    matrix4.cpp \
    vector3d.cpp

HEADERS  += mainwindow.h \
    glwidget.h \
    vector3d.h \
    matrix4.h

FORMS    += mainwindow.ui

OTHER_FILES += \
    bgr.png

RESOURCES += \
    gear_resources.qrc
