QT += widgets concurrent
CONFIG += c++20 release
CONFIG -= debug
TEMPLATE = app
TARGET = KartonPlaner
DESTDIR = $$PWD
INCLUDEPATH += $$PWD/include
SOURCES += $$PWD/src/packing.cpp \
           $$PWD/src/app/main.cpp \
           $$PWD/src/app/project.cpp \
           $$PWD/src/app/theme.cpp \
           $$PWD/src/app/packview.cpp \
           $$PWD/src/app/mainwindow.cpp
HEADERS += $$PWD/include/kartonplaner/packing.hpp \
           $$PWD/src/app/project.hpp \
           $$PWD/src/app/theme.hpp \
           $$PWD/src/app/packview.hpp \
           $$PWD/src/app/mainwindow.hpp
QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic
macx:contains(QT_ARCH, arm64): QMAKE_CXXFLAGS += -include arm_acle.h
QMAKE_MACOSX_DEPLOYMENT_TARGET = 14.0
QMAKE_TARGET_BUNDLE_PREFIX = de.enes-tuerksever
VERSION = 1.0.0
