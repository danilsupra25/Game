#ifndef AA_SPRITES_H
#define AA_SPRITES_H

#include <QPainter>
#include <QPointF>
#include <QColor>
#include "Game/Actors.h"
#include "Core/Types.h"

namespace aa {
namespace Spr {

// Архетипы тел
enum Arch {
    A_Humanoid = 0, A_Skeleton, A_Beast, A_Spider, A_Golem, A_Robed,
    A_Blob, A_Hive, A_Eye, A_Elemental, A_Horror, A_Plant, A_Wisp, A_Demon
};

struct Pal
{
    QColor main, dark, light, accent, eye;
};

Pal palFor(int tint);
int archFor(int tint);

// качество теней: общий выключатель «пятен» под существами (настройка видео)
void setBlobShadows(bool on);
bool blobShadows();

// --- существа ---
void drawActor(QPainter *p, const Actor &a, float time);
void drawPlayer(QPainter *p, const Actor &a, float time, int classId, int weaponGlyph, bool shieldEquipped);
void drawShadow(QPainter *p, const QPointF &pos, float rx, float ry, float alpha);
void drawStatusFx(QPainter *p, const Actor &a, float time);
void drawHealthPip(QPainter *p, const Actor &a);

// --- окружение ---
void drawProp(QPainter *p, const Prop &pr, float time, Biome biome);
void drawGroundItem(QPainter *p, const Item &it, const QPointF &pos, float time, Rarity rar, float pop = 1.f);
void drawGoldPile(QPainter *p, const QPointF &pos, float time, int amount);
void drawNpc(QPainter *p, const Npc &n, float time);

// --- эффекты ---
void drawProjectile(QPainter *p, const Proj &pr, float time);
void drawAttackFx(QPainter *p, const AttackFx &fx, float time);   // взмахи/когти/волны/круги
void drawZone(QPainter *p, const Zone &z, float time);
void drawTelegraph(QPainter *p, const Telegraph &t);
void drawParticle(QPainter *p, const Part &pt);
void drawPortal(QPainter *p, const QPointF &pos, float time, bool active);
void drawLightHalo(QPainter *p, const QPointF &pos, float radius, const QColor &c, float power = 1.f);

} // namespace Spr
} // namespace aa

#endif // AA_SPRITES_H
