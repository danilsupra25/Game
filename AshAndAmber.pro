#-------------------------------------------------
#  «ПЕПЕЛ И ЯНТАРЬ»  (Ash & Amber)
#  Action-RPG для Qt 5.9.1+  (C++11, Qt Widgets)
#
#  Сборка:
#     qmake AshAndAmber.pro
#     make        (Windows: mingw32-make / nmake)
#-------------------------------------------------

QT       += core gui widgets

TARGET    = AshAndAmber
TEMPLATE  = app

CONFIG   += c++11
CONFIG   -= app_bundle

# Звук не обязателен: если модуль QtMultimedia есть - включим процедурный синтез SFX
qtHaveModule(multimedia) {
    QT      += multimedia
    DEFINES += HAVE_AUDIO
}
qtHaveModule(opengl) {
    QT      += opengl
    DEFINES += HAVE_GL
}

RESOURCES += sprites.qrc

INCLUDEPATH += src

SOURCES += \
    src/main.cpp \
    src/Core/Rng.cpp src/Core/Binds.cpp src/Core/Ach.cpp \
    src/Core/Data.cpp \
    src/Core/Game.cpp \
    src/Core/Style.cpp \
    src/Core/Audio.cpp \
    src/World/Dungeon.cpp \
    src/Game/Sprites.cpp src/Game/GlFx.cpp src/Game/SpriteBank.cpp src/Game/TileBank.cpp \
    src/Game/GameView.cpp \
    src/UI/Hud.cpp \
    src/UI/Tooltip.cpp \
    src/UI/Panels.cpp \
    src/UI/MainWindow.cpp

HEADERS += \
    src/Core/Types.h \
    src/Core/Rng.h src/Core/Binds.h src/Core/Ach.h \
    src/Core/Data.h \
    src/Core/Game.h \
    src/Core/Style.h \
    src/Core/Audio.h \
    src/World/Dungeon.h \
    src/Game/Sprites.h src/Game/GlFx.h src/Game/SpriteBank.h src/Game/TileBank.h \
    src/Game/GameView.h \
    src/UI/Hud.h \
    src/UI/Tooltip.h \
    src/UI/Panels.h \
    src/UI/MainWindow.h

# UTF-8 исходники во всех компиляторах (важно: все имена/описания на русском)
win32-msvc*: QMAKE_CXXFLAGS += /utf-8
win32-clang-msvc*: QMAKE_CXXFLAGS += /utf-8
*-*g++*: QMAKE_CXXFLAGS += -finput-charset=UTF-8 -fexec-charset=UTF-8

# Небольшая оптимизация даже в debug - игра рисует много
QMAKE_CXXFLAGS_RELEASE += -O2
