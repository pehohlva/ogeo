

DEPENDSPATH += $$PWD
INCLUDEPATH += $$PWD
# Input
QT       += core
LANGUAGE	= C

DEFINES += "READLINEHAVING_IN"
DEFINES += "ONBOARD_SQLITE3"

cache()
# history from console action sql and other download
LIBS += -lreadline

CONFIG   += console release

HEADERS += $$PWD/sqlite3.h $$PWD/console_table.h $$PWD/qt_sqlite3.h
SOURCES += $$PWD/sqlite3.c $$PWD/console_table.cpp $$PWD/qt_sqlite3.cpp
