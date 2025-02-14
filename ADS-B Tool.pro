QT       += core gui quickwidgets location webenginewidgets webchannel network charts sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    board_read.cpp \
    filewriter.cpp \
    main.cpp \
    mainwindow.cpp \
    plot.cpp \
    process_.cpp \
    processmanager.cpp \
    sharedmemorymanager.cpp \
    sharedsource.cpp

HEADERS += \
    board_read.h \
    filewriter.h \
    globalBuffer.h \
    mainwindow.h \
    plot.h \
    predefine.h \
    process_.h \
    processmanager.h \
    sharedmemorymanager.h \
    sharedsource.h \
    taskQueue.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

LIBS += C:/jiangrd3/ads-b/adsb/libiio.lib

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

RC_ICONS += \
    icon.ico

RESOURCES += \
    resources.qrc

DISTFILES += \
    leafletmap.html \
    plane.png
