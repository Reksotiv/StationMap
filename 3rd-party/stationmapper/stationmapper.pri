QMAKE_CFLAGS += -std=c99

HEADERS += \
    $$PWD/include/loadbmp.h \
    $$PWD/include/stationmapper.h

SOURCES += $$PWD/src/stationmapper.c 

INCLUDEPATH += $$PWD/include

DISTFILES += \
    $$PWD/readme.md
