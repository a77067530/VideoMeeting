QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include(./netapi/netapi.pri)
INCLUDEPATH += ./netapi

include(./MD5/MD5.pri)
INCLUDEPATH += ./MD5

include(./AudioApi/audioapi.pri)
INCLUDEPATH += ./AudioApi

include(./VideoApi/VideoApi.pri)
INCLUDEPATH += ./VideoApi

SOURCES += \
    ckernel.cpp \
    logindialog.cpp \
    main.cpp \
    roomdialog.cpp \
    usershow.cpp \
    wechatdialog.cpp

HEADERS += \
    ckernel.h \
    logindialog.h \
    roomdialog.h \
    usershow.h \
    wechatdialog.h

FORMS += \
    logindialog.ui \
    roomdialog.ui \
    usershow.ui \
    wechatdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Resource.qrc
