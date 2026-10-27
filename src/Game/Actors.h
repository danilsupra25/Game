#ifndef AA_ACTORS_H
#define AA_ACTORS_H

#include <QPointF>
#include <QVector>
#include <QColor>
#include <QString>
#include "Core/Types.h"
#include "World/Dungeon.h"

namespace aa {

// Эффекты/статусы на существе
struct StatusFx
{
    float burn = 0.f, burnDps = 0.f;
    float chill = 0.f;              // замедление (0..0.6)
    float shock = 0.f;              // +получаемый урон
    float poison = 0.f, poisonDps = 0.f;
    float mark = 0.f;               // метка смерти
    float stun = 0.f;
    float shield = 0.f;
    float haste = 0.f;              // бафф скорости атаки
    float bloodlust = 0.f;
    float heal = 0.f, healDps = 0.f;
    float rage = 0.f;               // заряды «Жатвы пепла»

    void tick(float dt)
    {
        burn = qMax(0.f, burn - dt);
        chill = qMax(0.f, chill - dt);
        shock = qMax(0.f, shock - dt);
        poison = qMax(0.f, poison - dt);
        mark = qMax(0.f, mark - dt);
        stun = qMax(0.f, stun - dt);
        shield = qMax(0.f, shield - dt);
        haste = qMax(0.f, haste - dt);
        bloodlust = qMax(0.f, bloodlust - dt);
        heal = qMax(0.f, heal - dt);
    }
    bool any() const
    {
        return burn > 0 || chill > 0 || shock > 0 || poison > 0 || mark > 0 || stun > 0
                || shield > 0 || haste > 0 || bloodlust > 0 || heal > 0;
    }
};

struct Actor
{
    int id = -1;
    int defId = -1;
    bool isPlayer = false;
    bool isSummon = false;

    QPointF pos;
    QPointF prevPos;      // для интерполяции рендера
    QPointF vel;
    float facing = 0.f;
    float radius = 14.f;

    float hp = 1.f, maxHp = 1.f;
    float dmgMin = 1.f, dmgMax = 2.f;
    DmgType dmgType = Dmg_Physical;
    float armor = 0.f;
    float speed = 100.f;
    float atkRange = 26.f;
    float atkCd = 1.2f;
    float atkTimer = 0.f;
    int   level = 1;

    int   xp = 0;
    int   goldMin = 0, goldMax = 0;
    float dropChance = 0.1f;

    int   ai = Ai_Chase;
    float detect = 420.f;
    int   targetId = -1;
    int   aiState = 0;
    float aiTimer = 0.f;
    float aiParam = 0.f;
    QPointF aiPoint;
    float strafeDir = 1.f;

    StatusFx st;
    bool dead = false;
    float deathT = 0.f;
    float hitFlash = 0.f;
    float animT = 0.f;
    float attackAnim = 0.f;
    int   attackKind = 0;   // 0 ближняя, 1 умение-магия, 2 выстрел, 3 умение-физическое
    int   attackSeq = 0;    // счётчик ближних атак: чередование attack/attack2
    float invuln = 0.f;
    float rollT = 0.f;
    float spawnT = 1.f;     // появление: 0..1
    float windup = 0.f;     // замах/изготовка перед атакой 0..1
    float rimAng = 0.f;     // направление на основной источник света (рим-подсветка)
    float hitTilt = 0.f;    // флинч от удара
    float tiltDir = 0.f;    // направление источника удара
    float rollDir = 0.f;
    float knockX = 0.f, knockY = 0.f;

    bool elite = false;
    QVector<int> eliteAffixes;
    float eliteAuraT = 0.f;

    bool boss = false;
    int  phase = 1;
    int  patternIdx = 0;
    float patternT = 0.f;
    float scale = 1.f;
    float baseRadius = 14.f;   // проектный радиус тела (для визуала)
    float drawScale = 1.f;     // множитель визуального размера
    QString bossTitle;

    bool flying = false;
    bool explodes = false;
    float explodeRadius = 0.f;
    bool summoner = false;
    int  summonId = -1;
    int  summonCount = 1;
    float summonTimer = 0.f;
    float projSpeed = 300.f;
    int  patternSet = 0;

