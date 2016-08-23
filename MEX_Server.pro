#-------------------------------------------------
#
# Project created by QtCreator 2016-02-09T11:27:43
#
#-------------------------------------------------

QT       += core
QT       += network
QT       -= gui
QT       += sql

TARGET = MEX_Server
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    mex_server.cpp \
    mex_serverthread.cpp \
    mex_order.cpp \
    mex_xmlprocessor.cpp

HEADERS += \
    mex_server.h \
    mex_serverthread.h \
    mex_order.h \
    mex_xmlprocessor.h \
    mex_product.h
