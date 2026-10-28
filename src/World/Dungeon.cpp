#include "World/Dungeon.h"
#include "Core/Style.h"
#include "Core/Rng.h"
#include "Game/TileBank.h"

#include <QtMath>
#include <QPainter>
#include <QPainterPath>

namespace aa {

// ============================================================
//  ПАЛИТРЫ БИОМОВ
// ============================================================
BiomePal biomePalette(Biome b)
{
    static BiomePal cache[Bio_Count];
    static bool ready[Bio_Count] = { false, false, false, false, false, false, false, false, false };
    int bi = int(b);
    if (bi < 0 || bi >= Bio_Count) bi = Bio_Ash;
    if (ready[bi]) return cache[bi];
    ready[bi] = true;
    BiomePal &p = cache[bi];
    p.biome = b;
    switch (b) {
    case Bio_Ash:
        p.floor1 = QColor(58, 50, 48); p.floor2 = QColor(46, 40, 40); p.floor3 = QColor(72, 62, 58);
        p.wall = QColor(38, 32, 32); p.wallTop = QColor(96, 82, 74); p.wallDark = QColor(20, 17, 18);
        p.accent = QColor(226, 138, 60); p.hazard = QColor(255, 120, 40);
        p.sky = QColor(24, 18, 22); p.fog = QColor(60, 46, 40, 90);
        p.props = QVector<PropKind>() << Pk_Rock << Pk_Bone << Pk_DeadTree << Pk_Pillar << Pk_Stump << Pk_Brazier;
        p.propDensity = 0.045f; p.darkness = 0.40f; p.lightColor = QColor(255, 190, 120);
        break;
    case Bio_Ember:
        p.floor1 = QColor(44, 52, 36); p.floor2 = QColor(34, 42, 30); p.floor3 = QColor(58, 66, 44);
        p.wall = QColor(30, 34, 24); p.wallTop = QColor(76, 92, 56); p.wallDark = QColor(16, 20, 14);
        p.accent = QColor(240, 150, 60); p.hazard = QColor(255, 110, 40);
        p.sky = QColor(18, 24, 18); p.fog = QColor(40, 60, 34, 80);
        p.props = QVector<PropKind>() << Pk_Tree << Pk_DeadTree << Pk_Bush << Pk_Mushroom << Pk_Rock << Pk_Stump;
        p.propDensity = 0.085f; p.darkness = 0.46f; p.lightColor = QColor(255, 210, 150);
        break;
    case Bio_Frost:
        p.floor1 = QColor(150, 172, 196); p.floor2 = QColor(124, 148, 176); p.floor3 = QColor(186, 204, 222);
        p.wall = QColor(96, 116, 146); p.wallTop = QColor(214, 230, 244); p.wallDark = QColor(40, 52, 72);
        p.accent = QColor(150, 220, 255); p.hazard = QColor(120, 200, 255);
        p.sky = QColor(20, 28, 40); p.fog = QColor(190, 220, 240, 70);
        p.props = QVector<PropKind>() << Pk_IceSpike << Pk_Rock << Pk_DeadTree << Pk_Crystal << Pk_Pillar;
        p.propDensity = 0.055f; p.darkness = 0.30f; p.lightColor = QColor(200, 230, 255);
        break;
    case Bio_Crystal:
        p.floor1 = QColor(46, 40, 66); p.floor2 = QColor(34, 30, 52); p.floor3 = QColor(62, 54, 88);
        p.wall = QColor(32, 26, 50); p.wallTop = QColor(120, 96, 190); p.wallDark = QColor(16, 13, 28);
        p.accent = QColor(176, 132, 255); p.hazard = QColor(190, 120, 255);
        p.sky = QColor(14, 12, 26); p.fog = QColor(90, 60, 150, 80);
        p.props = QVector<PropKind>() << Pk_Crystal << Pk_Rock << Pk_Obelisk << Pk_Pillar;
        p.propDensity = 0.050f; p.darkness = 0.55f; p.lightColor = QColor(190, 160, 255);
        break;
    case Bio_Swamp:
        p.floor1 = QColor(48, 54, 38); p.floor2 = QColor(38, 44, 32); p.floor3 = QColor(62, 68, 46);
        p.wall = QColor(30, 34, 26); p.wallTop = QColor(84, 96, 62); p.wallDark = QColor(14, 18, 14);
        p.accent = QColor(150, 220, 90); p.hazard = QColor(120, 200, 70);
        p.sky = QColor(16, 20, 16); p.fog = QColor(70, 100, 60, 100);
        p.props = QVector<PropKind>() << Pk_DeadTree << Pk_Mushroom << Pk_Bush << Pk_Bone << Pk_Stump << Pk_Rock;
        p.propDensity = 0.070f; p.hasHazard = true; p.darkness = 0.48f; p.lightColor = QColor(190, 230, 150);
        break;
    case Bio_Ruins:
        p.floor1 = QColor(96, 88, 74); p.floor2 = QColor(78, 72, 60); p.floor3 = QColor(116, 106, 88);
        p.wall = QColor(66, 60, 50); p.wallTop = QColor(150, 138, 116); p.wallDark = QColor(28, 25, 22);
        p.accent = QColor(220, 190, 120); p.hazard = QColor(110, 160, 170);
        p.sky = QColor(22, 20, 18); p.fog = QColor(80, 74, 60, 70);
        p.props = QVector<PropKind>() << Pk_Ruin << Pk_Pillar << Pk_Statue << Pk_Rock << Pk_Bone << Pk_Obelisk;
        p.propDensity = 0.050f; p.darkness = 0.36f; p.lightColor = QColor(255, 225, 170);
        break;
    case Bio_Lava:
        p.floor1 = QColor(52, 34, 32); p.floor2 = QColor(38, 25, 25); p.floor3 = QColor(70, 46, 40);
        p.wall = QColor(34, 22, 22); p.wallTop = QColor(120, 62, 40); p.wallDark = QColor(16, 10, 10);
        p.accent = QColor(255, 140, 40); p.hazard = QColor(255, 110, 20);
        p.sky = QColor(26, 12, 10); p.fog = QColor(120, 50, 20, 90);
        p.props = QVector<PropKind>() << Pk_Vent << Pk_Rock << Pk_Obelisk << Pk_Brazier << Pk_Pillar;
        p.propDensity = 0.042f; p.hasHazard = true; p.darkness = 0.44f; p.lightColor = QColor(255, 150, 60);
        break;
    case Bio_Void:
        p.floor1 = QColor(28, 22, 40); p.floor2 = QColor(20, 16, 30); p.floor3 = QColor(40, 32, 58);
        p.wall = QColor(16, 12, 26); p.wallTop = QColor(96, 62, 150); p.wallDark = QColor(8, 6, 14);
        p.accent = QColor(200, 140, 255); p.hazard = QColor(180, 110, 255);
        p.sky = QColor(8, 6, 14); p.fog = QColor(70, 40, 120, 100);
        p.props = QVector<PropKind>() << Pk_Obelisk << Pk_Crystal << Pk_Statue << Pk_Rock << Pk_Pillar;
        p.propDensity = 0.040f; p.darkness = 0.60f; p.lightColor = QColor(190, 150, 255);
        break;
    case Bio_Town:
    default:
        p.floor1 = QColor(88, 74, 58); p.floor2 = QColor(72, 60, 48); p.floor3 = QColor(106, 90, 70);
        p.wall = QColor(56, 46, 38); p.wallTop = QColor(140, 118, 92); p.wallDark = QColor(26, 22, 18);
        p.accent = QColor(242, 181, 68); p.hazard = QColor(255, 170, 60);
        p.sky = QColor(24, 20, 18); p.fog = QColor(60, 50, 40, 50);
        p.props = QVector<PropKind>() << Pk_House << Pk_Stall << Pk_Barrel << Pk_Crate << Pk_Brazier << Pk_Tree << Pk_Statue;
        p.propDensity = 0.030f; p.darkness = 0.22f; p.lightColor = QColor(255, 210, 140);
        break;
    }
    return p;
}

// ============================================================
//  ГЕНЕРАЦИЯ
// ============================================================
namespace {

void carveRoom(Dungeon &d, int x0, int y0, int x1, int y1, Rng &r, int wobble)
{
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            if (x < 1 || y < 1 || x >= d.w - 1 || y >= d.h - 1) continue;
            int wx = 0, wy = 0;
            if (wobble > 0) {
                wx = int(r.gauss() * wobble);
                wy = int(r.gauss() * wobble);
            }
            int xx = x + (x == x0 || x == x1 ? wx : 0);
            int yy = y + (y == y0 || y == y1 ? wy : 0);
            if (xx < 1 || yy < 1 || xx >= d.w - 1 || yy >= d.h - 1) continue;
            d.tiles[d.idx(xx, yy)] = T_Floor;
        }
    }
}

