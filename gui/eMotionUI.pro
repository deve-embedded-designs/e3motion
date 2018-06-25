#-------------------------------------------------
#
# Project created by QtCreator 2016-07-07T22:47:02
#
#-------------------------------------------------

QT       += core gui printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets serialport

TARGET = eMotionUI8
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp\
        Driver.cpp \
    HandlerSerial.cpp\
    qcustomplot.cpp \
    pugixml/pugixml.cpp \
    Simulator.cpp \
    lib/comms/CommandMessage.cpp \
    lib/comms/TelemetryMessage.cpp

HEADERS  += mainwindow.h\
        HandlerSerial.h\
        Driver.h \
    qcustomplot.h \
    pugixml/pugiconfig.hpp \
    ThreadSafeDS/CircularVector.hpp \
    ThreadSafeDS/List.hpp \
    ThreadSafeDS/Queue.hpp \
    Simulator.h \
    lib/comms/CommandMessage.h \
    lib/comms/TelemetryMessage.h

FORMS    += mainwindow.ui

RESOURCES += \
    icons.qrc

