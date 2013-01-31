TEMPLATE = subdirs
SUBDIRS += socketconnector

greaterThan(QT_MAJOR_VERSION, 4) {
	SUBDIRS += qtbug27678
}
