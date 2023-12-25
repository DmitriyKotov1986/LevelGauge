QT -= gui
QT += network
QT += sql

CONFIG += c++11 console
CONFIG -= app_bundle

INCLUDEPATH += $$PWD/../../Common/Headers

LIBS+= -L$$PWD/../../Common/Lib -lCommon

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        fafnir.cpp \
        main.cpp \
        sens.cpp \
        senspassive.cpp \
        tconfig.cpp \
        tlevelgauge.cpp \
        tlevelgaugemonitoring.cpp \
        tls2.cpp \
        tls4.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    fafnir.h \
    sens.h \
    senspassive.h \
    tconfig.h \
    tlevelgauge.h \
    tlevelgaugemonitoring.h \
    tls2.h \
    tls4.h

DISTFILES += \
    ToDo

RC_ICONS += res/LevelGauge.ico
