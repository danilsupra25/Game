#ifndef AA_SPRITEBANK_H
#define AA_SPRITEBANK_H

// Загрузчик опциональных спрайт-листов (PNG + JSON).
// Если листа нет - существо рисуется процедурно, как раньше.
//
// ГДЕ ИЩЕТ (папки просматриваются ВСЕ и сливаются, первый найденный ключ выигрывает):
//   <корень>/assets/sprites/, <корень>/assets/sprite/, <корень>/sprites/
//   где корень = папка запуска (cwd), папка с exe, и обе папки на 1 и 2 уровня выше.
//   В последнюю очередь - сами папка с exe и cwd (строгий фильтр имён, см. ниже).
//
// ЧТО БЕРЁТ (строгий фильтр, чтобы случайные картинки не становились «листами»):
//   - имя файла без .png = ключ: "player", "e_<id>", "b_<id>", "mob<номер>",
//     "npc<номер>" (или просто "npc"),
//     плюс псевдонимы героя: hero/knight/rycar/воин/рыцарь/герой/персонаж/игрок...;
//   - ИЛИ рядом лежит .json с тем же именем, а в нём поле "key": "<ключ>"
//     (тогда имя файла может быть любым).
//   Остальные png игнорируются и попадают в SprBank::skipped() (строка журнала).
//
// Формат JSON рядом с png:
// {
//   "key": "player",            // необязательно: ключ вместо имени файла
//   "frameW": 96, "frameH": 96, "anchorX": 48, "anchorY": 84, "fps": 12,
//   "anims": { "idle": [0, 3], "walk": [4, 11], "attack": [12, 17], "death": [18, 23] }
// }
// Кадры идут слева направо, строками. anchor - точка ног внутри кадра.
// Ключи: "player", id-строки врагов ("e_scav", "b_ash", ...), "mob<тинт>" ("mob5")
// и "npc<вид>" ("npc0" торговец ... "npc3" старейшина, либо общий "npc").
//
// Если своих листов нигде нет - подставляется ВСТРОЕННЫЙ набор (Qt-ресурс):
// Пепельный странник, мобы первой локации и NPC; отключается кнопкой
// «Спрайт-листы» в настройках (видео).

#include <QImage>
#include <QHash>
#include <QString>
#include <QStringList>

namespace aa {
namespace SprBank {

struct Anim { int from = 0; int to = 0; };

struct Sheet
{
    QImage img;
    int fw = 64, fh = 64;
    int ax = 32, ay = 56;
    float fps = 12.f;
    QHash<QString, Anim> anims;
    bool valid = false;
};

void scan();                        // пересканировать папки
void setOverrideDir(const QString &dir);   // тесты: сканировать только эту папку
const Sheet *get(const QString &key);
QStringList loaded();               // список загруженных ключей
QStringList loadedPairs();          // "key <- источник" для журнала
QStringList reportLines();          // готовые строки журнала о спрайтах
QStringList skipped();              // имена png, отброшенных фильтром имён
QStringList skippedDirs();          // папки, соответствующие skipped()
QString spritesDir();               // первая папка, откуда что-то загрузилось
void setEnabled(bool on);           // общий выключатель (настройки)
bool enabled();

} // namespace SprBank
} // namespace aa

#endif // AA_SPRITEBANK_H
