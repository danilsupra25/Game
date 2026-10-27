#ifndef AA_DUNGEON_H
#define AA_DUNGEON_H

#include <QVector>
#include <QPointF>
#include <QImage>
#include <QColor>
#include "Core/Types.h"

namespace aa {

enum TileKind { T_Floor = 0, T_Wall = 1, T_Hazard = 2, T_Water = 3, T_Road = 4 };

enum PropKind {
    Pk_None = 0,
    Pk_Rock, Pk_Tree, Pk_DeadTree, Pk_Crystal, Pk_Bone, Pk_Pillar, Pk_Bush,
    Pk_Mushroom, Pk_Ruin, Pk_Brazier, Pk_Statue, Pk_Stump, Pk_IceSpike,
    Pk_Vent, Pk_Obelisk, Pk_House, Pk_Stall, Pk_Barrel, Pk_Crate, Pk_Altar, Pk_Chest, Pk_ChestOpen
};

struct Prop
{
    PropKind kind = Pk_None;
    QPointF pos;
    float radius = 12.f;   // радиус столкновения
    float height = 24.f;   // визуальная высота
    bool solid = true;
    int seed = 0;
    float scale = 1.f;
    QColor tint;
    float lightRadius = 0.f;   // если >0 - источник света
    QColor lightColor;
};

struct BiomePal
{
    Biome biome = Bio_Ash;
    QColor floor1, floor2, floor3, wall, wallTop, wallDark, accent, hazard, sky, fog;
    QVector<PropKind> props;
    float propDensity = 0.05f;
    bool hasHazard = false;
    float darkness = 0.55f;       // насколько затемнять вне света игрока
    QColor lightColor;
};

BiomePal biomePalette(Biome b);

struct Dungeon
{
    int w = 0, h = 0;
    int tileSize = 40;
    QVector<quint8> tiles;
    QVector<Prop> props;
    QVector<QPointF> spawnPoints;     // места появления врагов
    QVector<QPointF> npcPoints;       // торговцы и прочее
    QPointF playerSpawn;
    QPointF exitPos;
    Biome biome = Bio_Ash;
    int seed = 0;
    int tier = 1;
    int kind = 0;                     // 0 бой, 1 город, 2 святилище
    QImage ground;
    QString name;

    bool valid() const { return w > 0 && h > 0; }
    int idx(int tx, int ty) const { return ty * w + tx; }
    quint8 tileAt(int tx, int ty) const
    {
        if (tx < 0 || ty < 0 || tx >= w || ty >= h) return T_Wall;
        return tiles.at(idx(tx, ty));
    }
    quint8 tileAtPx(const QPointF &p) const { return tileAt(int(p.x() / tileSize), int(p.y() / tileSize)); }
    bool solidAtPx(const QPointF &p) const { return tileAtPx(p) == T_Wall; }
    float widthPx() const { return float(w * tileSize); }
    float heightPx() const { return float(h * tileSize); }
    QRectF rectPx() const { return QRectF(0, 0, widthPx(), heightPx()); }

    QPointF randomFloor() const;
    QPointF randomFloorFarFrom(const QPointF &p, float minDist) const;
    QPointF nearestFree(const QPointF &p, float maxR = 90.f) const;
    void buildGround();
    void collideCircle(QPointF &pos, float radius) const;
    QVector<const Prop *> solidPropsNear(const QPointF &p, float r) const;
};

namespace DungeonGen {

Dungeon generate(int locationId, Biome biome, int tier, int kind, int seed);

} // namespace DungeonGen

} // namespace aa

#endif // AA_DUNGEON_H
