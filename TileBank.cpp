#include "Game/TileBank.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>

namespace aa {
namespace TileBank {

static QHash<QString, Tileset> g_sets;
static QHash<QString, QString> g_src;      // ключ -> откуда взят (для журнала)
static QStringList g_order;
static QStringList g_skipped;
static QStringList g_skippedDir;
static QString g_dirReport;
static QString g_overrideDir;
static bool g_scanned = false;
static bool g_enabled = true;

// ---------- ключи ----------

// Канонические суффиксы 9 биомов: ключ тайлсета = "tiles_" + суффикс.
static const char *biomeSuffix[Bio_Count] = {
    "ash", "ember", "frost", "crystal", "swamp", "ruins", "lava", "void", "town"
};

// Псевдонимы биомов: как пользователи только не называют свои тайлсеты.
static QString canonBiomeSuffix(const QString &base)
{
    QString k = base.toLower().trimmed();
    if (k.isEmpty()) return QString();
    for (int i = 0; i < Bio_Count; ++i)
        if (k == QLatin1String(biomeSuffix[i])) return QLatin1String(biomeSuffix[i]);
    struct Al { const char *name; const char *canon; };
    static const Al al[] = {
        { "pustosh", "ash" },       { "pepel", "ash" },
        { "\xD0\xBF\xD0\xB5\xD0\xBF\xD0\xB5\xD0\xBB", "ash" },                     // пепел
        { "\xD1\x80\xD0\xB0\xD0\xB2\xD0\xBD\xD0\xB8\xD0\xBD\xD1\x8B", "ash" },     // равнины
        { "les", "ember" },         { "ugol", "ember" },     { "forest", "ember" },
        { "\xD0\xBB\xD0\xB5\xD1\x81", "ember" },                                    // лес
        { "\xD1\x83\xD0\xB3\xD0\xBE\xD0\xBB\xD1\x8C", "ember" },                    // уголь
        { "led", "frost" },         { "sneg", "frost" },      { "ice", "frost" },    { "snow", "frost" },
        { "\xD0\xBB\xD1\x91\xD0\xB4", "frost" },                                    // лёд
        { "\xD0\xBB\xD0\xB5\xD0\xB4", "frost" },                                    // лед
        { "\xD1\x81\xD0\xBD\xD0\xB5\xD0\xB3", "frost" },                            // снег
        { "\xD0\xBC\xD0\xBE\xD1\x80\xD0\xBE\xD0\xB7", "frost" },                    // мороз
        { "kristall", "crystal" },  { "grot", "crystal" },    { "cave", "crystal" },
        { "\xD0\xBA\xD1\x80\xD0\xB8\xD1\x81\xD1\x82\xD0\xB0\xD0\xBB\xD0\xBB", "crystal" }, // кристалл
        { "\xD0\xB3\xD1\x80\xD0\xBE\xD1\x82", "crystal" },                          // грот
        { "boloto", "swamp" },      { "top", "swamp" },       { "swampy", "swamp" },
        { "\xD0\xB1\xD0\xBE\xD0\xBB\xD0\xBE\xD1\x82\xD0\xBE", "swamp" },            // болото
        { "\xD1\x82\xD0\xBE\xD0\xBF\xD1\x8C", "swamp" },                            // топь
        { "ruiny", "ruins" },       { "ruin", "ruins" },
        { "\xD1\x80\xD1\x83\xD0\xB8\xD0\xBD\xD1\x8B", "ruins" },                    // руины
        { "magma", "lava" },        { "gornilo", "lava" },    { "forge", "lava" },
        { "\xD0\xBB\xD0\xB0\xD0\xB2\xD0\xB0", "lava" },                             // лава
        { "\xD0\xBC\xD0\xB0\xD0\xB3\xD0\xBC\xD0\xB0", "lava" },                     // магма
        { "\xD0\xB3\xD0\xBE\xD1\x80\xD0\xBD\xD0\xB8\xD0\xBB\xD0\xBE", "lava" },     // горнило
        { "pustota", "void" },      { "bezdna", "void" },     { "abyss", "void" },
        { "\xD0\xBF\xD1\x83\xD1\x81\xD1\x82\xD0\xBE\xD1\x82\xD0\xB0", "void" },     // пустота
        { "\xD0\xB1\xD0\xB5\xD0\xB7\xD0\xB4\xD0\xBD\xD0\xB0", "void" },             // бездна
        { "gorod", "town" },        { "derevnya", "town" },   { "city", "town" },    { "village", "town" },
        { "\xD0\xB3\xD0\xBE\xD1\x80\xD0\xBE\xD0\xB4", "town" },                     // город
        { "\xD0\xBF\xD0\xBE\xD1\x81\xD0\xB5\xD0\xBB\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB5", "town" }, // поселение
        { "\xD0\xB4\xD0\xB5\xD1\x80\xD0\xB5\xD0\xB2\xD0\xBD\xD1\x8F", "town" }      // деревня
    };
    for (unsigned i = 0; i < sizeof(al) / sizeof(al[0]); ++i)
        if (k == QString::fromUtf8(al[i].name)) return QLatin1String(al[i].canon);
    return QString();
}

// Похоже ли имя на ключ спрайт-листа (player / e_* / b_* / mob<N> / npc<N>):
// такие png в папке спрайтов - не мусор для тайлсетов, а чужие файлы: игнорируем молча.
static bool spriteKeyShape(const QString &k)
{
    if (k == QStringLiteral("player")) return true;
    if ((k.startsWith(QStringLiteral("e_")) || k.startsWith(QStringLiteral("b_"))) && k.size() > 3) return true;
    if (k.startsWith(QStringLiteral("fx_")) && k.size() > 3) return true;   // эффекты - не тайлы
    if (k.startsWith(QStringLiteral("mob")) && k.size() > 3) {
        for (int i = 3; i < k.size(); ++i)
            if (!k.at(i).isDigit()) return false;
        return true;
    }
    if (k.startsWith(QStringLiteral("npc")) && k.size() >= 3) {
        for (int i = 3; i < k.size(); ++i)
            if (!k.at(i).isDigit()) return false;
        return true;
    }
    return false;
}

// Ключ из имени файла или поля "key" соседнего json.
// Принимаются: tiles_<биом>, tile_<биом>, <биом> (в т.ч. русские псевдонимы).
static QString tileKey(const QString &base, const QString &keyField)
{
    QString k = (!keyField.isEmpty() ? keyField : base).toLower().trimmed();
    if (k.startsWith(QStringLiteral("tiles_"))) k = k.mid(6);
    else if (k.startsWith(QStringLiteral("tile_"))) k = k.mid(5);
    QString canon = canonBiomeSuffix(k);
    return canon.isEmpty() ? QString() : QStringLiteral("tiles_") + canon;
}

// ---------- раскладка атласа ----------

// Стандартный порядок ячеек (как в tools/make_tiles.py): 23 штуки.
static const char *stdNames[] = {
    "floor0", "floor1", "floor2", "floor3", "floor4",
    "road0", "road1", "road2",
    "wall0", "wall1", "wall2",
    "walltop0", "walltop1",
    "hazard0", "hazard1",
    "water0", "water1",
    "deco0", "deco1", "deco2", "deco3", "deco4", "deco5"
};

// Индекс ячейки -> прямоугольник в атласе; если не влезает - пустой QRect.
static QRect cellRect(const QImage &img, int idx, int cell, int cols)
{
    if (idx < 0 || cell < 1 || cols < 1) return QRect();
    int x = (idx % cols) * cell;
    int y = (idx / cols) * cell;
    if (x + cell > img.width() || y + cell > img.height()) return QRect();
    return QRect(x, y, cell, cell);
}

// Заполнить ts.cells по json (объект имя->индекс или список имён) либо
// по стандартному порядку. Уже известные из json имена не перезаписываются.
// colsJson - явное «cols» из json (0 = считать по ширине атласа).
static void buildCells(Tileset &ts, const QJsonObject &cellsObj, const QJsonArray &cellsArr, int colsJson)
{
    int cols = colsJson >= 1 ? colsJson : qMax(1, ts.img.width() / qMax(1, ts.cell));
    if (!cellsObj.isEmpty()) {
        for (QJsonObject::const_iterator it = cellsObj.constBegin(); it != cellsObj.constEnd(); ++it) {
            QRect rc = cellRect(ts.img, it.value().toInt(-1), ts.cell, cols);
            if (rc.isValid()) ts.cells.insert(it.key(), rc);
        }
    }
    if (!cellsArr.isEmpty()) {
        for (int i = 0; i < cellsArr.size(); ++i) {
            QString name = cellsArr.at(i).toString();
            if (name.isEmpty() || ts.cells.contains(name)) continue;
            QRect rc = cellRect(ts.img, i, ts.cell, cols);
            if (rc.isValid()) ts.cells.insert(name, rc);
        }
    }
    if (ts.cells.isEmpty()) {
        // нет json-раскладки: стандартный порядок ячеек
        int n = int(sizeof(stdNames) / sizeof(stdNames[0]));
        for (int i = 0; i < n; ++i) {
            QRect rc = cellRect(ts.img, i, ts.cell, cols);
            if (rc.isValid()) ts.cells.insert(QLatin1String(stdNames[i]), rc);
        }
    }
}

// ---------- чтение одного тайлсета ----------

static bool readJsonSidecar(const QString &jsonPath, Tileset &ts, QString &keyField,
                            QJsonObject &cellsObj, QJsonArray &cellsArr, int &colsJson)
{
    colsJson = 0;
    QFile jf(jsonPath);
    if (!jf.open(QIODevice::ReadOnly)) return false;
    QJsonDocument doc = QJsonDocument::fromJson(jf.readAll());
    jf.close();
    if (!doc.isObject()) return false;
    QJsonObject o = doc.object();
    keyField = o.value(QStringLiteral("key")).toString().trimmed();
    ts.cell = o.value(QStringLiteral("cell")).toInt(40);
    if (ts.cell < 4) ts.cell = 4;
    if (ts.cell > 512) ts.cell = 512;
    QJsonValue cv = o.value(QStringLiteral("cells"));
    if (cv.isObject()) cellsObj = cv.toObject();
    else if (cv.isArray()) cellsArr = cv.toArray();
    int cols = o.value(QStringLiteral("cols")).toInt(0);
    if (cols >= 1 && cols <= 4096) colsJson = cols;
    return true;
}

// Положить тайлсет в банк, если ключ ещё не занят (ранние папки выигрывают).
static void put(const QString &key, const Tileset &ts, const QString &srcNote)
{
    if (key.isEmpty() || g_sets.contains(key)) return;
    g_sets.insert(key, ts);
    g_src.insert(key, srcNote);
    g_order.append(key);
}

// Загрузить один тайлсет из png (+ опциональный json). Возвращает ключ или "" если отброшен.
// Правило как у спрайт-листов: имя файла считается ключом только если оно похоже
// на ключ (tiles_<биом> / tile_<биом> / <биом>, в т.ч. по-русски); иначе спасает
// лишь явное поле "key" в соседнем json.
static QString loadTiles(const QString &pngPath, const QString &jsonPath, const QString &srcNote,
                         bool *filteredOut)
{
    if (filteredOut) *filteredOut = false;
    QFileInfo fi(pngPath);
    QString base = fi.completeBaseName();
    Tileset ts;
    ts.img = QImage(pngPath);
    if (ts.img.isNull()) return QString();
    QString keyField;
    QJsonObject cellsObj;
    QJsonArray cellsArr;
    int colsJson = 0;
    if (!jsonPath.isEmpty() && QFileInfo(jsonPath).exists())
        readJsonSidecar(jsonPath, ts, keyField, cellsObj, cellsArr, colsJson);
    QString key = tileKey(base, keyField);
    if (key.isEmpty()) {
        // чужие файлы (спрайт-листы, в т.ч. с полем "key" в своём json)
        // молча пропускаем: они не мусор для тайлсетов
        if (spriteKeyShape(base.toLower().trimmed()) || spriteKeyShape(keyField.toLower().trimmed()))
            return QString();
        if (filteredOut) *filteredOut = true;
        return QString();
    }
    buildCells(ts, cellsObj, cellsArr, colsJson);
    if (ts.cells.isEmpty()) { if (filteredOut) *filteredOut = true; return QString(); }
    ts.valid = true;
    put(key, ts, srcNote);
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
    subs << QStringLiteral("/assets/tiles") << QStringLiteral("/assets/tile")
         << QStringLiteral("/tiles")
         << QStringLiteral("/assets/sprites") << QStringLiteral("/assets/sprite")
         << QStringLiteral("/sprites");
    QStringList out;
    for (int r = 0; r < roots.size(); ++r)
        for (int i = 0; i < subs.size(); ++i) {
            QDir d(roots.at(r) + subs.at(i));
            if (d.exists()) out << QDir(d.absolutePath()).absolutePath();
        }
    // последние в очереди - сами папка с exe и cwd: так тоже можно,
    // но строгий фильтр имён не даст случайным картинкам стать «тайлсетами»
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
        QString key = loadTiles(pngPath, jsonPath, QFileInfo(pngPath).fileName(), &filtered);
        if (key.isEmpty() && filtered) { g_skipped << name; g_skippedDir << dir.absolutePath(); }
        else if (!key.isEmpty() && g_dirReport.isEmpty()) g_dirReport = dir.absolutePath();
    }
}

void scan()
{
    g_sets.clear();
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
    // Встроенный набор тайлсетов (все 9 биомов): гарантия, что спрайтовая
    // земля видна даже без единого файла на диске.
    for (int i = 0; i < Bio_Count; ++i) {
        QString key = QStringLiteral("tiles_") + QLatin1String(biomeSuffix[i]);
        if (g_sets.contains(key)) continue;
        Tileset ts;
        ts.img = QImage(QStringLiteral(":/builtin/tiles/%1.png").arg(key));
        if (ts.img.isNull()) continue;
        QString keyField;
        QJsonObject cellsObj;
        QJsonArray cellsArr;
        int colsJson = 0;
        readJsonSidecar(QStringLiteral(":/builtin/tiles/%1.json").arg(key), ts, keyField, cellsObj, cellsArr, colsJson);
        buildCells(ts, cellsObj, cellsArr, colsJson);
        if (ts.cells.isEmpty()) continue;
        ts.valid = true;
        put(key, ts, QStringLiteral("встроенный тайлсет"));
    }
}

void setOverrideDir(const QString &dir) { g_overrideDir = dir; g_scanned = false; }

int Tileset::count(const QString &prefix) const
{
    int n = 0;
    while (cells.contains(prefix + QString::number(n))) ++n;
    return n;
}

const Tileset *get(Biome b)
{
    if (!g_scanned) scan();
    if (!g_enabled) return 0;
    int bi = int(b);
    if (bi < 0 || bi >= Bio_Count) bi = Bio_Ash;
    QHash<QString, Tileset>::const_iterator it =
        g_sets.constFind(QStringLiteral("tiles_") + QLatin1String(biomeSuffix[bi]));
    return it == g_sets.constEnd() ? 0 : &it.value();
}

const Tileset *getByKey(const QString &key)
{
    if (!g_scanned) scan();
    if (!g_enabled) return 0;
    QString k = key.toLower().trimmed();
    if (k.startsWith(QStringLiteral("tiles_"))) k = k.mid(6);
    else if (k.startsWith(QStringLiteral("tile_"))) k = k.mid(5);
    QString canon = canonBiomeSuffix(k);
    if (!canon.isEmpty()) k = canon;
    QHash<QString, Tileset>::const_iterator it = g_sets.constFind(QStringLiteral("tiles_") + k);
    return it == g_sets.constEnd() ? 0 : &it.value();
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
        out << QStringLiteral("Тайлсеты карт не найдены - земля рисуется процедурно.");
        return out;
    }
    QStringList show;
    for (int i = 0; i < g_order.size() && i < 8; ++i)
        show << QStringLiteral("%1 <- %2").arg(g_order.at(i), g_src.value(g_order.at(i)));
    if (g_order.size() > 8) show << QStringLiteral("…");
    out << QStringLiteral("Тайлсетов карт загружено: %1. Папки: assets/tiles/, assets/sprites/ или папка игры; имя png = ключ «tiles_<биом>».")
              .arg(g_order.size());
    out << QStringLiteral("  %1").arg(show.join(QStringLiteral(", ")));
    if (!g_skipped.isEmpty()) {
        QString where = g_skippedDir.value(0);
        for (int i = 1; i < g_skippedDir.size(); ++i)
            if (g_skippedDir.at(i) != where) { where = QStringLiteral("несколько папок"); break; }
        QStringList names = g_skipped.mid(0, qMin(5, g_skipped.size()));
        if (g_skipped.size() > 5) names << QStringLiteral("… ещё %1").arg(g_skipped.size() - 5);
        out << QStringLiteral("  Пропущено файлов: %1 (%2) - имя не «tiles_<биом>» и нет .json с полем \"key\". Папка: %3")
                  .arg(g_skipped.size()).arg(names.join(QStringLiteral(", "))).arg(where);
    }
    return out;
}

QStringList skipped() { if (!g_scanned) scan(); return g_skipped; }

void setEnabled(bool on)
{
    if (g_enabled == on) return;
    g_enabled = on;
    g_scanned = false;   // ленивый перескан при следующем обращении
}

bool enabled() { return g_enabled; }

} // namespace TileBank
} // namespace aa
