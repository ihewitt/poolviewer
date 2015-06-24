TEMPLATE = app
QT = gui core widgets printsupport serialport

VERSION = 0.4

CONFIG += qt warn_on debug_and_release

DESTDIR = bin

release {
	OBJECTS_DIR = build/release
	MOC_DIR = build/release
	UI_DIR = build/release
	}

debug {
	OBJECTS_DIR = build/debug
	MOC_DIR = build/debug
	UI_DIR = build/debug
	}


unix {
	LIBS += -lusb-1.0
	INCLUDEPATH += ./src
}

win32 {
	INCLUDEPATH += ./src c:/libusb/include
	QMAKE_LIBDIR += C:/libusb/ms32/dll
	LIBS += -llibusb-1.0
}

CONFIG(debug, debug|release) {
	unix: TARGET = $$join(TARGET,,,_g)
	win32: TARGET = $$join(TARGET,,d)
}

target.path = $$PREFIX/usr/bin
INSTALLS += target

FORMS = ui/summary.ui ui/sync.ui ui/config.ui ui/upload.ui
HEADERS = src/uploadimpl.h src/syncimpl.h src/configimpl.h src/summaryimpl.h src/graphwidget.h src/datastore.h src/calendar.h \
    src/usblink.h \
    src/poolmate.h \
    src/logging.h \
    src/FIT.hpp src/stdintfwd.hpp src/GarminConvert.hpp
SOURCES = src/uploadimpl.cpp src/syncimpl.cpp src/configimpl.cpp src/summaryimpl.cpp src/main.cpp src/graphwidget.cpp src/datastore.cpp src/poolmate.c src/calendar.cpp \
    src/usblink.cpp \
    src/FIT.cpp src/GarminConvert.cpp
