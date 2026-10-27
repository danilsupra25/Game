QT       += core gui widgets
qtHaveModule(multimedia) {
    QT      += multimedia
    DEFINES += HAVE_AUDIO
}
qtHaveModule(opengl) {
    QT      += opengl
    DEFINES += HAVE_GL
}
TARGET    = shottool
TEMPLATE  = app
CONFIG   += c++11 console
CONFIG   -= app_bundle
RESOURCES += sprites.qrc

INCLUDEPATH += src

SOURCES += tests/shots.cpp \
    src/Core/Rng.cpp src/Core/Binds.cpp src/Core/Ach.cpp src/Core/Data.cpp src/Core/Game.cpp src/Core/Style.cpp src/Core/Audio.cpp \
    src/World/Dungeon.cpp src/Game/Sprites.cpp src/Game/GlFx.cpp src/Game/SpriteBank.cpp src/Game/TileBank.cpp src/Game/GameView.cpp \
    src/UI/Hud.cpp src/UI/Tooltip.cpp src/UI/Panels.cpp src/UI/MainWindow.cpp

HEADERS += src/Core/Types.h src/Core/Rng.h src/Core/Binds.h src/Core/Ach.h src/Core/Data.h src/Core/Game.h src/Core/Style.h \
    src/Core/Audio.h src/World/Dungeon.h src/Game/Actors.h src/Game/Sprites.h src/Game/GlFx.h src/Game/SpriteBank.h src/Game/TileBank.h src/Game/GameView.h \
    src/UI/Hud.h src/UI/Tooltip.h src/UI/Panels.h src/UI/MainWindow.h
