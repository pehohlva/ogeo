#-------------------------------------------------
#
# Project created by QtCreator 2013-11-30T10:40:21
#
#-------------------------------------------------

CONFIG +=   warn_off silent
QT       += core xml network sql gui
cache()
# object better
greaterThan( QT_MAJOR_VERSION, 4 ):QT *= widgets

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