    int   owner = -1;
    float lifeTime = -1.f;

    int tint = 0;
    int glyph = 0;
    int lastHitType = 0;
    QString nameOverride;
};

// Короткоживущий эффект атаки: взмах клинка, когти, ударная волна, колдовской круг.
// Рисуется спрайт-листом fx_* (если есть) или процедурно.
struct AttackFx
{
    QPointF pos;
    float ang = 0.f;        // направление удара, радианы
    float t = 0.f;          // прожитое время
    float dur = 0.26f;      // полная длительность
    int kind = 0;           // 0 взмах, 1 широкий взмах, 2 когти, 3 ударная волна, 4 выпад, 5 колдовской круг
    QColor color;
    float scale = 60.f;     // размер в мировых px
    int seed = 0;
    bool flip = false;      // зеркалить (чередование ударов)
};

struct Proj
{
    int id = -1;
    QPointF pos, prevPos, vel;
    float dmg = 1.f;
    DmgType type = Dmg_Physical;
    float radius = 6.f;
    int owner = -1;
    bool hostile = true;
    float life = 2.f;
    int kind = 0;             // визуал: 0 стрела,1 огненный шар,2 лёд,3 молния,4 яд,5 тайный,6 камень
    int pierce = 0;
    int chains = 0;
    QColor color;
    float scale = 1.f;
    float baseRadius = 14.f;   // проектный радиус тела (для визуала)
    float drawScale = 1.f;     // множитель визуального размера
    QVector<int> hit;
    float slow = 0.f;
    float burnDps = 0.f;
    float dotDps = 0.f;
    bool crit = false;
    float spin = 0.f;
    bool explodeOnHit = false;
    float explodeRadius = 0.f;
    int glyph = 0;
    QPointF startPos;
};

struct Part
{
    QPointF pos, vel;
    float life = 1.f, maxLife = 1.f;
    float size = 3.f;
    QColor c;
    int kind = 0;             // 0 точка,1 искра,2 дым,3 осколок,4 капля,5 руна
    float rot = 0.f, spin = 0.f;
    float grav = 0.f;
    bool additive = true;
};

struct FText
{
    QPointF pos;
    float life = 1.f, maxLife = 1.f;
    QString text;
    QColor c;
    float size = 14.f;
    bool crit = false;
    float vx = 0.f, vy = -40.f;
    int kind = 0;             // 0 урон,1 крит,2 лечение,3 текст,4 опыт
};

struct Zone
{
    QPointF pos;
    float radius = 60.f;
    float life = 3.f, maxLife = 3.f;
    float tickT = 0.f, tickRate = 0.5f;
    float dps = 10.f;
    DmgType type = Dmg_Fire;
    int owner = -1;
    bool hostile = true;
    int kind = 0;             // 0 огонь,1 яд,2 лёд,3 тайна,4 свет
    QColor c;
    float slow = 0.f;
    float delay = 0.f;      // задержка до активации (для «Землетрясения»)
    int follow = -1;        // если >=0 - зона привязана к актёру с этим id
    float followOffsetY = 0.f;
};

struct Telegraph
{
    QPointF pos;
    QPointF pos2;
    float radius = 60.f;
    float t = 0.f, dur = 1.f;
    int kind = 0;             // 0 круг,1 линия,2 конус
    QColor c;
    float dmg = 0.f;
    DmgType type = Dmg_Physical;
    int owner = -1;
    bool done = false;
    float width = 30.f;
};

struct Pickup
{
    QPointF pos;
    Item item;
    int kind = 0;             // 0 предмет,1 золото,2 зелье
    float bob = 0.f;
    float age = 0.f;        // для поп-ина
    QPointF vel;            // разлёт монет
    int gold = 0;
    float life = 0.f;         // 0 = вечно
    bool taken = false;
};

struct Npc
{
    int id = -1;
    QPointF pos;
    int kind = 0;             // 0 торговец,1 кузнец,2 целитель,3 старейшина
    QString name;
    QString line;
    float animT = 0.f;
    int tint = 0;
};

} // namespace aa

#endif // AA_ACTORS_H
