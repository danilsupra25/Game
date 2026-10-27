#ifndef AA_TILEBANK_H
#define AA_TILEBANK_H

// ============================================================
//  Загрузчик опциональных ТАЙЛСЕТОВ карт (PNG + JSON).
//  Тайлсет заменя собой процедурную запеканку земли Dungeon::buildGround():
//  пол/дорога/стены/верхние грани/опасность/вода/детали stamp'ятся в карту
//  один раз при генерации локации - дальше это обычная QImage, FPS не тратится.
//  Если тайлсета для биома нет (или спрайты выключены) - всё рисуется
//  процедурно, как раньше.
//
//  ГДЕ ИЩЕТ (папки просматриваются ВСЕ и сливаются, первый найденный ключ выигрывает):
//    <корень>/assets/tiles/, <корень>/assets/sprites/, <корень>/tiles/,
//    <корень>/assets/sprite/, <корень>/sprites/
//    где корень = папка запуска (cwd), папка с exe, и обе на 1 и 2 уровня выше.
//
//  ИМЯ ФАЙЛА = КЛЮЧ: «tiles_<биом>» (или «tile_<биом>»):
//    tiles_ash, tiles_ember, tiles_frost, tiles_crystal, tiles_swamp,
//    tiles_ruins, tiles_lava, tiles_void, tiles_town.
//  Понимаются и русские/альтернативные имена: tiles_пепел, tiles_лес,
//  tiles_снег, tiles_болото, tiles_руины, tiles_лава, tiles_пустота,
//  tiles_город ... Либо любое имя png + поле "key" в соседнем .json.
//
//  Формат JSON рядом с png (без него ячейки читаются в стандартном порядке):
//  {
//    "key": "tiles_ash",       // необязательно: ключ вместо имени файла
//    "cell": 40,               // размер ячейки в пикселях (стандарт 40 = tileSize)
//    "cols": 7,                // ячеек в строке атласа
//    "cells": { "floor0": 0, "floor1": 1, ..., "deco5": 22 }   // имя -> индекс
//  }
//  Или "cells": ["floor0","floor1",...] - список имён по порядку.
//
//  ИМЕНА ЯЧЕЕК (все необязательны, чего нет - рисуется процедурно):
//    floor0..floor4   варианты пола
//    road0..road2     дорога/тропа
//    wall0..wall2     кладка стены
//    walltop0..1      верхняя грань стены (смотрит на игрока)
//    hazard0..1       опасная зона
//    water0..1        вода
//    deco0..deco5     прозрачные детали пола (трава, трещины, камешки...)
//
//  Ячейки любого размера: при запекании масштабируются до tileSize.
//  Встроенный набор (Qt-ресурс) покрывает все 9 биомов; выключается вместе
//  со спрайт-листами кнопкой «Спрайт-листы» в настройках (видео).
// ============================================================

#include <QImage>
#include <QHash>
#include <QRect>
#include <QString>
#include <QStringList>

#include "Core/Types.h"

namespace aa {
namespace TileBank {

struct Tileset
{
    QImage img;                       // атлас
    int cell = 40;                    // размер ячейки в атласе
    QHash<QString, QRect> cells;      // имя ячейки -> прямоугольник в атласе
    bool valid = false;

    QRect rect(const QString &name) const { return cells.value(name); }
    int count(const QString &prefix) const;   // сколько подряд идущих prefix0..N-1
};

void scan();                                  // пересканировать папки
void setOverrideDir(const QString &dir);      // тесты: сканировать только эту папку
const Tileset *get(Biome b);                  // 0, если выключено или нет тайлсета
const Tileset *getByKey(const QString &key);
QStringList loaded();                         // список загруженных ключей
QStringList loadedPairs();                    // "key <- источник" для журнала
QStringList reportLines();                    // готовые строки журнала
QStringList skipped();                        // png, отброшенные фильтром имён
void setEnabled(bool on);                     // общий выключатель (настройки)
bool enabled();

} // namespace TileBank
} // namespace aa

#endif // AA_TILEBANK_H
