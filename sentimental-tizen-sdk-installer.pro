#-------------------------------------------------
#
# Project created by QtCreator 2012-01-15T01:56:01
#
#-------------------------------------------------

QT       += core gui network

TARGET = sentimental-tizen-installer
TEMPLATE = app

SOURCES += main.cpp \
    installwizard.cpp \
    tizenpackageindex.cpp \
    meego.cpp

HEADERS  += \
    installwizard.h \
    tizenpackageindex.h \
    meego.h

# quazip
INCLUDEPATH += quazip
DEPENDPATH += quazip
DEFINES += QUAZIP_STATIC
HEADERS += quazip/crypt.h \
           quazip/ioapi.h \
           quazip/JlCompress.h \
           quazip/quaadler32.h \
           quazip/quachecksum32.h \
           quazip/quacrc32.h \
           quazip/quazip.h \
           quazip/quazipfile.h \
           quazip/quazipfileinfo.h \
           quazip/quazipnewinfo.h \
           quazip/quazip_global.h \
           quazip/unzip.h \
           quazip/zip.h
SOURCES += quazip/qioapi.cpp \
           quazip/JlCompress.cpp \
           quazip/quaadler32.cpp \
           quazip/quacrc32.cpp \
           quazip/quazip.cpp \
           quazip/quazipfile.cpp \
           quazip/quazipnewinfo.cpp \
           quazip/unzip.c \
           quazip/zip.c
unix {
    LIBS += -lz
}
unix:!mac {
    CONFIG += qdbus
    SOURCES += pktransaction.cpp
    HEADERS += pktransaction.h
}

TRANSLATIONS += translations/i18n_ko_KR.ts
RESOURCES +=resources.qrc

mac {
    ICON    = images/tizen-sdk-installer-alternative.icns
    TARGET = SentimentalTizenSDKInstaller
    QMAKE_INFO_PLIST = Info_mac.plist
}
