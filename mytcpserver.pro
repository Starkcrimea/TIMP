QT -= gui
QT += network

CONFIG += c++11 console
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

TARGET = GuessNumberServer

SOURCES += \
    main.cpp \
    mytcpserver.cpp \

HEADERS += \
    mytcpserver.h \

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
