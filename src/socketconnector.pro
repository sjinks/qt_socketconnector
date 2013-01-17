QT      -= gui
QT      += network
TARGET   = socketconnector
TEMPLATE = lib
CONFIG  += staticlib create_prl release
DESTDIR  = ../lib

HEADERS = \
	socketconnector.h \
	socketconnector_p.h

SOURCES = \
	socketconnector.cpp \
	socketconnector_p.cpp

headers.files = socketconnector.h

unix {
	CONFIG += create_pc
	headers.path = /usr/include

	QMAKE_PKGCONFIG_NAME        = socketconnector
	QMAKE_PKGCONFIG_DESCRIPTION = "SocketConnector for Qt"
	QMAKE_PKGCONFIG_LIBDIR      = $$target.path
	QMAKE_PKGCONFIG_INCDIR      = $$headers.path
	QMAKE_PKGCONFIG_DESTDIR     = pkgconfig
}
else {
	headers.path = $$DESTDIR
}

unix:!symbian {
	maemo5 {
		target.path = /opt/usr/lib
	}
	else {
		target.path = /usr/lib
	}
}

INSTALLS += target headers
