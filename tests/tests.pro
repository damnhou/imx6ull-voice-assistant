QT += core network testlib
QT -= gui
CONFIG += testcase c++11 console
TEMPLATE = app
TARGET = voice-assistant-core-tests

INCLUDEPATH += ../src

SOURCES += \
    tst_core.cpp \
    ../src/cloud/iflytekauth.cpp \
    ../src/cloud/iatresultparser.cpp \
    ../src/domain/commandparser.cpp

HEADERS += \
    ../src/cloud/iflytekauth.h \
    ../src/cloud/iatresultparser.h \
    ../src/domain/commandparser.h

