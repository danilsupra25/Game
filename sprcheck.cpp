// Мини-проверка поиска спрайт-листов в пользовательском сценарии.
#include <QCoreApplication>
#include <QImage>
#include <QDir>
#include <QDebug>
#include "Game/SpriteBank.h"
using namespace aa;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    SprBank::scan();
    qInfo("loaded: %s", qPrintable(SprBank::loadedPairs().join(QStringLiteral(" | "))));
    qInfo("skipped(%d): %s", SprBank::skipped().size(), qPrintable(SprBank::skipped().join(QStringLiteral(", "))));
    qInfo("dir: %s", qPrintable(SprBank::spritesDir()));
    return 0;
}
