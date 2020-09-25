TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    ecat.cpp

include(deployment.pri)
qtcAddDeployment()

QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -Wall

LIBS += -lrt
LIBS += -lpthread
LIBS += -lethercat

HEADERS += \
    ecat.h
