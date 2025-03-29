QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# 设置应用程序图标
win32:RC_ICONS += icons/loco.ico

SOURCES += \
    ddc_controller.cpp \
    main.cpp \
    mainwindow.cpp \
    settingsdialog.cpp \
    winhotkey.cpp

HEADERS += \
    ddc_controller.h \
    mainwindow.h \
    settingsdialog.h \
    winhotkey.h

FORMS += \
    mainwindow.ui \
    settingsdialog.ui

RESOURCES += \
    resources.qrc

TRANSLATIONS += \
    display_zh_CN.ts \
    display_en_US.ts
CONFIG += lrelease
CONFIG += embed_translations

# 添加Windows平台的库依赖
win32 {
    # 使用mingw时，确保正确链接库
    mingw {
        LIBS += -ldxva2 -lgdi32 -luser32
    } else {
        # MSVC编译器使用
        LIBS += -lDxva2 -lgdi32 -luser32
    }
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