void carveCorridor(Dungeon &d, int x0, int y0, int x1, int y1, int width)
{
    int x = x0, y = y0;
    while (x != x1) {
        for (int k = -width / 2; k <= width / 2; ++k) {
            int yy = qBound(1, y + k, d.h - 2);
            d.tiles[d.idx(qBound(1, x, d.w - 2), yy)] = T_Floor;
        }
        x += (x1 > x) ? 1 : -1;
    }
    while (y != y1) {
        for (int k = -width / 2; k <= width / 2; ++k) {
            int xx = qBound(1, x + k, d.w - 2);
            d.tiles[d.idx(xx, qBound(1, y, d.h - 2))] = T_Floor;
        }
        y += (y1 > y) ? 1 : -1;
    }
}

} // anonymous

Dungeon DungeonGen::generate(int locationId, Biome biome, int tier, int kind, int seed)
{
    Dungeon d;
    Rng r(quint64(seed) * 6364136223846793005ull + 1442695040888963407ull + quint64(locationId) * 7919ull);
    d.biome = biome;
    d.seed = seed;
    d.tier = tier;
    d.kind = kind;
    d.tileSize = 40;

    if (kind == 1) { d.w = 42; d.h = 32; }
    else if (kind == 2) { d.w = 30; d.h = 24; }
    else { d.w = r.range(46, 58); d.h = r.range(34, 44); }

    d.tiles = QVector<quint8>(d.w * d.h, T_Wall);

    BiomePal pal = biomePalette(biome);

    if (kind == 1) {
        // ГОРОД: большая площадь + улицы
        carveRoom(d, 3, 3, d.w - 4, d.h - 4, r, 0);
        for (int x = 3; x < d.w - 3; ++x)
            for (int y = d.h / 2 - 1; y <= d.h / 2 + 1; ++y) d.tiles[d.idx(x, y)] = T_Road;
        for (int y = 3; y < d.h - 3; ++y)
            for (int x = d.w / 2 - 1; x <= d.w / 2 + 1; ++x) d.tiles[d.idx(x, y)] = T_Road;
    } else if (kind == 2) {
        // СВЯТИЛИЩЕ: круглая поляна
        float cx = d.w * 0.5f, cy = d.h * 0.5f, rad = qMin(d.w, d.h) * 0.36f;
        for (int y = 1; y < d.h - 1; ++y)
            for (int x = 1; x < d.w - 1; ++x) {
                float dd = float(qSqrt((x - cx) * (x - cx) + (y - cy) * (y - cy) * 1.0f));
                if (dd < rad + r.gauss() * 1.2f) d.tiles[d.idx(x, y)] = T_Floor;
            }
    } else {
        // БОЕВАЯ АРЕНА: основная комната + 1-3 ответвления
        int m = 4;
        carveRoom(d, m, m, d.w - m - 1, d.h - m - 1, r, 1);
        int chambers = (tier >= 3) ? r.range(1, 3) : r.range(0, 2);
        QVector<QPoint> centers;
        centers.append(QPoint(d.w / 2, d.h / 2));
        for (int i = 0; i < chambers; ++i) {
            int cw = r.range(9, 16), ch = r.range(8, 13);
            int cx = r.range(m + 2, d.w - m - cw - 2);
            int cy = r.range(m + 2, d.h - m - ch - 2);
            carveRoom(d, cx, cy, cx + cw, cy + ch, r, 2);
            centers.append(QPoint(cx + cw / 2, cy + ch / 2));
            carveCorridor(d, centers.first().x(), centers.first().y(), cx + cw / 2, cy + ch / 2, r.range(3, 5));
        }
        // колонны / препятствия
        int pillars = r.range(4, 10);
        for (int i = 0; i < pillars; ++i) {
            int px = r.range(6, d.w - 7), py = r.range(6, d.h - 7);
            int pw = r.range(1, 2), ph = r.range(1, 3);
            if (qAbs(px - d.w / 2) < 5 && qAbs(py - d.h / 2) < 5) continue;
            for (int y = py; y <= py + ph; ++y)
                for (int x = px; x <= px + pw; ++x) {
                    if (x < 2 || y < 2 || x >= d.w - 2 || y >= d.h - 2) continue;
                    if (d.tiles[d.idx(x, y)] == T_Floor) d.tiles[d.idx(x, y)] = T_Wall;
                }
        }
        // опасные зоны
        if (pal.hasHazard || biome == Bio_Lava || biome == Bio_Swamp) {
            int n = r.range(3, 7);
            for (int i = 0; i < n; ++i) {
                int px = r.range(5, d.w - 6), py = r.range(5, d.h - 6);
                int rad = r.range(1, 3);
                for (int y = py - rad; y <= py + rad; ++y)
                    for (int x = px - rad; x <= px + rad; ++x) {
                        if (x < 2 || y < 2 || x >= d.w - 2 || y >= d.h - 2) continue;
                        float dd = float(qSqrt(float((x - px) * (x - px) + (y - py) * (y - py))));
                        if (dd <= rad + r.gauss() * 0.6f && d.tiles[d.idx(x, y)] == T_Floor)
                            d.tiles[d.idx(x, y)] = T_Hazard;
                    }
            }
        }
        // лужи/вода в руинах и топях
        if (biome == Bio_Swamp || biome == Bio_Ruins) {
            int n = r.range(2, 5);
            for (int i = 0; i < n; ++i) {
                int px = r.range(5, d.w - 6), py = r.range(5, d.h - 6);
                int rad = r.range(2, 4);
                for (int y = py - rad; y <= py + rad; ++y)
                    for (int x = px - rad; x <= px + rad; ++x) {
                        if (x < 2 || y < 2 || x >= d.w - 2 || y >= d.h - 2) continue;
                        float dd = float(qSqrt(float((x - px) * (x - px) + (y - py) * (y - py))));
                        if (dd <= rad + r.gauss() * 0.8f && d.tiles[d.idx(x, y)] == T_Floor)
                            d.tiles[d.idx(x, y)] = T_Water;
                    }
            }
        }
    }

    // ---- точка появления игрока и выход ----
    auto findFloorNear = [&](int tx, int ty) -> QPointF {
        for (int rad = 0; rad < qMax(d.w, d.h); ++rad) {
            for (int y = ty - rad; y <= ty + rad; ++y)
                for (int x = tx - rad; x <= tx + rad; ++x) {
                    if (x < 1 || y < 1 || x >= d.w - 1 || y >= d.h - 1) continue;
                    quint8 t = d.tiles[d.idx(x, y)];
                    if (t == T_Floor || t == T_Road)
                        return QPointF((x + 0.5f) * d.tileSize, (y + 0.5f) * d.tileSize);
                }
        }
        return QPointF(d.w * 0.5f * d.tileSize, d.h * 0.5f * d.tileSize);
    };
    d.playerSpawn = findFloorNear(4, d.h - 5);
    d.exitPos = findFloorNear(d.w - 5, 4);

    // ---- точки появления врагов ----
    QVector<QPointF> floors;
    for (int y = 1; y < d.h - 1; ++y)
        for (int x = 1; x < d.w - 1; ++x) {
            quint8 t = d.tiles[d.idx(x, y)];
            if (t == T_Floor || t == T_Road)
                floors.append(QPointF((x + 0.5f) * d.tileSize, (y + 0.5f) * d.tileSize));
        }
    if (floors.isEmpty()) floors.append(d.playerSpawn);

    int want = 26;
    for (int i = 0; i < want * 6 && int(d.spawnPoints.size()) < want; ++i) {
        QPointF p = floors.at(r.pick(floors.size()));
        float dx = p.x() - d.playerSpawn.x(), dy = p.y() - d.playerSpawn.y();
        if (qSqrt(dx * dx + dy * dy) < 300.f) continue;
        bool ok = true;
        for (int k = 0; k < d.spawnPoints.size(); ++k) {
            QPointF q = d.spawnPoints.at(k);
            float ddx = p.x() - q.x(), ddy = p.y() - q.y();
            if (qSqrt(ddx * ddx + ddy * ddy) < 90.f) { ok = false; break; }
        }
        if (ok) d.spawnPoints.append(p);
    }
    while (d.spawnPoints.size() < 8) d.spawnPoints.append(floors.at(r.pick(floors.size())));

    // ---- NPC (города) ----
    if (kind == 1) {
        d.npcPoints.append(QPointF(d.w * 0.5f * d.tileSize, (d.h * 0.5f - 3.0f) * d.tileSize));
        d.npcPoints.append(QPointF((d.w * 0.5f - 6) * d.tileSize, (d.h * 0.5f + 3.0f) * d.tileSize));
        d.npcPoints.append(QPointF((d.w * 0.5f + 6) * d.tileSize, (d.h * 0.5f + 3.0f) * d.tileSize));
        d.npcPoints.append(QPointF((d.w * 0.5f + 10) * d.tileSize, (d.h * 0.5f - 3.0f) * d.tileSize)); // старейшина
    }

    // ---- ПРОПСЫ ----
    QVector<QPointF> occupied;
    occupied.append(d.playerSpawn);
    occupied.append(d.exitPos);
    for (int i = 0; i < d.spawnPoints.size(); ++i) occupied.append(d.spawnPoints.at(i));
    for (int i = 0; i < d.npcPoints.size(); ++i) occupied.append(d.npcPoints.at(i));

    float dens = pal.propDensity;
    if (kind == 1) dens = 0.022f;
    if (kind == 2) dens = 0.030f;

    for (int y = 2; y < d.h - 2; ++y) {
        for (int x = 2; x < d.w - 2; ++x) {
            quint8 t = d.tiles[d.idx(x, y)];
            if (t != T_Floor && t != T_Road) continue;
            if (!r.chance(dens)) continue;
            QPointF pos((x + 0.5f + r.gauss() * 0.25f) * d.tileSize, (y + 0.5f + r.gauss() * 0.25f) * d.tileSize);
            bool blocked = false;
            for (int k = 0; k < occupied.size(); ++k) {
                QPointF q = occupied.at(k);
                float dx = pos.x() - q.x(), dy = pos.y() - q.y();
                float need = (k < 2) ? 130.f : 70.f;
                if (dx * dx + dy * dy < need * need) { blocked = true; break; }
            }
            if (blocked) continue;

            Prop pp;
            pp.kind = pal.props.at(r.pick(pal.props.size()));
            pp.pos = pos;
            pp.seed = int(r.next());
            pp.scale = r.frange(0.8f, 1.35f);
            switch (pp.kind) {
            case Pk_Tree:      pp.radius = 14 * pp.scale; pp.height = 96 * pp.scale; break;
            case Pk_DeadTree:  pp.radius = 10 * pp.scale; pp.height = 78 * pp.scale; break;
            case Pk_Rock:      pp.radius = 15 * pp.scale; pp.height = 26 * pp.scale; break;
            case Pk_Crystal:   pp.radius = 11 * pp.scale; pp.height = 46 * pp.scale; pp.lightRadius = 130; pp.lightColor = pal.accent; break;
            case Pk_Pillar:    pp.radius = 15 * pp.scale; pp.height = 92 * pp.scale; break;
            case Pk_Ruin:      pp.radius = 18 * pp.scale; pp.height = 40 * pp.scale; break;
            case Pk_Statue:    pp.radius = 14 * pp.scale; pp.height = 66 * pp.scale; break;
            case Pk_Obelisk:   pp.radius = 12 * pp.scale; pp.height = 84 * pp.scale; pp.lightRadius = 110; pp.lightColor = pal.accent; break;
            case Pk_IceSpike:  pp.radius = 12 * pp.scale; pp.height = 54 * pp.scale; break;
            case Pk_Brazier:   pp.radius = 11 * pp.scale; pp.height = 44 * pp.scale; pp.lightRadius = 210; pp.lightColor = QColor(255, 170, 80); break;
            case Pk_Vent:      pp.radius = 16 * pp.scale; pp.height = 10 * pp.scale; pp.lightRadius = 150; pp.lightColor = QColor(255, 120, 40); break;
            case Pk_House:     pp.radius = 40 * pp.scale; pp.height = 110 * pp.scale; break;
            case Pk_Stall:     pp.radius = 26 * pp.scale; pp.height = 46 * pp.scale; break;
            case Pk_Barrel:    pp.radius = 12 * pp.scale; pp.height = 26 * pp.scale; break;
            case Pk_Crate:     pp.radius = 13 * pp.scale; pp.height = 26 * pp.scale; break;
            case Pk_Altar:     pp.radius = 20 * pp.scale; pp.height = 34 * pp.scale; pp.lightRadius = 190; pp.lightColor = pal.accent; break;
            case Pk_Bush:      pp.radius = 9 * pp.scale;  pp.height = 22 * pp.scale; pp.solid = false; break;
            case Pk_Mushroom:  pp.radius = 6 * pp.scale;  pp.height = 16 * pp.scale; pp.solid = false; pp.lightRadius = 70; pp.lightColor = QColor(160, 240, 140); break;
            case Pk_Stump:     pp.radius = 13 * pp.scale; pp.height = 20 * pp.scale; break;
            case Pk_Bone:      pp.radius = 8 * pp.scale;  pp.height = 12 * pp.scale; pp.solid = false; break;
            default:           pp.radius = 12; pp.height = 24; break;
            }
            d.props.append(pp);
        }
    }

    // сундуки с сокровищами в боевых локациях
    if (kind == 0 && r.chance(0.9f)) {
        int n = r.range(1, 2);
        for (int i = 0; i < n; ++i) {
            QPointF cp = d.randomFloorFarFrom(d.playerSpawn, 320.f);
            Prop ch;
            ch.kind = Pk_Chest;
            ch.pos = cp;
            ch.radius = 15;
            ch.height = 26;
            ch.solid = true;
            ch.seed = int(r.next());
            ch.scale = r.frange(0.9f, 1.15f);
            d.props.append(ch);
        }
    }

    // алтарь в святилище
    if (kind == 2) {
        Prop p;
        p.kind = Pk_Altar;
        p.pos = QPointF(d.w * 0.5f * d.tileSize, d.h * 0.44f * d.tileSize);
        p.radius = 22; p.height = 40; p.scale = 1.4f;
        p.lightRadius = 260; p.lightColor = pal.accent;
        d.props.append(p);
        d.exitPos = QPointF(d.w * 0.5f * d.tileSize, (d.h - 3.5f) * d.tileSize);
    }

    d.buildGround();
    return d;
}

