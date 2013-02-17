QT      += network testlib
QT      -= gui
TARGET   = tst_socketconnector
CONFIG  += console
CONFIG  -= app_bundle
TEMPLATE = app
DESTDIR  = ..

lessThan(QT_MAJOR_VERSION, 5): CONFIG += qtestlib

SOURCES  = tst_socketconnector.cpp

INCLUDEPATH += ../../src
DEPENDPATH  += ../../src
LIBS        += -L$$OUT_PWD/$$DESTDIR/../lib -lsocketconnector

unix: PRE_TARGETDEPS += $$OUT_PWD/$$DESTDIR/../lib/libsocketconnector.a
