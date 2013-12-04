#-------------------------------------------------
#
# Project created by QtCreator 2013-11-30T10:40:21
#
#-------------------------------------------------

CONFIG +=   warn_off silent
QT       += core xml network sql gui
# remove sql after sqlite3 inside
cache()
# object better
greaterThan( QT_MAJOR_VERSION, 4 ):QT *= widgets
# history from console action sql and other download
LIBS += -lreadline -lcurses 

CONFIG   += console release
CONFIG   -= app_bundle  debug
LIBS   += -lz
TEMPLATE = app
# open geo
TARGET = ogeo
BINDIR = /usr/bin
target.path = $$BINDIR
INSTALLS += target

SOURCES += main.cpp \
    geohandler.cpp \
    decompress.cpp \
    tablecreator.cpp

HEADERS += \
    geohandler.h \
    decompress.h \
    tablecreator.h
