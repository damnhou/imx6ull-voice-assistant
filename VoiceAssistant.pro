QT += core gui widgets network multimedia websockets

TARGET = imx6ull-voice-assistant
TEMPLATE = app
CONFIG += c++11 warn_on
DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += $$PWD/src

SOURCES += \
    src/main.cpp \
    src/app/applicationcontroller.cpp \
    src/audio/pcmaudiocapture.cpp \
    src/bsp/sysfsleddevice.cpp \
    src/cloud/iflytekauth.cpp \
    src/cloud/iflytekiatclient.cpp \
    src/cloud/iatresultparser.cpp \
    src/config/appconfig.cpp \
    src/domain/commandparser.cpp \
    src/ui/mainwindow.cpp

HEADERS += \
    src/app/applicationcontroller.h \
    src/audio/pcmaudiocapture.h \
    src/bsp/ileddevice.h \
    src/bsp/sysfsleddevice.h \
    src/cloud/iflytekauth.h \
    src/cloud/iflytekiatclient.h \
    src/cloud/iatresultparser.h \
    src/config/appconfig.h \
    src/domain/commandparser.h \
    src/ui/mainwindow.h

RESOURCES += resources/resources.qrc

unix:!android {
    target.path = /opt/voice-assistant/bin
    config_files.files = config/voice_assistant.ini.example config/userwords.txt
    config_files.path = /opt/voice-assistant/config
    INSTALLS += target config_files
}