// ============================================================
//  ГЕОМЕТРИЯ
// ============================================================
QPointF Dungeon::randomFloor() const
{
    Rng r(quint64(seed) ^ 0xBEEF);
    for (int i = 0; i < 400; ++i) {
        int x = r.range(1, w - 2), y = r.range(1, h - 2);
        quint8 t = tiles.at(idx(x, y));
        if (t == T_Floor || t == T_Road)
            return QPointF((x + 0.5f) * tileSize, (y + 0.5f) * tileSize);
    }
    return playerSpawn;
}

QPointF Dungeon::randomFloorFarFrom(const QPointF &p, float minDist) const
{
    Rng r(quint64(seed) ^ quint64(int(p.x() * 13 + p.y() * 7)) ^ 0xC0FFEE);
    QPointF best = p;
    float bestD = -1;
    for (int i = 0; i < 300; ++i) {
        int x = r.range(1, w - 2), y = r.range(1, h - 2);
        quint8 t = tiles.at(idx(x, y));
        if (t != T_Floor && t != T_Road) continue;
        QPointF q((x + 0.5f) * tileSize, (y + 0.5f) * tileSize);
        float dx = q.x() - p.x(), dy = q.y() - p.y();
        float dd = qSqrt(dx * dx + dy * dy);
        if (dd > bestD) { bestD = dd; best = q; }
        if (dd >= minDist) return q;
    }
    return best;
}

