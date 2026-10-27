#ifndef AA_DATA_H
#define AA_DATA_H

#include "Core/Types.h"
#include <QVector>
#include <QHash>

namespace aa {
namespace Data {

void ensureLoaded();

const QVector<ItemBaseDef> &bases();
const QVector<AffixDef>    &affixes();
const QVector<UniqueDef>   &uniques();
const QVector<ActiveSkill> &skills();
const QVector<SkillNode>   &nodes();
const QVector<EnemyDef>    &enemies();
const QVector<LocationDef> &locations();
const QVector<EliteAffix>  &eliteAffixes();

const ItemBaseDef *base(int id);
const AffixDef    *affix(int id);
const UniqueDef   *unq(int id);
const ActiveSkill *skill(int id);
const SkillNode   *node(int id);
const EnemyDef    *enemy(int id);
const LocationDef *loc(int id);

// подбор пулов
QVector<int> basesForSlot(Slot s, int ilvl);          // подходящие базы под уровень предмета
QVector<int> affixesFor(Slot s, int ilvl, bool prefix);
QVector<int> basesOfBag(BagKind k);

// именование
QString rareName(const Item &it);
QString itemNameFull(const Item &it);
QString describeMods(const StatMods &m, float mult = 1.0f);
QString skillDesc(const ActiveSkill &s, int rank);

// мир
int locationCount();
int regionCount();
QString regionName(int region);
QColor  regionColor(int region);
QVector<int> locationsOfRegion(int region);
int firstLocation();

// геометрия паутины
float ringRadius(int ring);
float angDiff(float a, float b);

// паутина навыков
QVector<int> nodesOfRing(int ring);
QPointF nodePos(const SkillNode &n);      // координаты в нормализованном пространстве [-1,1]
int nodeReqLevel(int ring);

// враги
QVector<int> enemyPoolForLocation(int locId);
QString enemyNameFor(int defId, int levelDelta);

} // namespace Data
} // namespace aa

#endif // AA_DATA_H
