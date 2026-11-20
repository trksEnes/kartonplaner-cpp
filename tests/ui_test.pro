QT += widgets concurrent testlib
CONFIG += console c++20 release
CONFIG -= app_bundle debug
TEMPLATE = app
TARGET = ui_tests
INCLUDEPATH += $$PWD/../include $$PWD/../src/app
SOURCES += $$PWD/ui_test.cpp $$PWD/../src/packing.cpp $$PWD/../src/app/project.cpp $$PWD/../src/app/theme.cpp \
           $$PWD/../src/app/packview.cpp $$PWD/../src/app/mainwindow.cpp
macx:contains(QT_ARCH, arm64): QMAKE_CXXFLAGS += -include arm_acle.h
QMAKE_MACOSX_DEPLOYMENT_TARGET = 14.0
