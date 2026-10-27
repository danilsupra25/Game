// ============================================================
//  «ПЕПЕЛ И ЯНТАРЬ»  —  action-RPG на Qt 5.9.1
//  Точка входа
// ============================================================
#include <QApplication>
#include <QCoreApplication>
#include <QFont>
#include <QFontDatabase>
#include <QTextCodec>
#include <QDir>

#include "UI/MainWindow.h"
#include "Core/Data.h"
#include "Core/Audio.h"
#include "Core/Rng.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // корректная работа с UTF-8 строками в исходниках
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif
    // HighDPI должен включаться до создания QApplication
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Пепел и Янтарь"));
    app.setApplicationVersion(QStringLiteral("1.0"));
    app.setOrganizationName(QStringLiteral("AshAmber"));

    // базовый шрифт интерфейса
    QFont def = app.font();
    def.setPointSize(qMax(9, def.pointSize()));
    app.setFont(def);

    aa::Data::ensureLoaded();
    aa::Audio::inst().init();

    aa::MainWindow w;
    w.show();
    return app.exec();
}
