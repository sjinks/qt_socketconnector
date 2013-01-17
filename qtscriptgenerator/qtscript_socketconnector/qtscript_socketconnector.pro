TARGET   = qtscript_socketconnector
QT      += script network
QT      -= gui
CONFIG  += plugin release
TEMPLATE = lib

SOURCES += ../generated_cpp/socketconnector/plugin.cpp
exists(../generated_cpp/socketconnector/plugin.h) {
	HEADERS += ../generated_cpp/socketconnector/plugin.h
}

INCLUDEPATH += ../../src/

include(../generated_cpp/socketconnector/socketconnector.pri)
