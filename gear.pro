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
    glwidget.cpp

HEADERS  += mainwindow.h \
    glwidget.h

FORMS    += mainwindow.ui

OTHER_FILES += \
    bgr.png

RESOURCES += \
    gear_resources.qrc