QPointF Dungeon::nearestFree(const QPointF &p, float maxR) const
{
    if (!solidAtPx(p)) return p;
    for (float r = 10.f; r < maxR; r += 10.f) {
        for (int i = 0; i < 10; ++i) {
            float a = float(i) * 0.6283f + r * 0.05f;
            QPointF q = p + QPointF(qCos(a) * r, qSin(a) * r);
            if (!solidAtPx(q)) return q;
        }
    }
    return playerSpawn;
}

void Dungeon::collideCircle(QPointF &pos, float radius) const
{
    if (!valid()) return;
    int tx0 = int((pos.x() - radius) / tileSize), tx1 = int((pos.x() + radius) / tileSize);
    int ty0 = int((pos.y() - radius) / tileSize), ty1 = int((pos.y() + radius) / tileSize);
    for (int ty = ty0; ty <= ty1; ++ty) {
        for (int tx = tx0; tx <= tx1; ++tx) {
            if (tx < 0 || ty < 0 || tx >= w || ty >= h) {
                pos.setX(qBound(float(radius), float(pos.x()), float(w * tileSize - radius)));
                pos.setY(qBound(float(radius), float(pos.y()), float(h * tileSize - radius)));
                continue;
            }
            if (tiles.at(idx(tx, ty)) != T_Wall) continue;
            QRectF rct(tx * tileSize, ty * tileSize, tileSize, tileSize);
            QPointF c(qBound(rct.left(), pos.x(), rct.right()), qBound(rct.top(), pos.y(), rct.bottom()));
            float dx = float(pos.x() - c.x()), dy = float(pos.y() - c.y());
            float dist2 = dx * dx + dy * dy;
            if (dist2 >= radius * radius) continue;
            float dist = qSqrt(dist2);
            if (dist < 0.0001f) {
                // центр внутри тайла - выталкиваем к ближайшей грани
                float left = pos.x() - rct.left(), right = rct.right() - pos.x();
                float top = pos.y() - rct.top(), bottom = rct.bottom() - pos.y();
                float mn = qMin(qMin(left, right), qMin(top, bottom));
                if (mn == left) pos.setX(rct.left() - radius);
                else if (mn == right) pos.setX(rct.right() + radius);
                else if (mn == top) pos.setY(rct.top() - radius);
                else pos.setY(rct.bottom() + radius);
            } else {
                float push = radius - dist;
                pos.setX(pos.x() + dx / dist * push);
                pos.setY(pos.y() + dy / dist * push);
            }
        }
    }
}

