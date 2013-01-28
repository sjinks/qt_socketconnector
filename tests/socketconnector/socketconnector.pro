QT      += network testlib
QT      -= gui
TARGET   = tst_socketconnector
CONFIG  += console
CONFIG  -= app_bundle
TEMPLATE = app
DESTDIR  = ../

lessThan(QT_MAJOR_VERSION, 5): CONFIG += qtestlib

SOURCES  = tst_socketconnector.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"

INCLUDEPATH += ../../src
DEPENDPATH  += ../../src
LIBS        += -L../../lib -lsocketconnector

unix: PRE_TARGETDEPS += $$PWD/../../lib/libsocketconnector.a
