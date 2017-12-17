TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += /usr/lib/llvm-3.9/include
LIBS += -L/usr/lib/llvm-3.9/lib `llvm-config-3.9 --libs`

SOURCES += main.cpp

HEADERS +=