QVector<const Prop *> Dungeon::solidPropsNear(const QPointF &p, float r) const
{
    QVector<const Prop *> out;
    for (int i = 0; i < props.size(); ++i) {
        const Prop &pr = props.at(i);
        if (!pr.solid) continue;
        float dx = pr.pos.x() - p.x(), dy = pr.pos.y() - p.y();
        float rr = r + pr.radius + 4.f;
        if (dx * dx + dy * dy < rr * rr) out.append(&props.at(i));
    }
    return out;
}

// ============================================================
//  РЕНДЕР ЗЕМЛИ В КЭШ
// ============================================================

// Устойчивый хэш координат тайла: вариант спрайта не должен «плыть»
// от потока Rng, иначе запеканка менялась бы при любом чихе генератора.
static inline int cellHash(int x, int y, int seed, quint32 salt = 0u)
{
    quint32 hh = quint32(x) * 374761393u + quint32(y) * 668265263u
               + quint32(seed) * 2246822519u + salt;
    hh = (hh ^ (hh >> 13)) * 1274126177u;
    return int((hh ^ (hh >> 16)) & 0x7fffffffu);
}

// Штамповка ячейки тайлсета в тайл карты: 1:1 без фильтрации, если размеры
// совпадают (наш встроенный набор = tileSize), иначе плавное масштабирование.
static void stampTile(QPainter &p, const TileBank::Tileset &ts, const QRect &cr, const QRectF &rc)
{
    if (!cr.isValid()) return;
    if (cr.width() == int(rc.width()) && cr.height() == int(rc.height()))
        p.drawImage(rc.topLeft(), ts.img, cr);
    else
        p.drawImage(rc, ts.img, QRectF(cr));
}

