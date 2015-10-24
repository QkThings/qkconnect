#-------------------------------------------------
#
# Project created by QtCreator 2014-11-22T15:46:35
#
#-------------------------------------------------

QT       += core network serialport
QT       -= gui

TARGET = qkconnect
CONFIG += console
CONFIG -= app_bundle
CONFIG += c++11

TEMPLATE = app

INCLUDEPATH += \
include \
src \
src/include \
 ../utils

SOURCES += \
src/main.cpp \
    src/clhandler.cpp \
    src/qkconnectserver.cpp \
    src/qkconnserial.cpp \
    src/qkconn.cpp \
    src/qkconnthread.cpp \
    src/qkconnloopback.cpp \
    src/qkclientthread.cpp \
    src/qksocket.cpp \
    src/qkserver.cpp \
    src/qkspyserver.cpp \
    ../utils/qkutils.cpp \
    ../utils/qkcore.cpp \
    src/qkprotocol.cpp \
    src/qkprotocolserial.cpp

HEADERS += \
    include/qkconnect_global.h \
    src/clhandler.h \
    src/qkconnectserver.h \
    src/qkconnserial.h \
    src/qkconn.h \
    src/qkconnthread.h \
    src/qkconnloopback.h \
    src/qkclientthread.h \
    src/qksocket.h \
    src/qkserver.h \
    src/qkspyserver.h \
    ../utils/qkutils.h \
    ../utils/qkcore.h \
    src/qkprotocol.h \
    src/qkprotocolserial.h

CONFIG(debug, debug|release){
    DESTDIR = debug
} else {
    DESTDIR = release
}

OBJECTS_DIR = obj
MOC_DIR = moc
RCC_DIR = rcc
UI_DIR = ui
