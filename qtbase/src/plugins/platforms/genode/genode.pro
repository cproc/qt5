TARGET = qgenode

QT += \
    core-private \
    egl_support-private \
    eventdispatcher_support-private \
    fontdatabase_support-private \
    gui-private

CONFIG += exceptions c++2a

DEFINES += QT_NO_FOREACH

SOURCES =   main.cpp \
            qgenodeclipboard.cpp \
            qgenodecursor.cpp \
            qgenodeglcontext.cpp \
            qgenodeintegration.cpp \
            qgenodeplatformwindow.cpp \
            qgenodewindowsurface.cpp

HEADERS =   qgenodeclipboard.h \
            qgenodeintegrationplugin.h \
            qgenodeplatformwindow.h \
            qgenodescreen.h \
            qgenodesignalproxythread.h \
            qgenodewindowsurface.h

OTHER_FILES += genode.json

LIBS += -l:ld.lib.so

qtConfig(freetype): QMAKE_USE_PRIVATE += freetype

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QGenodeIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)