void Dungeon::buildGround()
{
    if (!valid()) return;
    BiomePal pal = biomePalette(biome);
    QImage img(w * tileSize, h * tileSize, QImage::Format_RGB32);
    img.fill(pal.wallDark.rgb());
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, false);
    Rng r(quint64(seed) * 2862933555777941757ull + 3037000493ull);

    const int T = tileSize;

    // Спрайтовый тайлсет биома (если загружен и спрайты включены):
    // тайлы штамповаются в запеканку вместо процедурной отрисовки.
    const TileBank::Tileset *ts = TileBank::get(biome);
    const int nFloor  = ts ? ts->count(QStringLiteral("floor"))   : 0;
    const int nRoad   = ts ? ts->count(QStringLiteral("road"))   : 0;
    const int nWall   = ts ? ts->count(QStringLiteral("wall"))   : 0;
    const int nTop    = ts ? ts->count(QStringLiteral("walltop")) : 0;
    const int nHazard = ts ? ts->count(QStringLiteral("hazard")) : 0;
    const int nWater  = ts ? ts->count(QStringLiteral("water"))  : 0;
    const int nDeco   = ts ? ts->count(QStringLiteral("deco"))   : 0;

    // --- базовые тайлы ---
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            quint8 t = tiles.at(idx(x, y));
            QRectF rc(x * T, y * T, T, T);
            float n = r.unit();
            // спрайтовый тайл по типу; вариант - хэш координат (без rng).
            // Если ячеек этого типа в наборе нет - тайл рисуется процедурно
            // целиком (важно для воды/опасности: их нельзя заменять полом).
            if (ts) {
                QRect cr;
                if (t == T_Wall && nWall > 0)
                    cr = ts->rect(QStringLiteral("wall%1").arg(cellHash(x, y, seed) % nWall));
                else if (t == T_Road && nRoad > 0)
                    cr = ts->rect(QStringLiteral("road%1").arg(cellHash(x, y, seed) % nRoad));
                else if (t == T_Hazard && nHazard > 0)
                    cr = ts->rect(QStringLiteral("hazard%1").arg(cellHash(x, y, seed) % nHazard));
                else if (t == T_Water && nWater > 0)
                    cr = ts->rect(QStringLiteral("water%1").arg(cellHash(x, y, seed) % nWater));
                else if (t == T_Floor && nFloor > 0)
                    cr = ts->rect(QStringLiteral("floor%1").arg(cellHash(x, y, seed) % nFloor));
                if (cr.isValid()) { stampTile(p, *ts, cr, rc); continue; }
            }
            if (t == T_Wall) {
                QColor c = pal.wall.darker(120 + int(n * 30));
                p.fillRect(rc, c);
                // «каменная кладка»
                int rows = 2;
                for (int i = 0; i < rows; ++i) {
                    float yy = rc.top() + rc.height() / rows * i;
                    p.setPen(QPen(pal.wallDark, 1));
                    p.drawLine(QPointF(rc.left(), yy), QPointF(rc.right(), yy));
                    float xx = rc.left() + ((i + x + y) % 2 ? rc.width() * 0.5f : rc.width() * 0.25f);
                    p.drawLine(QPointF(xx, yy), QPointF(xx, yy + rc.height() / rows));
                }
                continue;
            }
            // почти однородный пол с мягким шумом - без «шахматной» клетки
            float mix = n;
            QColor base;
            {
                int rr = int(pal.floor2.red()   + (pal.floor1.red()   - pal.floor2.red())   * mix * 0.5f + r.gauss() * 3.5f);
                int gg = int(pal.floor2.green() + (pal.floor1.green() - pal.floor2.green()) * mix * 0.5f + r.gauss() * 3.5f);
                int bb = int(pal.floor2.blue()  + (pal.floor1.blue()  - pal.floor2.blue())  * mix * 0.5f + r.gauss() * 3.5f);
                base = QColor(qBound(0, rr, 255), qBound(0, gg, 255), qBound(0, bb, 255));
            }
            if (t == T_Road) base = base.darker(112);
            p.fillRect(rc, base);
            // мягкие пятна поверх, чтобы сгладить стыки
            p.setPen(Qt::NoPen);
            for (int q = 0; q < 2; ++q) {
                QColor bl = base.lighter(106 + int(r.unit() * 8));
                bl.setAlpha(26);
                p.setBrush(bl);
                p.drawEllipse(QPointF(rc.left() + r.unit() * T, rc.top() + r.unit() * T),
                              T * (0.22f + r.unit() * 0.30f), T * (0.16f + r.unit() * 0.22f));
            }

            if (t == T_Hazard) {
                QColor hz = pal.hazard;
                QRadialGradient g(rc.center(), rc.width() * 0.7f);
                g.setColorAt(0, hz.lighter(150));
                g.setColorAt(1, hz.darker(220));
                p.fillRect(rc, base.darker(150));
                p.fillRect(rc, QBrush(g));
            } else if (t == T_Water) {
                QColor wt = QColor(38, 66, 78);
                p.fillRect(rc, wt);
                p.setPen(QPen(wt.lighter(140), 1));
                for (int i = 0; i < 2; ++i) {
                    float yy = rc.top() + r.unit() * rc.height();
                    p.drawLine(QPointF(rc.left() + 4, yy), QPointF(rc.right() - 4, yy));
                }
            }

            // едва заметный шов плитки
            p.setPen(QPen(QColor(0, 0, 0, 12), 1));
            p.drawRect(rc.adjusted(0.5, 0.5, -0.5, -0.5));
        }
    }

    // --- детали биомов на полу ---
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            quint8 t = tiles.at(idx(x, y));
            if (t == T_Wall) continue;
            QRectF rc(x * T, y * T, T, T);
            float n = r.unit();
            // спрайтовые детали пола из атласа (трава, трещины, камешки...)
            if (ts && nDeco > 0 && t == T_Floor && n < 0.22f) {
                QRect cr = ts->rect(QStringLiteral("deco%1").arg(cellHash(x, y, seed, 0x5bf03635u) % nDeco));
                if (cr.isValid()) { stampTile(p, *ts, cr, rc); continue; }
            }
            p.setRenderHint(QPainter::Antialiasing, true);
            switch (biome) {
            case Bio_Ash:
                if (n < 0.16f) { // трещины
                    p.setPen(QPen(QColor(20, 16, 16, 150), 1.4f));
                    QPointF a(rc.left() + r.unit() * T, rc.top() + r.unit() * T);
                    QPointF b(rc.left() + r.unit() * T, rc.top() + r.unit() * T);
                    p.drawLine(a, b);
                } else if (n < 0.30f) { // пепел
                    p.setPen(Qt::NoPen);
                    p.setBrush(QColor(190, 180, 172, 26));
                    p.drawEllipse(rc.center(), T * 0.36f, T * 0.28f);
                } else if (n < 0.33f) { // камешки
                    p.setBrush(QColor(30, 26, 26, 160));
                    p.setPen(Qt::NoPen);
                    for (int i = 0; i < 3; ++i)
                        p.drawEllipse(QPointF(rc.left() + r.unit() * T, rc.top() + r.unit() * T), 2.2f, 1.8f);
                }
                break;
            case Bio_Ember:
                if (n < 0.30f && t != T_Hazard) { // трава
                    p.setPen(QPen(QColor(96, 120, 62, 190), 1.4f, Qt::SolidLine, Qt::RoundCap));
                    for (int i = 0; i < 3; ++i) {
                        float bx = rc.left() + r.unit() * T, by = rc.bottom() - r.unit() * T * 0.4f;
                        p.drawLine(QPointF(bx, by), QPointF(bx + r.gauss() * 4, by - 6 - r.unit() * 6));
                    }
                } else if (n < 0.36f) { // корни
                    p.setPen(QPen(QColor(58, 44, 30, 170), 2.4f, Qt::SolidLine, Qt::RoundCap));
                    p.drawLine(rc.topLeft() + QPointF(4, r.unit() * T), rc.bottomRight() - QPointF(4, r.unit() * T));
                }
                break;
            case Bio_Frost:
                if (n < 0.28f) { // снег
                    p.setPen(Qt::NoPen);
                    p.setBrush(QColor(240, 248, 255, 44));
                    p.drawEllipse(rc.center() + QPointF(r.gauss() * 8, r.gauss() * 8), T * 0.42f, T * 0.30f);
                } else if (n < 0.34f) { // ледяные трещины
                    p.setPen(QPen(QColor(200, 235, 255, 110), 1.2f));
                    p.drawLine(rc.left() + r.unit() * T, rc.top() + r.unit() * T, rc.left() + r.unit() * T, rc.top() + r.unit() * T);
                }
                break;
            case Bio_Crystal:
                if (n < 0.12f) { // жилы
                    p.setPen(QPen(QColor(176, 132, 255, 120), 1.8f, Qt::SolidLine, Qt::RoundCap));
                    QPointF a(rc.left(), rc.top() + r.unit() * T);
                    QPointF b(rc.right(), rc.top() + r.unit() * T);
                    p.drawLine(a, b);
                } else if (n < 0.20f) { // мелкие кристаллы
                    p.setPen(Qt::NoPen);
                    p.setBrush(QColor(190, 150, 255, 90));
                    QPolygonF pg;
                    pg << QPointF(rc.center().x(), rc.top() + 8)
                       << QPointF(rc.center().x() + 6, rc.center().y())
                       << QPointF(rc.center().x(), rc.bottom() - 8)
                       << QPointF(rc.center().x() - 6, rc.center().y());
                    p.drawPolygon(pg);
                }
                break;
            case Bio_Swamp:
                if (n < 0.22f && t != T_Water) { // мох
                    p.setPen(Qt::NoPen);
                    p.setBrush(QColor(70, 96, 44, 60));
                    p.drawEllipse(rc.center() + QPointF(r.gauss() * 10, r.gauss() * 10), T * 0.45f, T * 0.34f);
                } else if (n < 0.26f) { // пузыри
                    p.setPen(QPen(QColor(160, 210, 120, 90), 1.0f));
                    p.setBrush(Qt::NoBrush);
                    p.drawEllipse(rc.center(), 3.0f, 3.0f);
                }
                break;
            case Bio_Ruins:
                if (n < 0.34f) { // плиты
                    p.setPen(QPen(QColor(50, 44, 36, 110), 1.2f));
                    p.drawRect(rc.adjusted(3, 3, -3, -3));
                } else if (n < 0.40f) { // песок
                    p.setPen(Qt::NoPen);
                    p.setBrush(QColor(196, 176, 132, 40));
                    p.drawEllipse(rc.center(), T * 0.40f, T * 0.26f);
                }
                break;
            case Bio_Lava:
                if (n < 0.24f) { // раскалённые трещины
                    p.setPen(QPen(QColor(255, 130, 40, 150), 1.8f, Qt::SolidLine, Qt::RoundCap));
                    QPointF a(rc.left() + r.unit() * T * 0.3f, rc.top() + r.unit() * T);
                    QPointF b(rc.left() + r.unit() * T, rc.bottom() - r.unit() * T * 0.3f);
                    p.drawLine(a, b);
                } else if (n < 0.30f) { // обсидиан
                    p.setPen(Qt::NoPen);
                    p.setBrush(QColor(16, 10, 12, 120));
                    p.drawEllipse(rc.center(), T * 0.36f, T * 0.28f);
                }
                break;
            case Bio_Void:
                if (n < 0.14f) { // звёзды
                    p.setPen(Qt::NoPen);
                    p.setBrush(QColor(220, 190, 255, int(60 + r.unit() * 120)));
                    p.drawEllipse(rc.center() + QPointF(r.gauss() * 12, r.gauss() * 12), 1.4f, 1.4f);
                } else if (n < 0.19f) { // руны
                    p.setPen(QPen(QColor(180, 120, 255, 90), 1.2f));
                    p.setBrush(Qt::NoBrush);
                    p.drawEllipse(rc.center(), T * 0.24f, T * 0.24f);
                }
                break;
            case Bio_Town:
            default:
                if (n < 0.30f) { // доски/плитка
                    p.setPen(QPen(QColor(46, 38, 30, 120), 1.2f));
                    p.drawLine(rc.left(), rc.center().y(), rc.right(), rc.center().y());
                }
                break;
            }
            p.setRenderHint(QPainter::Antialiasing, false);
        }
    }

    // --- запечённый ambient occlusion: пол темнеет у стен ---
    p.setRenderHint(QPainter::Antialiasing, true);
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            if (tiles.at(idx(x, y)) == T_Wall) continue;
            // расстояние до ближайшей стены в кольце 1..2
            float near = 9.f;
            for (int dy = -2; dy <= 2; ++dy) {
                for (int dx = -2; dx <= 2; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    if (tileAt(x + dx, y + dy) == T_Wall) {
                        float d = qSqrt(float(dx * dx + dy * dy));
                        if (d < near) near = d;
                    }
                }
            }
            if (near > 2.6f) continue;
            float a = (1.f - near / 2.6f);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0, 0, 0, int(64 * a * a)));
            p.drawEllipse(QPointF((x + 0.5f) * T, (y + 0.5f) * T), T * 0.62f, T * 0.62f);
        }
    }
    // мягкие тени под крупными пропами запекаем в землю
    for (int i = 0; i < props.size(); ++i) {
        const Prop &pr = props.at(i);
        if (!pr.solid) continue;
        QRadialGradient g(pr.pos + QPointF(4, 5), pr.radius * 2.2f);
        g.setColorAt(0, QColor(0, 0, 0, 90));
        g.setColorAt(1, QColor(0, 0, 0, 0));
        p.setBrush(g);
        p.setPen(Qt::NoPen);
        p.save();
        p.translate(pr.pos + QPointF(4, 5));
        p.scale(1.0, 0.5);
        p.translate(-(pr.pos.x() + 4), -(pr.pos.y() + 5));
        p.drawEllipse(pr.pos + QPointF(4, 5), pr.radius * 2.2f, pr.radius * 2.2f);
        p.restore();
    }
    p.setRenderHint(QPainter::Antialiasing, false);

    // --- «объём» стен: верхняя грань ---
    p.setRenderHint(QPainter::Antialiasing, false);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (tiles.at(idx(x, y)) != T_Wall) continue;
            bool belowFloor = (y + 1 < h) && tiles.at(idx(x, y + 1)) != T_Wall;
            QRectF rc(x * T, y * T, T, T);
            if (y > 0 && tiles.at(idx(x, y - 1)) != T_Wall) {
                // верхняя плоскость стены: спрайт walltop, иначе процедурно
                if (ts && nTop > 0) {
                    QRect cr = ts->rect(QStringLiteral("walltop%1").arg(cellHash(x, y, seed, 0x1f123bb5u) % nTop));
                    if (cr.isValid()) { stampTile(p, *ts, cr, rc); continue; }
                }
                p.fillRect(rc, pal.wallTop.darker(140));
                p.fillRect(rc.adjusted(0, 0, 0, -T * 0.55f), pal.wallTop.darker(125));
                p.setPen(QPen(pal.wallDark, 1));
                p.drawLine(rc.bottomLeft(), rc.bottomRight());
                p.drawLine(rc.topLeft(), rc.topRight());
            } else if (belowFloor) {
                // лицевая грань в тени: поверх спрайтовой кладки - полупрозрачная
                // wash-тень (текстура кирпичей проглядывает), поверх процедурной -
                // плоский тёмный цвет, как раньше
                if (ts && nWall > 0)
                    p.fillRect(rc.adjusted(0, 0, 0, -6), QColor(0, 0, 0, 105));
                else
                    p.fillRect(rc.adjusted(0, 0, 0, -6), pal.wall.darker(165));
            }
        }
    }

    // --- лёгкое освещение/виньетка карты ---
    QRadialGradient vg(img.rect().center(), qMax(img.width(), img.height()) * 0.75f);
    vg.setColorAt(0.35, QColor(255, 255, 255, 12));
    vg.setColorAt(1.0, QColor(0, 0, 0, 90));
    p.fillRect(img.rect(), QBrush(vg));
    p.end();

    ground = img;
}

} // namespace aa
