#include "Game/SpriteBank.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>

namespace aa {
namespace SprBank {

static QHash<QString, Sheet> g_sheets;
static QHash<QString, QString> g_src;      // ключ -> откуда взят (для журнала)
static QStringList g_order;
static QStringList g_skipped;
static QStringList g_skippedDir;
static QString g_dirReport;
static QString g_overrideDir;
static bool g_scanned = false;
static bool g_enabled = true;

// ---------- ключи ----------

// Псевдонимы героя: как пользователи только не называют своего странника.
static QString aliasKey(const QString &base)
{
    QString k = base.toLower().trimmed();
    static const char *al[] = {
        "hero", "knight", "rycar", "rytsar", "ricar", "voin", "warrior",
        "main", "char", "character", "pc", "me",
        "\xD0\xB3\xD0\xB5\xD1\x80\xD0\xBE\xD0\xB9",                 // герой
        "\xD1\x80\xD1\x8B\xD1\x86\xD0\xB0\xD1\x80\xD1\x8C",         // рыцарь
        "\xD0\xB2\xD0\xBE\xD0\xB8\xD0\xBD",                         // воин
        "\xD0\xBF\xD0\xB5\xD1\x80\xD1\x81\xD0\xBE\xD0\xBD\xD0\xB0\xD0\xB6", // персонаж
        "\xD0\xB8\xD0\xB3\xD1\x80\xD0\xBE\xD0\xBA"                  // игрок
    };
    for (unsigned i = 0; i < sizeof(al) / sizeof(al[0]); ++i)
        if (k == QString::fromUtf8(al[i])) return QStringLiteral("player");
    return k;
}

// Похоже ли имя на ключ существа: player / e_* / b_* / mob<цифры> / npc<цифры> / fx_*
static bool keyShapeOk(const QString &k)
{
    if (k == QStringLiteral("player")) return true;
    if (k.startsWith(QStringLiteral("e_")) && k.size() > 3) return true;
    if (k.startsWith(QStringLiteral("b_")) && k.size() > 3) return true;
    if (k.startsWith(QStringLiteral("fx_")) && k.size() > 3) return true;   // эффекты атак
    if (k.startsWith(QStringLiteral("mob")) && k.size() > 3) {
        for (int i = 3; i < k.size(); ++i)
            if (!k.at(i).isDigit()) return false;
        return true;
    }
    if (k == QStringLiteral("npc")) return true;
    if (k.startsWith(QStringLiteral("npc")) && k.size() > 3) {
        for (int i = 3; i < k.size(); ++i)
            if (!k.at(i).isDigit()) return false;
        return true;
    }
    return false;
}

// ---------- чтение одного листа ----------

static bool readJsonSidecar(const QString &jsonPath, Sheet &sh, QString &keyField)
{
    QFile jf(jsonPath);
    if (!jf.open(QIODevice::ReadOnly)) return false;
    QJsonDocument doc = QJsonDocument::fromJson(jf.readAll());
    jf.close();
    if (!doc.isObject()) return false;
    QJsonObject o = doc.object();
    keyField = o.value(QStringLiteral("key")).toString().trimmed();
    sh.fw = o.value(QStringLiteral("frameW")).toInt(64);
    sh.fh = o.value(QStringLiteral("frameH")).toInt(64);
    if (sh.fw < 4) sh.fw = 4;
    if (sh.fh < 4) sh.fh = 4;
    sh.ax = o.value(QStringLiteral("anchorX")).toInt(sh.fw / 2);
    sh.ay = o.value(QStringLiteral("anchorY")).toInt(sh.fh - 8);
    sh.fps = float(o.value(QStringLiteral("fps")).toDouble(12.0));
    if (sh.fps < 1.f) sh.fps = 1.f;
    QJsonArray kc = o.value(QStringLiteral("keyColor")).toArray();
    int keyR = -1, keyG = -1, keyB = -1;
    if (kc.size() == 3) { keyR = kc.at(0).toInt(0); keyG = kc.at(1).toInt(0); keyB = kc.at(2).toInt(0); }
    int tol = o.value(QStringLiteral("keyTolerance")).toInt(40);
    QJsonObject an = o.value(QStringLiteral("anims")).toObject();
    for (QJsonObject::const_iterator it = an.constBegin(); it != an.constEnd(); ++it) {
        QJsonArray a = it.value().toArray();
        if (a.size() < 2) continue;
        Anim am;
        am.from = a.at(0).toInt(0);
        am.to = a.at(1).toInt(am.from);
        sh.anims.insert(it.key(), am);
    }
    // chroma-key: вырезаем фон по цвету (удобно для сгенерированных листов)
    if (keyR >= 0 && !sh.img.isNull()) {
        QImage cimg = sh.img.convertToFormat(QImage::Format_ARGB32);
        for (int y = 0; y < cimg.height(); ++y) {
            QRgb *line = reinterpret_cast<QRgb *>(cimg.scanLine(y));
            for (int x = 0; x < cimg.width(); ++x) {
                QRgb c = line[x];
                if (qAbs(qRed(c) - keyR) <= tol && qAbs(qGreen(c) - keyG) <= tol &&
                    qAbs(qBlue(c) - keyB) <= tol)
                    line[x] = 0;
            }
        }
        sh.img = cimg;
    }
    return true;
}

static void finishAnims(Sheet &sh)
{
    if (!sh.anims.isEmpty()) return;
    // без json-анимаций: весь лист - одна зацикленная анимация
    int cols = qMax(1, sh.img.width() / sh.fw);
    int rows = qMax(1, sh.img.height() / sh.fh);
    Anim am;
    am.from = 0;
    am.to = cols * rows - 1;
    sh.anims.insert(QStringLiteral("idle"), am);
    sh.anims.insert(QStringLiteral("walk"), am);
}

// Положить лист в банк, если ключ ещё не занят (ранние папки выигрывают).
static void put(const QString &key, const Sheet &sh, const QString &srcNote)
{
    if (key.isEmpty() || g_sheets.contains(key)) return;
    g_sheets.insert(key, sh);
    g_src.insert(key, srcNote);
    g_order.append(key);
}

// Загрузить один лист из png (+ опциональный json). Возвращает ключ или "" если отброшен.
// Правило одно для всех папок: имя файла считается ключом только если оно похоже
// на ключ (player / e_* / b_* / mob<N> / псевдонимы героя); иначе спасает лишь
// явное поле "key" в соседнем json - это осознанный выбор автора листа.
static QString loadSheet(const QString &pngPath, const QString &jsonPath, const QString &srcNote,
                         bool *filteredOut)
{
    if (filteredOut) *filteredOut = false;
    QFileInfo fi(pngPath);
    QString base = fi.completeBaseName();
    Sheet sh;
    QString keyField;
    sh.img = QImage(pngPath);
    if (sh.img.isNull()) return QString();
    if (!jsonPath.isEmpty() && QFileInfo(jsonPath).exists())
        readJsonSidecar(jsonPath, sh, keyField);
    QString key = !keyField.isEmpty() ? aliasKey(keyField) : aliasKey(base);
    // тайлсеты карт (tiles_*/tile_*) - не листы и не мусор: ими занимается TileBank
    if (key.startsWith(QStringLiteral("tile"))) return QString();
    if (keyField.isEmpty() && !keyShapeOk(key)) { if (filteredOut) *filteredOut = true; return QString(); }
    finishAnims(sh);
    sh.valid = true;
    put(key, sh, srcNote);
    return key;
}

// ---------- папки ----------

static QStringList candidateDirs()
{
    QStringList roots;
    roots << QDir::currentPath()
          << QCoreApplication::applicationDirPath()
          << QDir::currentPath() + QStringLiteral("/..")
          << QCoreApplication::applicationDirPath() + QStringLiteral("/..")
          << QDir::currentPath() + QStringLiteral("/../..")
          << QCoreApplication::applicationDirPath() + QStringLiteral("/../..");
    QStringList subs;
    subs << QStringLiteral("/assets/sprites") << QStringLiteral("/assets/sprite")
         << QStringLiteral("/sprites");
    QStringList out;
    for (int r = 0; r < roots.size(); ++r)
        for (int i = 0; i < subs.size(); ++i) {
            QDir d(roots.at(r) + subs.at(i));
            if (d.exists()) out << QDir(d.absolutePath()).absolutePath();
        }
    // последние в очереди - сами папка с exe и cwd: так тоже можно,
    // но строгий фильтр имён не даст случайным картинкам стать «листами»
    out << QDir(QCoreApplication::applicationDirPath()).absolutePath()
        << QDir(QDir::currentPath()).absolutePath();
    // дедупликация
    QStringList res;
    for (int i = 0; i < out.size(); ++i)
        if (!res.contains(out.at(i))) res << out.at(i);
    return res;
}

static void scanDir(const QDir &dir)
{
    QStringList pngs = dir.entryList(QStringList() << QStringLiteral("*.png"), QDir::Files, QDir::Name);
    for (int i = 0; i < pngs.size(); ++i) {
        QString name = pngs.at(i);
        QString base = name;
        base.chop(4);
        QString pngPath = dir.absoluteFilePath(name);
        QString jsonPath = dir.absoluteFilePath(base + QStringLiteral(".json"));
        if (!QFileInfo(jsonPath).exists())
            jsonPath = dir.absoluteFilePath(base + QStringLiteral(".JSON"));
        bool filtered = false;
        QString key = loadSheet(pngPath, jsonPath, QFileInfo(pngPath).fileName(), &filtered);
        if (key.isEmpty() && filtered) { g_skipped << name; g_skippedDir << dir.absolutePath(); }
        else if (!key.isEmpty() && g_dirReport.isEmpty()) g_dirReport = dir.absolutePath();
    }
}

void scan()
{
    g_sheets.clear();
    g_src.clear();
    g_order.clear();
    g_skipped.clear();
    g_skippedDir.clear();
    g_dirReport.clear();
    g_scanned = true;
    if (!g_enabled) return;

    if (!g_overrideDir.isEmpty()) {
        QDir d(g_overrideDir);
        if (d.exists()) scanDir(d);
    } else {
        QStringList dirs = candidateDirs();
        for (int i = 0; i < dirs.size(); ++i) {
            QDir d(dirs.at(i));
            if (d.exists()) scanDir(d);
        }
    }
    // Встроенный набор листов: герой, ВСЕ мобы и боссы регионов, NPC и эффекты
    // атак (fx_*). Гарантия, что спрайтовая графика видна без единого файла на диске.
    static const char *bundled[] = {
        "player", "e_scav", "e_arch", "e_beetle", "e_axer", "e_dustm",
        "npc0", "npc1", "npc2", "npc3",
        // регион 2
        "e_fang", "e_treant", "e_hive", "e_savage", "e_thornp",
        // регион 3
        "e_icehunt", "e_golem", "e_cspider", "e_frostm", "e_swolf",
        // регион 4
        "e_rot", "e_devour", "e_morpri", "e_drown", "e_plague",
        // регион 5
        "e_elem", "e_forge", "e_eye", "e_warp", "e_horror",
        // боссы
        "b_ash", "b_thorn", "b_frost", "b_rot", "b_star",
        // эффекты атак и снарядов
        "fx_slash", "fx_claw", "fx_smash", "fx_cast",
        "fx_arrow", "fx_bolt", "fx_shard", "fx_zap", "fx_gob", "fx_soul", "fx_rock"
    };
    for (unsigned i = 0; i < sizeof(bundled) / sizeof(bundled[0]); ++i) {
        QString key = QString::fromLatin1(bundled[i]);
        if (g_sheets.contains(key)) continue;
        Sheet sh;
        sh.img = QImage(QStringLiteral(":/builtin/sprites/%1.png").arg(key));
        if (sh.img.isNull()) continue;
        QString keyField;
        readJsonSidecar(QStringLiteral(":/builtin/sprites/%1.json").arg(key), sh, keyField);
        finishAnims(sh);
        sh.valid = true;
        put(key, sh, QStringLiteral("встроенный лист"));
    }
}

void setOverrideDir(const QString &dir) { g_overrideDir = dir; g_scanned = false; }

const Sheet *get(const QString &key)
{
    if (!g_scanned) scan();
    if (!g_enabled) return 0;
    QHash<QString, Sheet>::const_iterator it = g_sheets.constFind(key.toLower());
    return it == g_sheets.constEnd() ? 0 : &it.value();
}

QStringList loaded() { if (!g_scanned) scan(); return g_order; }

QStringList loadedPairs()
{
    if (!g_scanned) scan();
    QStringList out;
    for (int i = 0; i < g_order.size(); ++i)
        out << QStringLiteral("%1 <- %2").arg(g_order.at(i), g_src.value(g_order.at(i)));
    return out;
}

QStringList reportLines()
{
    if (!g_scanned) scan();
    QStringList out;
    if (g_order.isEmpty()) {
        out << QStringLiteral("Спрайт-листы не найдены - рисуется моя процедурная графика.");
        return out;
    }
    QStringList show;
    for (int i = 0; i < g_order.size() && i < 8; ++i)
        show << QStringLiteral("%1 <- %2").arg(g_order.at(i), g_src.value(g_order.at(i)));
    if (g_order.size() > 8) show << QStringLiteral("…");
    out << QStringLiteral("Спрайт-листов загружено: %1. Папки: assets/sprites/, sprites/ или папка игры; имя png = ключ.")
              .arg(g_order.size());
    out << QStringLiteral("  %1").arg(show.join(QStringLiteral(", ")));
    if (!g_skipped.isEmpty()) {
        QString where = g_skippedDir.value(0);
        for (int i = 1; i < g_skippedDir.size(); ++i)
            if (g_skippedDir.at(i) != where) { where = QStringLiteral("несколько папок"); break; }
        QStringList names = g_skipped.mid(0, qMin(5, g_skipped.size()));
        if (g_skipped.size() > 5) names << QStringLiteral("… ещё %1").arg(g_skipped.size() - 5);
        out << QStringLiteral("  Пропущено файлов: %1 (%2) - имя не ключ и нет .json с полем \"key\". "
                              "Ключи: player, e_<id>, b_<id>, mob<N>, npc<N>. Папка: %3")
                  .arg(g_skipped.size()).arg(names.join(QStringLiteral(", "))).arg(where);
    }
    return out;
}

QStringList skipped() { if (!g_scanned) scan(); return g_skipped; }

QStringList skippedDirs() { if (!g_scanned) scan(); return g_skippedDir; }

QString spritesDir()
{
    if (!g_scanned) scan();
    if (!g_dirReport.isEmpty()) return g_dirReport;
    return QCoreApplication::applicationDirPath() + QStringLiteral("/assets/sprites");
}

void setEnabled(bool on)
{
    if (g_enabled == on) return;
    g_enabled = on;
    g_scanned = false;   // ленивый перескан при следующем обращении
}

bool enabled() { return g_enabled; }

} // namespace SprBank
} // namespace aa
