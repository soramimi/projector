DESTDIR = $$PWD/_bin

TARGET = projector
TEMPLATE = app
CONFIG -= qt
CONFIG += c++17

INCLUDEPATH += $$PWD/../FileType/lib

QMAKE_CXXFLAGS += -include ../FileType/src/fileconfig.h

LIBS += $$PWD/../FileType/lib/libfile.a

SOURCES += \
	Projector.cpp \
	joinpath.cpp \
	../FileType/src/FileType.cpp \
	../FileType/src/magic_mgc.c \
	main.cpp

HEADERS += \
	Projector.h \
	joinpath.h \
	../FileType/src/FileType.h \
	../FileType/src/magic_mgc.h


