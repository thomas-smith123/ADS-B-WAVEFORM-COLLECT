QT       += core gui quickwidgets location webenginewidgets webchannel network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    board_read.cpp \
    icaoobject.cpp \
    main.cpp \
    process.cpp \
    widget.cpp

HEADERS += \
    board_read.h \
    icaoobject.h \
    iio.h \
    overall_control.h \
    process.h \
    widget.h

FORMS += \
    widget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


# win32:LIBS += "$$PWD/libiio.lib"
LIBS += C:/jiangrd3/ads-b/adsb/libiio.lib

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

RESOURCES += resources.qrc
