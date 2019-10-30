#-------------------------------------------------
#
# Project created by QtCreator 2014-02-26T12:48:03
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GUI
TEMPLATE = app


SOURCES += main.cpp\
    mainwindow.cpp\
    waveformwindow.cpp\
    systemsettings.cpp \
    videomode.cpp \
    patternmode.cpp \
    batchfilespage.cpp \
    peripherals.cpp \
    firmwarepage.cpp \
    HiresLib/flashloader.c \
    HiresLib/batchfile.c \
    HiresLib/compress.c \
    HiresLib/Error.c \
    HiresLib/firmware.c \
    HiresLib/flashimage.c \
    HiresLib/API.c \
    HiresLib/usb.c \
    HiresLib/pattern.c \
    HiresLib/splash.c \

HEADERS  += mainwindow.h\
    patternelement.h\
    waveformwindow.h\
    FirmwareW.h \
    PtnImage.h \
    version.h \
    HiresLib/usb.h\
    HiresLib/Error.h\
    HiresLib/Common.h\
    HiresLib/firmware.h\
    HiresLib/splash.h \
    HiresLib/compress.h \
    HiresLib/flashloader.h \
    HiresLib/pattern.h \
    HiresLib/batchfile.h \
    HiresLib/API.h

INCLUDEPATH += "hidapi-master\\hidapi"
INCLUDEPATH += "HiresLib"

FORMS    += \
    mainwindow.ui

RESOURCES += \
    HiRes.qrc

# Icons
macx{
ICON = icons/GUI_icon.icns
}
win32{
RC_FILE = icons/GUI_icon.rc
}

win32:CONFIG(release, debug|release): LIBS += -L../GUI/hidapi-master/windows/release/ -lhidapi
else:win32:CONFIG(debug, debug|release): LIBS += -L../GUI/hidapi-master/windows/debug/ -lhidapi

win32: LIBS += -lSetupAPI

INCLUDEPATH += ../GUI/hidapi-master/hidapi
DEPENDPATH += ../GUI/hidapi-master/hidapi

OTHER_FILES +=  icons\GUI_icon.rc \
                icons\GUI_icon.ico \
    Icons/GUI_icon.rc
