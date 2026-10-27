#include "Game/Sprites.h"
#include "Core/Style.h"
#include "Core/Data.h"
#include "Core/Rng.h"
#include "Game/SpriteBank.h"

#include <QtMath>
#include <QPainterPath>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QHash>

namespace aa {
namespace Spr {

// общий выключатель пятен-теней под существами (настройка качества теней)
static bool g_blobShadows = true;
void setBlobShadows(bool on) { g_blobShadows = on; }
bool blobShadows() { return g_blobShadows; }

// ============================================================
//  ПАЛИТРЫ СУЩЕСТВ
// ============================================================
Pal palFor(int t)
{
    Pal p;
    switch (t) {
    case 0:  p = { QColor(126,116,110), QColor(52,46,46),  QColor(176,164,154), QColor(226,138,60),  QColor(255,150,50)  }; break; // падальщик
    case 1:  p = { QColor(214,204,180), QColor(96,88,74),   QColor(244,238,220), QColor(180,150,90),  QColor(255,220,120) }; break; // лучник-скелет
    case 2:  p = { QColor(58,48,44),    QColor(24,20,20),   QColor(110,86,66),   QColor(255,122,46),  QColor(255,180,60)  }; break; // жук
    case 3:  p = { QColor(142,104,80),  QColor(60,42,34),   QColor(190,150,116), QColor(200,90,50),   QColor(255,140,60)  }; break; // топорщик
    case 4:  p = { QColor(120,60,50),   QColor(46,22,22),   QColor(190,110,80),  QColor(255,150,60),  QColor(255,220,140) }; break; // праховый маг
    case 5:  p = { QColor(176,74,44),   QColor(66,20,14),   QColor(255,150,80),  QColor(255,210,110), QColor(255,240,180) }; break; // БОСС патриарх
    case 6:  p = { QColor(72,58,96),    QColor(28,22,40),   QColor(126,104,160), QColor(190,140,255), QColor(240,190,255) }; break; // теневой клык
    case 7:  p = { QColor(96,110,66),   QColor(40,48,28),   QColor(150,164,104), QColor(210,170,90),  QColor(255,230,140) }; break; // древесный страж
    case 8:  p = { QColor(120,110,70),  QColor(52,46,28),   QColor(180,168,110), QColor(190,230,90),  QColor(240,255,150) }; break; // улей
    case 9:  p = { QColor(110,140,74),  QColor(46,62,30),   QColor(166,196,116), QColor(200,90,120),  QColor(255,140,160) }; break; // дикарь
    case 10: p = { QColor(88,116,70),   QColor(34,48,26),   QColor(146,180,116), QColor(220,120,180), QColor(255,170,220) }; break; // жрица шипов
    case 11: p = { QColor(110,60,80),   QColor(40,20,32),   QColor(176,110,140), QColor(255,120,170), QColor(255,190,220) }; break; // БОСС матка
    case 12: p = { QColor(140,176,206), QColor(60,86,110),  QColor(206,232,250), QColor(120,200,255), QColor(200,240,255) }; break; // ледяной охотник
    case 13: p = { QColor(122,158,190), QColor(48,72,100),  QColor(190,220,244), QColor(140,220,255), QColor(220,250,255) }; break; // голем
    case 14: p = { QColor(150,130,206), QColor(66,52,104),  QColor(210,190,250), QColor(190,150,255), QColor(240,220,255) }; break; // кристальный паук
    case 15: p = { QColor(120,150,190), QColor(48,66,96),   QColor(186,214,244), QColor(150,220,255), QColor(230,250,255) }; break; // инейный чародей
    case 16: p = { QColor(222,230,238), QColor(120,134,150),QColor(250,252,255), QColor(150,210,255), QColor(210,240,255) }; break; // снежный волк
    case 17: p = { QColor(150,196,232), QColor(58,92,128),  QColor(220,240,255), QColor(140,220,255), QColor(230,250,255) }; break; // БОСС вьюговей
    case 18: p = { QColor(96,116,58),   QColor(38,48,22),   QColor(150,172,90),  QColor(190,240,90),  QColor(230,255,150) }; break; // гниль
    case 19: p = { QColor(150,124,104), QColor(64,50,42),   QColor(200,176,152), QColor(220,80,70),   QColor(255,150,140) }; break; // трупоед
    case 20: p = { QColor(84,66,120),   QColor(32,24,52),   QColor(140,116,190), QColor(200,140,255), QColor(240,200,255) }; break; // жрец морока
    case 21: p = { QColor(84,102,96),   QColor(32,44,42),   QColor(140,160,152), QColor(120,200,190), QColor(200,255,245) }; break; // затонувший страж
    case 22: p = { QColor(150,160,96),  QColor(64,72,38),   QColor(206,214,140), QColor(200,240,90),  QColor(240,255,160) }; break; // чумной стрелок
    case 23: p = { QColor(104,120,58),  QColor(40,50,22),   QColor(160,180,96),  QColor(190,255,90),  QColor(235,255,160) }; break; // БОСС колосс
    case 24: p = { QColor(255,140,50),  QColor(150,50,20),  QColor(255,210,120), QColor(255,240,180), QColor(255,255,220) }; break; // элементаль
    case 25: p = { QColor(120,64,44),   QColor(48,22,16),   QColor(190,110,70),  QColor(255,150,50),  QColor(255,220,140) }; break; // страж горнила
    case 26: p = { QColor(120,90,180),  QColor(44,28,80),   QColor(180,150,240), QColor(230,180,255), QColor(255,240,255) }; break; // пустотный глаз
    case 27: p = { QColor(150,110,190), QColor(60,36,90),   QColor(210,170,250), QColor(255,150,255), QColor(255,220,255) }; break; // искажённый
    case 28: p = { QColor(70,50,120),   QColor(24,16,52),   QColor(140,110,220), QColor(220,170,255), QColor(255,230,255) }; break; // звёздный ужас
    case 29: p = { QColor(180,120,240), QColor(50,20,90),   QColor(240,200,255), QColor(255,220,120), QColor(255,255,240) }; break; // БОСС пожиратель
    case 30: p = { QColor(120,150,70),  QColor(50,68,28),   QColor(180,210,110), QColor(240,150,200), QColor(255,210,240) }; break; // отпрыск терний
    case 31: p = { QColor(110,110,150), QColor(40,40,66),   QColor(170,170,220), QColor(190,190,255), QColor(240,240,255) }; break; // тень
    case 32: p = { QColor(220,120,60),  QColor(110,44,20),  QColor(255,180,110), QColor(255,230,150), QColor(255,255,210) }; break; // пепельный отпрыск
    default: p = { QColor(150,140,140), QColor(60,56,56),   QColor(210,200,200), QColor(240,180,80),  QColor(255,220,140) }; break;
    }
    return p;
}

int archFor(int t)
{
    switch (t) {
    case 0: case 3: case 9: case 12: case 27: return A_Humanoid;
    case 1: case 22: return A_Skeleton;
    case 6: case 16: case 19: return A_Beast;
    case 2: case 14: case 11: return A_Spider;
    case 7: case 13: case 21: case 25: return A_Golem;
    case 4: case 10: case 15: case 20: return A_Robed;
    case 18: case 23: return A_Blob;
    case 8: return A_Hive;
    case 26: return A_Eye;
    case 24: case 32: return A_Elemental;
    case 28: case 29: return A_Horror;
    case 30: return A_Plant;
    case 31: return A_Wisp;
    case 5: case 17: return A_Demon;
    default: return A_Humanoid;
    }
}

// ============================================================
//  ХЕЛПЕРЫ
// ============================================================
static void blob(QPainter *p, const QPointF &c, qreal rx, qreal ry, const QColor &fill,
                 const QColor &line = QColor(), qreal lw = 1.0)
{
    p->setBrush(fill);
    p->setPen(line.isValid() ? QPen(line, lw) : QPen(Qt::NoPen));
    p->drawEllipse(c, rx, ry);
}

static void polyf(QPainter *p, const QVector<QPointF> &pts, const QColor &fill,
                  const QColor &line = QColor(), qreal lw = 1.0)
{
    QPolygonF pg;
    for (int i = 0; i < pts.size(); ++i) pg << pts.at(i);
    p->setBrush(fill);
    p->setPen(line.isValid() ? QPen(line, lw) : QPen(Qt::NoPen));
    p->drawPolygon(pg);
}

static void limb(QPainter *p, const QPointF &a, const QPointF &b, qreal w, const QColor &c)
{
    p->setPen(QPen(c, w, Qt::SolidLine, Qt::RoundCap));
    p->drawLine(a, b);
}

static void glowDot(QPainter *p, const QPointF &c, qreal r, const QColor &col, float power = 1.f)
{
    Style::glow(p, c, r * 3.2f, col, power);
    p->setPen(Qt::NoPen);
    p->setBrush(col.lighter(160));
    p->drawEllipse(c, r * 0.55f, r * 0.55f);
}

// ============================================================
//  ЧАСТИ ТЕЛ
// ============================================================
static void bodyHumanoid(QPainter *p, const Actor &a, const Pal &c, float t)
{
    // присед в изготовке перед рывком/ударом
    if (a.windup > 0.02f) p->scale(1.f + a.windup * 0.10f, 1.f - a.windup * 0.16f);
    float spd = qSqrt(a.vel.x() * a.vel.x() + a.vel.y() * a.vel.y());
    float walk = qSin(a.animT * 9.f) * (spd < 5.f ? 0.15f : 1.f);
    float bob = qAbs(qSin(a.animT * 9.f)) * 1.4f;
    float R = a.radius;
    float big = R > 16.f ? 1.25f : 1.f;
    float lean = clampf(spd * 0.030f, 0.f, 7.f) * (qCos(a.facing) >= 0.f ? 1.f : -1.f);
    p->rotate(lean);
    float breathe = 1.f + qSin(t * 2.3f + float(a.id) * 0.7f) * (spd < 5.f ? 0.020f : 0.008f);
    float bl = fmodf(t * 0.55f + float(a.id) * 0.37f, 3.2f);
    bool blink = bl > 3.02f;

    // ноги
    limb(p, QPointF(-R * 0.30f, -R * 0.35f), QPointF(-R * 0.38f + walk * 4, 0), R * 0.30f, c.dark);
    limb(p, QPointF(R * 0.30f, -R * 0.35f), QPointF(R * 0.38f - walk * 4, 0), R * 0.30f, c.dark);

    // торс
    QVector<QPointF> torso;
    torso << QPointF(-R * 0.52f * big, (-R * 0.42f - bob) * breathe)
          << QPointF(-R * 0.40f * big, (-R * 1.50f * big - bob) * breathe)
          << QPointF(R * 0.40f * big, (-R * 1.50f * big - bob) * breathe)
          << QPointF(R * 0.52f * big, (-R * 0.42f - bob) * breathe);
    polyf(p, torso, c.main, c.dark, 1.2);
    // рёбра/пояс
    p->setPen(QPen(c.dark, 1.2));
    p->drawLine(QPointF(-R * 0.42f, -R * 0.86f - bob), QPointF(R * 0.42f, -R * 0.86f - bob));
    polyf(p, QVector<QPointF>() << QPointF(-R * 0.20f, -R * 1.42f * big - bob)
            << QPointF(0, -R * 1.05f - bob) << QPointF(R * 0.20f, -R * 1.42f * big - bob), c.accent.darker(120));

    // плечи
    blob(p, QPointF(-R * 0.52f * big, -R * 1.42f * big - bob), R * 0.24f * big, R * 0.22f * big, c.light, c.dark, 1.0);
    blob(p, QPointF(R * 0.52f * big, -R * 1.42f * big - bob), R * 0.24f * big, R * 0.22f * big, c.light, c.dark, 1.0);

    // голова
    float hy = -R * 1.72f * big - bob;
    blob(p, QPointF(0, hy), R * 0.40f, R * 0.42f, c.light, c.dark, 1.2);
    // челюсть
    polyf(p, QVector<QPointF>() << QPointF(-R * 0.24f, hy + R * 0.16f) << QPointF(R * 0.24f, hy + R * 0.16f)
          << QPointF(0, hy + R * 0.48f), c.dark);
    // глаза (с морганием)
    float ex = qCos(a.facing) * R * 0.10f;
    if (blink) {
        p->setPen(QPen(c.eye.darker(130), 1.4));
        p->drawLine(QPointF(-R * 0.24f + ex, hy - R * 0.04f), QPointF(-R * 0.06f + ex, hy - R * 0.04f));
        p->drawLine(QPointF(R * 0.06f + ex, hy - R * 0.04f), QPointF(R * 0.24f + ex, hy - R * 0.04f));
    } else {
        glowDot(p, QPointF(-R * 0.15f + ex, hy - R * 0.04f), R * 0.13f, c.eye, 0.9f);
        glowDot(p, QPointF(R * 0.15f + ex, hy - R * 0.04f), R * 0.13f, c.eye, 0.9f);
    }

    // руки + оружие в направлении взгляда
    p->save();
    float swing = a.attackAnim > 0.f ? swingCurve(1.f - a.attackAnim) : 0.f;
    p->translate(0, (-R * 1.20f * big - bob) * breathe);
    p->rotate(qRadiansToDegrees(a.facing) + 90.f + qRadiansToDegrees(swing) * 0.75f);
    // трейл клинка в фазе удара
    if (a.attackAnim > 0.f) {
        float tt = 1.f - a.attackAnim;
        if (tt > 0.22f && tt < 0.66f) {
            p->save();
            p->setCompositionMode(QPainter::CompositionMode_Plus);
            for (int g = 1; g <= 3; ++g) {
                p->save();
                p->rotate(-g * 15.f);
                QColor gc = c.light;
                gc.setAlpha(int(70.f / float(g)));
                polyf(p, QVector<QPointF>() << QPointF(0, -R * 1.55f) << QPointF(R * 0.15f, -R * 1.1f)
                      << QPointF(R * 0.15f, -R * 0.35f) << QPointF(-R * 0.15f, -R * 0.35f)
                      << QPointF(-R * 0.15f, -R * 1.1f), gc);
                p->restore();
            }
            p->restore();
        }
    }
    limb(p, QPointF(0, 0), QPointF(0, -R * 0.85f), R * 0.20f, c.main);
    // оружие
    p->setPen(QPen(c.dark, 2.0));
    p->drawLine(QPointF(-R * 0.12f, -R * 0.80f), QPointF(R * 0.12f, -R * 0.80f));
    polyf(p, QVector<QPointF>() << QPointF(-R * 0.10f, -R * 0.85f) << QPointF(R * 0.10f, -R * 0.85f)
          << QPointF(R * 0.05f, -R * 1.55f) << QPointF(-R * 0.05f, -R * 1.55f), c.light, c.dark, 1.0);
    p->restore();
}

static void bodySkeleton(QPainter *p, const Actor &a, const Pal &c, float t)
{
    Q_UNUSED(t);
    float R = a.radius;
    float walk = qSin(a.animT * 8.f);
    limb(p, QPointF(-R * 0.24f, -R * 0.4f), QPointF(-R * 0.30f + walk * 4, 0), R * 0.18f, c.main);
    limb(p, QPointF(R * 0.24f, -R * 0.4f), QPointF(R * 0.30f - walk * 4, 0), R * 0.18f, c.main);
    // таз
    polyf(p, QVector<QPointF>() << QPointF(-R * 0.34f, -R * 0.42f) << QPointF(R * 0.34f, -R * 0.42f)
          << QPointF(R * 0.20f, -R * 0.66f) << QPointF(-R * 0.20f, -R * 0.66f), c.main, c.dark, 1.0);
    // позвоночник
    p->setPen(QPen(c.main, R * 0.16f, Qt::SolidLine, Qt::RoundCap));
    p->drawLine(QPointF(0, -R * 0.62f), QPointF(0, -R * 1.30f));
    // рёбра
    p->setPen(QPen(c.main, 1.6));
    for (int i = 0; i < 4; ++i) {
        float y = -R * (0.78f + i * 0.15f);
        p->drawLine(QPointF(-R * (0.42f - i * 0.05f), y), QPointF(R * (0.42f - i * 0.05f), y));
    }
    // череп
    float hy = -R * 1.62f;
    blob(p, QPointF(0, hy), R * 0.40f, R * 0.42f, c.light, c.dark, 1.2);
    blob(p, QPointF(-R * 0.15f, hy + R * 0.02f), R * 0.13f, R * 0.15f, QColor(10, 8, 10));
    blob(p, QPointF(R * 0.15f, hy + R * 0.02f), R * 0.13f, R * 0.15f, QColor(10, 8, 10));
    glowDot(p, QPointF(-R * 0.15f, hy + R * 0.02f), R * 0.08f, c.eye, 1.1f);
    glowDot(p, QPointF(R * 0.15f, hy + R * 0.02f), R * 0.08f, c.eye, 1.1f);
    p->setPen(QPen(c.dark, 1.0));
    for (int i = 0; i < 4; ++i)
        p->drawLine(QPointF(-R * 0.16f + i * R * 0.11f, hy + R * 0.28f), QPointF(-R * 0.16f + i * R * 0.11f, hy + R * 0.42f));

    // лук
    p->save();
    p->translate(0, -R * 1.05f);
    p->rotate(qRadiansToDegrees(a.facing) + 90.f + (a.attackAnim > 0 ? qSin(a.attackAnim * float(M_PI)) * 24.f : 0));
    p->setPen(QPen(c.accent, 2.2, Qt::SolidLine, Qt::RoundCap));
    QPainterPath bow;
    bow.moveTo(QPointF(-R * 0.55f, -R * 0.70f));
    bow.quadTo(QPointF(R * 0.30f, 0), QPointF(-R * 0.55f, R * 0.70f));
    p->drawPath(bow);
    p->setPen(QPen(c.light, 1.0));
    p->drawLine(QPointF(-R * 0.55f, -R * 0.70f), QPointF(-R * 0.55f, R * 0.70f));
    p->restore();
}

static void bodyBeast(QPainter *p, const Actor &a, const Pal &c, float t)
{
    Q_UNUSED(t);
    float R = a.radius;
    float walk = qSin(a.animT * 12.f);
    float bob = qAbs(qSin(a.animT * 12.f)) * 1.2f;
    float fx = qCos(a.facing), fy = qSin(a.facing) * 0.5f;

    // лапы
    limb(p, QPointF(-R * 0.55f + fx * R * 0.2f, -R * 0.30f), QPointF(-R * 0.62f + fx * R * 0.2f + walk * 5, 0), R * 0.17f, c.dark);
    limb(p, QPointF(R * 0.55f + fx * R * 0.2f, -R * 0.30f), QPointF(R * 0.62f + fx * R * 0.2f - walk * 5, 0), R * 0.17f, c.dark);
    limb(p, QPointF(-R * 0.35f - fx * R * 0.3f, -R * 0.30f), QPointF(-R * 0.40f - fx * R * 0.3f - walk * 5, 0), R * 0.16f, c.main.darker(130));
    limb(p, QPointF(R * 0.35f - fx * R * 0.3f, -R * 0.30f), QPointF(R * 0.40f - fx * R * 0.3f + walk * 5, 0), R * 0.16f, c.main.darker(130));

    // туловище
    blob(p, QPointF(fx * R * 0.05f, -R * 0.62f - bob), R * 0.78f, R * 0.46f, c.main, c.dark, 1.2);
    // холка
    polyf(p, QVector<QPointF>() << QPointF(-R * 0.30f, -R * 0.95f - bob) << QPointF(0, -R * 1.15f - bob)
          << QPointF(R * 0.30f, -R * 0.95f - bob), c.dark);
    // хвост
    p->setPen(QPen(c.main, R * 0.16f, Qt::SolidLine, Qt::RoundCap));
    QPainterPath tail;
    tail.moveTo(QPointF(-fx * R * 0.70f, -R * 0.70f - bob));
    tail.quadTo(QPointF(-fx * R * 1.20f, -R * 1.00f - bob + walk * 3), QPointF(-fx * R * 1.45f, -R * 0.55f - bob));
    p->drawPath(tail);

    // голова
    QPointF h(fx * R * 0.85f, -R * 0.80f - bob + fy * R * 0.3f);
    blob(p, h, R * 0.42f, R * 0.36f, c.light, c.dark, 1.2);
    // морда
    polyf(p, QVector<QPointF>() << h + QPointF(fx * R * 0.28f, -R * 0.10f) << h + QPointF(fx * R * 0.70f, R * 0.05f)
          << h + QPointF(fx * R * 0.28f, R * 0.20f), c.main, c.dark, 1.0);
    // уши
    polyf(p, QVector<QPointF>() << h + QPointF(-R * 0.18f, -R * 0.28f) << h + QPointF(-R * 0.06f, -R * 0.62f)
          << h + QPointF(R * 0.10f, -R * 0.30f), c.dark);
    polyf(p, QVector<QPointF>() << h + QPointF(R * 0.14f, -R * 0.28f) << h + QPointF(R * 0.26f, -R * 0.58f)
          << h + QPointF(R * 0.36f, -R * 0.24f), c.dark);
    // глаза
    glowDot(p, h + QPointF(fx * R * 0.16f - R * 0.10f, -R * 0.06f), R * 0.10f, c.eye, 1.2f);
    glowDot(p, h + QPointF(fx * R * 0.16f + R * 0.10f, -R * 0.06f), R * 0.10f, c.eye, 1.2f);
    // клыки
    if (a.attackAnim > 0.f) {
        p->setPen(QPen(C::bone, 1.4));
        for (int i = 0; i < 3; ++i)
            p->drawLine(h + QPointF(fx * R * (0.45f + i * 0.08f), R * 0.10f), h + QPointF(fx * R * (0.45f + i * 0.08f), R * 0.26f));
    }
}

static void bodySpider(QPainter *p, const Actor &a, const Pal &c, float t)
{
    float R = a.radius;
    float fx = qCos(a.facing);
    float walk = a.animT * 11.f;
    float bossy = a.boss ? 1.25f : 1.f;

    // лапы
    for (int side = -1; side <= 1; side += 2) {
        for (int i = 0; i < 4; ++i) {
            float ph = walk + i * 1.1f + (side > 0 ? 0.55f : 0.f);
            float lift = qMax(0.f, float(qSin(ph))) * R * 0.30f;
            float ang = (-0.95f + i * 0.62f) * side;
            QPointF k1(qCos(ang) * R * 0.95f * side * 0.6f + fx * R * 0.1f, -R * 0.55f + qSin(ang) * R * 0.55f);
            QPointF k2(qCos(ang) * R * 1.55f * side * 0.7f + fx * R * 0.1f, -R * 0.95f + qSin(ang) * R * 0.85f - lift);
            QPointF k3(qCos(ang) * R * 1.75f * side * 0.7f + fx * R * 0.1f, -R * 0.05f - lift * 0.2f);
            limb(p, k1, k2, R * 0.13f * bossy, c.dark);
            limb(p, k2, k3, R * 0.10f * bossy, c.main.darker(120));
        }
    }
    // брюшко
    blob(p, QPointF(-fx * R * 0.45f, -R * 0.72f), R * 0.82f * bossy, R * 0.68f * bossy, c.main, c.dark, 1.4);
    // узор
    polyf(p, QVector<QPointF>() << QPointF(-fx * R * 0.45f - R * 0.22f, -R * 0.95f)
          << QPointF(-fx * R * 0.45f, -R * 1.20f) << QPointF(-fx * R * 0.45f + R * 0.22f, -R * 0.95f)
          << QPointF(-fx * R * 0.45f, -R * 0.72f), c.accent.darker(110));
    // головогрудь
    blob(p, QPointF(fx * R * 0.42f, -R * 0.62f), R * 0.50f, R * 0.42f, c.light, c.dark, 1.2);
    // глаза
    for (int i = 0; i < 4; ++i) {
        float ex = (i - 1.5f) * R * 0.16f;
        glowDot(p, QPointF(fx * R * 0.62f + ex * (1 - qAbs(fx) * 0.4f), -R * 0.70f + qAbs(ex) * 0.25f),
                R * 0.085f, c.eye, 1.2f);
    }
    // хелицеры
    p->setPen(QPen(c.dark, R * 0.10f, Qt::SolidLine, Qt::RoundCap));
    p->drawLine(QPointF(fx * R * 0.78f, -R * 0.52f), QPointF(fx * R * 1.05f, -R * 0.30f));
    p->drawLine(QPointF(fx * R * 0.72f, -R * 0.44f), QPointF(fx * R * 0.95f, -R * 0.18f));
    if (a.boss) {
        // яйцевые мешки
        for (int i = 0; i < 3; ++i)
            blob(p, QPointF(-fx * R * (0.60f + i * 0.22f), -R * (0.50f + (i % 2) * 0.2f)), R * 0.26f, R * 0.24f,
                 c.accent.darker(110), c.dark, 1.0);
    }
    Q_UNUSED(t);
}

static void bodyGolem(QPainter *p, const Actor &a, const Pal &c, float t)
{
    float R = a.radius;
    float walk = qSin(a.animT * 5.5f);
    float bob = qAbs(qSin(a.animT * 5.5f)) * 1.6f;

    // ноги-колонны
    polyf(p, QVector<QPointF>() << QPointF(-R * 0.62f, 0) << QPointF(-R * 0.20f, 0)
          << QPointF(-R * 0.24f, -R * 0.62f) << QPointF(-R * 0.58f, -R * 0.62f), c.dark, c.main.darker(140), 1.2);
    polyf(p, QVector<QPointF>() << QPointF(R * 0.20f + walk * 2, 0) << QPointF(R * 0.62f + walk * 2, 0)
          << QPointF(R * 0.58f, -R * 0.62f) << QPointF(R * 0.24f, -R * 0.62f), c.dark, c.main.darker(140), 1.2);
    // корпус
    QVector<QPointF> body;
    body << QPointF(-R * 0.72f, -R * 0.60f - bob) << QPointF(-R * 0.60f, -R * 1.62f - bob)
         << QPointF(R * 0.60f, -R * 1.62f - bob) << QPointF(R * 0.72f, -R * 0.60f - bob);
    polyf(p, body, c.main, c.dark, 1.6);
    // трещины со свечением
    p->setPen(QPen(c.accent, 1.6));
    float pulse = 0.55f + 0.45f * qSin(t * 2.4f);
    QColor gc = c.accent; gc.setAlphaF(pulse);
    p->setPen(QPen(gc, 1.8));
    p->drawLine(QPointF(-R * 0.40f, -R * 1.40f - bob), QPointF(-R * 0.10f, -R * 1.05f - bob));
    p->drawLine(QPointF(-R * 0.10f, -R * 1.05f - bob), QPointF(-R * 0.30f, -R * 0.75f - bob));
    p->drawLine(QPointF(R * 0.30f, -R * 1.45f - bob), QPointF(R * 0.15f, -R * 1.00f - bob));
    // плечи-валуны
    blob(p, QPointF(-R * 0.78f, -R * 1.44f - bob), R * 0.34f, R * 0.30f, c.light, c.dark, 1.4);
    blob(p, QPointF(R * 0.78f, -R * 1.44f - bob), R * 0.34f, R * 0.30f, c.light, c.dark, 1.4);
    // руки
    float sw = a.attackAnim > 0.f ? qSin(a.attackAnim * float(M_PI)) * R * 0.9f : 0.f;
    limb(p, QPointF(-R * 0.78f, -R * 1.30f - bob), QPointF(-R * 0.92f, -R * 0.52f - bob + sw * 0.3f), R * 0.26f, c.main.darker(115));
    limb(p, QPointF(R * 0.78f, -R * 1.30f - bob), QPointF(R * 0.92f + sw, -R * 0.52f - bob - sw * 0.5f), R * 0.26f, c.main.darker(115));
    blob(p, QPointF(R * 0.92f + sw, -R * 0.48f - bob - sw * 0.5f), R * 0.30f, R * 0.28f, c.light, c.dark, 1.2);
    // голова-плита
    polyf(p, QVector<QPointF>() << QPointF(-R * 0.34f, -R * 1.66f - bob) << QPointF(R * 0.34f, -R * 1.66f - bob)
          << QPointF(R * 0.26f, -R * 2.14f - bob) << QPointF(-R * 0.26f, -R * 2.14f - bob), c.light, c.dark, 1.4);
    glowDot(p, QPointF(-R * 0.12f, -R * 1.90f - bob), R * 0.10f, c.eye, 1.4f);
    glowDot(p, QPointF(R * 0.12f, -R * 1.90f - bob), R * 0.10f, c.eye, 1.4f);
}

static void bodyRobed(QPainter *p, const Actor &a, const Pal &c, float t)
{
    float R = a.radius;
    float hov = qSin(t * 2.2f + a.animT) * R * 0.10f;
    float fx = qCos(a.facing);

    // подол
    QPainterPath robe;
    robe.moveTo(QPointF(-R * 0.66f, R * 0.10f - hov));
    robe.cubicTo(QPointF(-R * 0.78f, -R * 0.90f - hov), QPointF(-R * 0.42f, -R * 1.62f - hov), QPointF(0, -R * 1.72f - hov));
    robe.cubicTo(QPointF(R * 0.42f, -R * 1.62f - hov), QPointF(R * 0.78f, -R * 0.90f - hov), QPointF(R * 0.66f, R * 0.10f - hov));
    // рваный край
    for (int i = 6; i >= -6; --i) {
        float x = R * 0.66f * (i / 6.0f);
        float y = R * 0.10f - hov + qSin(i * 1.7f + t * 3.f) * R * 0.10f + (qAbs(i) % 2 ? R * 0.12f : 0);
        robe.lineTo(QPointF(x, y));
    }
    robe.closeSubpath();
    p->setBrush(c.main);
    p->setPen(QPen(c.dark, 1.4));
    p->drawPath(robe);
    // отделка
    p->setPen(QPen(c.accent.darker(110), 1.6));
    p->drawLine(QPointF(0, -R * 1.60f - hov), QPointF(0, -R * 0.10f - hov));

    // капюшон
    blob(p, QPointF(0, -R * 1.72f - hov), R * 0.44f, R * 0.46f, c.light, c.dark, 1.3);
    QPainterPath hood;
    hood.moveTo(QPointF(-R * 0.46f, -R * 1.66f - hov));
    hood.cubicTo(QPointF(-R * 0.40f, -R * 2.34f - hov), QPointF(R * 0.40f, -R * 2.34f - hov), QPointF(R * 0.46f, -R * 1.66f - hov));
    hood.cubicTo(QPointF(R * 0.20f, -R * 1.84f - hov), QPointF(-R * 0.20f, -R * 1.84f - hov), QPointF(-R * 0.46f, -R * 1.66f - hov));
    p->setBrush(c.dark);
    p->setPen(QPen(c.main.darker(150), 1.0));
    p->drawPath(hood);
    // глаза во тьме капюшона (моргание)
    if (fmodf(t * 0.6f + float(a.id) * 0.53f, 3.4f) > 3.2f) {
        p->setPen(QPen(c.eye.darker(140), 1.2));
        p->drawLine(QPointF(-R * 0.22f + fx * R * 0.06f, -R * 1.86f - hov), QPointF(-R * 0.06f + fx * R * 0.06f, -R * 1.86f - hov));
        p->drawLine(QPointF(R * 0.06f + fx * R * 0.06f, -R * 1.86f - hov), QPointF(R * 0.22f + fx * R * 0.06f, -R * 1.86f - hov));
    } else {
        glowDot(p, QPointF(-R * 0.14f + fx * R * 0.06f, -R * 1.86f - hov), R * 0.09f, c.eye, 1.5f);
        glowDot(p, QPointF(R * 0.14f + fx * R * 0.06f, -R * 1.86f - hov), R * 0.09f, c.eye, 1.5f);
    }

    // посох (поднимается в изготовке)
    p->save();
    p->translate(R * 0.62f, -R * 0.90f - hov);
    p->rotate(qSin(t * 1.6f) * 4.f + (a.attackAnim > 0.f ? qSin(a.attackAnim * float(M_PI)) * 26.f : 0.f)
              - a.windup * 38.f);
    p->setPen(QPen(c.dark.lighter(120), R * 0.12f, Qt::SolidLine, Qt::RoundCap));
    p->drawLine(QPointF(0, R * 0.95f), QPointF(0, -R * 1.30f));
    glowDot(p, QPointF(0, -R * 1.42f), R * 0.20f * (1.f + a.windup * 0.7f), c.accent,
            1.3f + (a.attackAnim > 0.f ? 0.7f : 0.f) + a.windup * 1.2f);
    p->restore();
    // руна приготовления под ногами
    if (a.windup > 0.05f) {
        p->save();
        p->setCompositionMode(QPainter::CompositionMode_Plus);
        p->scale(1.0, 0.42);
        QColor wc = c.accent;
        wc.setAlpha(int(140 * a.windup));
        p->setPen(QPen(wc, 1.6));
        p->setBrush(Qt::NoBrush);
        p->drawPath(runeRing(QPointF(0, 0), R * (1.1f + 0.5f * a.windup), 8, R * 0.2f, t * 3.f));
        p->restore();
    }
}

static void bodyBlob(QPainter *p, const Actor &a, const Pal &c, float t)
{
    float R = a.radius;
    float bossy = a.boss ? 1.15f : 1.f;
    // тело-жижа
    QPainterPath path;
    int N = 14;
    for (int i = 0; i <= N; ++i) {
        float ang = float(i) / float(N) * 2.f * float(M_PI);
        float wob = 1.f + qSin(ang * 3.f + t * 3.2f) * 0.10f + qSin(ang * 5.f - t * 2.1f) * 0.06f;
        float rx = R * 1.05f * bossy * wob;
        float ry = R * 0.92f * bossy * (2.f - wob) * 0.98f;
        QPointF pt(qCos(ang) * rx, -R * 0.85f * bossy + qSin(ang) * ry);
        if (i == 0) path.moveTo(pt); else path.lineTo(pt);
    }
    path.closeSubpath();
    QRadialGradient g(QPointF(-R * 0.3f, -R * 1.2f), R * 1.8f);
    g.setColorAt(0, c.light);
    g.setColorAt(0.55, c.main);
    g.setColorAt(1, c.dark);
    p->setBrush(g);
    p->setPen(QPen(c.dark.darker(130), 1.6));
    p->drawPath(path);
    // пузыри
    for (int i = 0; i < 4; ++i) {
        float ph = t * 1.6f + i * 1.9f;
        float bx = qSin(ph * 0.8f + i) * R * 0.55f;
        float by = -R * 0.6f - (fmodf(ph, 2.f)) * R * 0.35f;
        blob(p, QPointF(bx, by), R * 0.10f, R * 0.10f, c.accent.lighter(120));
    }
    // глаза
    int eyes = a.boss ? 5 : 3;
    for (int i = 0; i < eyes; ++i) {
        float ang = -float(M_PI) * 0.75f + float(i) * float(M_PI) * 0.5f / float(eyes);
        QPointF e(qCos(ang) * R * 0.42f, -R * 1.05f * bossy + qSin(ang) * R * 0.22f);
        blob(p, e, R * 0.16f, R * 0.19f, C::bone, c.dark, 1.0);
        glowDot(p, e + QPointF(qCos(a.facing) * R * 0.05f, 0), R * 0.08f, c.eye, 1.2f);
    }
    // щупальца-подтёки
    p->setPen(QPen(c.main.darker(120), R * 0.16f, Qt::SolidLine, Qt::RoundCap));
    for (int i = 0; i < 3; ++i) {
        float x = (-1 + i) * R * 0.55f;
        p->drawLine(QPointF(x, -R * 0.20f), QPointF(x + qSin(t * 2.f + i) * R * 0.15f, R * 0.05f));
    }
}

static void bodyHive(QPainter *p, const Actor &a, const Pal &c, float t)
{
    float R = a.radius;
    // масса
    QPainterPath path;
    int N = 12;
    for (int i = 0; i <= N; ++i) {
        float ang = float(i) / float(N) * 2.f * float(M_PI);
        float rr = R * (1.15f + qSin(ang * 4.f + t) * 0.08f);
        QPointF pt(qCos(ang) * rr, -R * 1.0f + qSin(ang) * rr * 0.85f);
        if (i == 0) path.moveTo(pt); else path.lineTo(pt);
    }
    path.closeSubpath();
    p->setBrush(c.main);
    p->setPen(QPen(c.dark, 1.6));
    p->drawPath(path);
    // соты
    for (int i = 0; i < 6; ++i) {
        float ang = i * 1.05f;
        QPointF q(qCos(ang) * R * 0.5f, -R * 1.0f + qSin(ang * 1.7f) * R * 0.45f);
        float pulse = 0.5f + 0.5f * qSin(t * 3.f + i);
        QColor gc = c.accent; gc.setAlphaF(0.35f + pulse * 0.5f);
        blob(p, q, R * 0.16f, R * 0.16f, gc, c.dark, 1.0);
    }
    // лапки-корни
    p->setPen(QPen(c.dark, R * 0.12f, Qt::SolidLine, Qt::RoundCap));
    for (int i = 0; i < 5; ++i) {
        float x = (-2 + i) * R * 0.42f;
        p->drawLine(QPointF(x, -R * 0.25f), QPointF(x * 1.25f, R * 0.05f));
    }
}

static void bodyEye(QPainter *p, const Actor &a, const Pal &c, float t)
{
    float R = a.radius;
    float hov = qSin(t * 2.0f + a.animT) * R * 0.14f;
    // щупальца
    p->setPen(QPen(c.dark, R * 0.13f, Qt::SolidLine, Qt::RoundCap));
    for (int i = 0; i < 6; ++i) {
        float ang = float(i) / 6.f * 2.f * float(M_PI) + t * 0.4f;
        QPointF st(qCos(ang) * R * 0.55f, -R * 0.95f + qSin(ang) * R * 0.45f + hov);
        QPointF en(qCos(ang) * R * 1.35f, -R * 0.55f + qSin(ang) * R * 1.0f + hov + qSin(t * 3.f + i) * R * 0.15f);
        p->drawLine(st, en);
    }
    // глазное яблоко
    blob(p, QPointF(0, -R * 1.05f + hov), R * 0.85f, R * 0.80f, c.light, c.dark, 1.6);
    float ex = qCos(a.facing) * R * 0.28f, ey = qSin(a.facing) * R * 0.20f;
    blob(p, QPointF(ex, -R * 1.05f + ey + hov), R * 0.36f, R * 0.36f, c.main, c.dark, 1.0);
    blob(p, QPointF(ex, -R * 1.05f + ey + hov), R * 0.17f, R * 0.17f, QColor(10, 6, 16));
    glowDot(p, QPointF(ex * 1.1f, -R * 1.05f + ey + hov), R * 0.12f, c.eye, 1.4f);
    // жилки
    p->setPen(QPen(c.accent.darker(110), 1.0));
    for (int i = 0; i < 4; ++i) {
        float ang = i * 1.6f + 0.4f;
        p->drawLine(QPointF(qCos(ang) * R * 0.45f, -R * 1.05f + qSin(ang) * R * 0.45f + hov),
                    QPointF(qCos(ang) * R * 0.78f, -R * 1.05f + qSin(ang) * R * 0.72f + hov));
    }
}

static void bodyElemental(QPainter *p, const Actor &a, const Pal &c, float t)
{
    float R = a.radius;
    float fx = qCos(a.facing);
    // огненное тело
    for (int layer = 0; layer < 3; ++layer) {
        float k = 1.0f - layer * 0.22f;
        QColor col = (layer == 0) ? c.dark : (layer == 1 ? c.main : c.light);
        col.setAlphaF(0.85f - layer * 0.1f);
        QPainterPath path;
        int N = 12;
        for (int i = 0; i <= N; ++i) {
            float ang = float(i) / float(N) * 2.f * float(M_PI);
            float flick = 1.f + qSin(ang * 3.f + t * 7.f + layer) * 0.14f + qSin(ang * 6.f - t * 5.f) * 0.07f;
            float rr = R * k * flick * 1.05f;
            float yy = -R * 1.05f * k + qSin(ang) * rr * 1.15f;
            QPointF pt(qCos(ang) * rr * 0.9f + fx * R * 0.06f, yy);
            if (i == 0) path.moveTo(pt); else path.lineTo(pt);
        }
        path.closeSubpath();
        p->setBrush(col);
        p->setPen(Qt::NoPen);
        p->drawPath(path);
    }
    // ядро
    glowDot(p, QPointF(0, -R * 1.05f), R * 0.34f, c.eye, 1.6f);
    // глаза
    glowDot(p, QPointF(-R * 0.26f + fx * R * 0.1f, -R * 1.30f), R * 0.10f, c.accent, 1.4f);
    glowDot(p, QPointF(R * 0.26f + fx * R * 0.1f, -R * 1.30f), R * 0.10f, c.accent, 1.4f);
    // языки пламени вверх
    p->setPen(QPen(c.light, R * 0.14f, Qt::SolidLine, Qt::RoundCap));
    for (int i = 0; i < 3; ++i) {
        float x = (-1 + i) * R * 0.42f;
        float hh = R * (0.5f + 0.4f * qAbs(qSin(t * 5.f + i * 2.f)));
        p->drawLine(QPointF(x, -R * 1.70f), QPointF(x + qSin(t * 4.f + i) * R * 0.2f, -R * 1.70f - hh));
    }
}

static void bodyHorror(QPainter *p, const Actor &a, const Pal &c, float t)
{
    float R = a.radius;
    float hov = qSin(t * 1.4f) * R * 0.12f;
    // щупальца
    for (int i = 0; i < 8; ++i) {
        float ang = float(i) / 8.f * 2.f * float(M_PI) + t * 0.3f;
        QPainterPath path;
        QPointF base(qCos(ang) * R * 0.5f, -R * 1.1f + qSin(ang) * R * 0.4f + hov);
        path.moveTo(base);
        for (int k = 1; k <= 3; ++k) {
            float kk = k / 3.0f;
            path.lineTo(base + QPointF(qCos(ang + qSin(t * 2.f + i) * 0.6f) * R * 1.5f * kk,
                                       qSin(ang) * R * 1.2f * kk + R * 0.5f * kk + qSin(t * 3.f + i) * R * 0.12f));
        }
        p->setPen(QPen(i % 2 ? c.dark : c.main, R * (0.20f - 0.0f) * (1.f - i * 0.02f), Qt::SolidLine, Qt::RoundCap));
        p->drawPath(path);
    }
    // ядро-звезда
    QRadialGradient g(QPointF(0, -R * 1.15f + hov), R * 1.1f);
    g.setColorAt(0, c.light);
    g.setColorAt(0.4, c.main);
    g.setColorAt(1, c.dark);
    p->setBrush(g);
    p->setPen(QPen(c.dark, 1.4));
    p->drawEllipse(QPointF(0, -R * 1.15f + hov), R * 0.95f, R * 0.95f);
    p->setBrush(c.light);
    p->setPen(Qt::NoPen);
    p->drawPolygon(starPolygon(QPointF(0, -R * 1.15f + hov), R * 0.75f, R * 0.26f, 5, t * 0.6f));
    // множество глаз
    for (int i = 0; i < 5; ++i) {
        float ang = t * 0.5f + i * 1.25f;
        QPointF e(qCos(ang) * R * 0.55f, -R * 1.15f + qSin(ang * 1.4f) * R * 0.42f + hov);
        blob(p, e, R * 0.13f, R * 0.16f, C::bone, c.dark, 0.9);
        glowDot(p, e, R * 0.07f, c.eye, 1.3f);
    }
    Style::glow(p, QPointF(0, -R * 1.15f + hov), R * 2.4f, c.accent, 0.5f);
}

static void bodyPlant(QPainter *p, const Actor &a, const Pal &c, float t)
{
    float R = a.radius;
    float sway = qSin(t * 2.5f + a.animT) * R * 0.10f;
    // стебель
    p->setPen(QPen(c.dark.lighter(120), R * 0.22f, Qt::SolidLine, Qt::RoundCap));
    p->drawLine(QPointF(0, 0), QPointF(sway, -R * 1.30f));
    // листья
    for (int i = 0; i < 3; ++i) {
        float ang = -0.7f + i * 0.7f + qSin(t * 2.f + i) * 0.1f;
        polyf(p, QVector<QPointF>() << QPointF(sway * 0.5f, -R * (0.5f + i * 0.25f))
              << QPointF(sway * 0.5f + qCos(ang) * R * 0.8f, -R * (0.5f + i * 0.25f) + qSin(ang) * R * 0.4f)
              << QPointF(sway * 0.5f + qCos(ang) * R * 0.3f, -R * (0.5f + i * 0.25f) + R * 0.22f), c.main, c.dark, 1.0);
    }
    // бутон с зубами
    blob(p, QPointF(sway, -R * 1.55f), R * 0.55f, R * 0.48f, c.light, c.dark, 1.3);
    float open = 0.35f + (a.attackAnim > 0.f ? qSin(a.attackAnim * float(M_PI)) * 0.5f : qMax(0.f, float(qSin(t * 1.6f))) * 0.18f);
    polyf(p, QVector<QPointF>() << QPointF(sway - R * 0.5f, -R * 1.60f) << QPointF(sway, -R * (1.60f + open))
          << QPointF(sway + R * 0.5f, -R * 1.60f) << QPointF(sway, -R * 1.30f), c.accent.darker(130), c.dark, 1.0);
    p->setPen(QPen(C::bone, 1.2));
    for (int i = 0; i < 4; ++i) {
        float x = sway - R * 0.36f + i * R * 0.24f;
        p->drawLine(QPointF(x, -R * 1.55f), QPointF(x, -R * 1.42f));
    }
    glowDot(p, QPointF(sway - R * 0.16f, -R * 1.78f), R * 0.08f, c.eye, 1.0f);
    glowDot(p, QPointF(sway + R * 0.16f, -R * 1.78f), R * 0.08f, c.eye, 1.0f);
}

static void bodyWisp(QPainter *p, const Actor &a, const Pal &c, float t)
{
    float R = a.radius;
    float hov = qSin(t * 3.f + a.animT) * R * 0.16f;
    QRadialGradient g(QPointF(0, -R * 1.0f + hov), R * 1.3f);
    g.setColorAt(0, c.light);
    g.setColorAt(0.5, QColor(c.main.red(), c.main.green(), c.main.blue(), 180));
    g.setColorAt(1, QColor(c.dark.red(), c.dark.green(), c.dark.blue(), 0));
    p->setBrush(g);
    p->setPen(Qt::NoPen);
    QPainterPath path;
    path.moveTo(QPointF(-R * 0.6f, -R * 1.0f + hov));
    path.cubicTo(QPointF(-R * 0.7f, -R * 2.0f + hov), QPointF(R * 0.7f, -R * 2.0f + hov), QPointF(R * 0.6f, -R * 1.0f + hov));
    for (int i = 4; i >= -4; --i) {
        float x = R * 0.6f * (i / 4.0f);
        path.lineTo(QPointF(x, -R * 1.0f + hov + qAbs(i) * R * 0.14f + qSin(t * 5.f + i) * R * 0.08f));
    }
    path.closeSubpath();
    p->drawPath(path);
    glowDot(p, QPointF(-R * 0.20f, -R * 1.55f + hov), R * 0.10f, c.eye, 1.5f);
    glowDot(p, QPointF(R * 0.20f, -R * 1.55f + hov), R * 0.10f, c.eye, 1.5f);
}

static void bodyDemon(QPainter *p, const Actor &a, const Pal &c, float t)
{
    float R = a.radius;
    float walk = qSin(a.animT * 5.f);
    float bob = qAbs(qSin(a.animT * 5.f)) * R * 0.06f;
    // ноги
    limb(p, QPointF(-R * 0.38f, -R * 0.50f), QPointF(-R * 0.46f + walk * R * 0.14f, 0), R * 0.32f, c.dark);
    limb(p, QPointF(R * 0.38f, -R * 0.50f), QPointF(R * 0.46f - walk * R * 0.14f, 0), R * 0.32f, c.dark);
    // торс
    polyf(p, QVector<QPointF>() << QPointF(-R * 0.72f, -R * 0.52f - bob) << QPointF(-R * 0.82f, -R * 1.70f - bob)
          << QPointF(R * 0.82f, -R * 1.70f - bob) << QPointF(R * 0.72f, -R * 0.52f - bob), c.main, c.dark, 1.8);
    // грудная пластина
    polyf(p, QVector<QPointF>() << QPointF(-R * 0.34f, -R * 1.60f - bob) << QPointF(0, -R * 1.10f - bob)
          << QPointF(R * 0.34f, -R * 1.60f - bob), c.accent.darker(120), c.dark, 1.2);
    glowDot(p, QPointF(0, -R * 1.32f - bob), R * 0.14f, c.eye, 1.4f + 0.4f * float(qSin(t * 3.f)));
    // руки
    float sw = a.attackAnim > 0.f ? swingCurve(1.f - a.attackAnim) * R * 1.1f : 0.f;
    limb(p, QPointF(-R * 0.80f, -R * 1.55f - bob), QPointF(-R * 1.05f, -R * 0.62f - bob), R * 0.28f, c.main.darker(115));
    limb(p, QPointF(R * 0.80f, -R * 1.55f - bob), QPointF(R * 1.05f + sw, -R * 0.62f - bob - sw * 0.4f), R * 0.28f, c.main.darker(115));
    // когти
    p->setPen(QPen(c.light, 2.0, Qt::SolidLine, Qt::RoundCap));
    for (int i = -1; i <= 1; ++i)
        p->drawLine(QPointF(R * 1.05f + sw + i * R * 0.12f, -R * 0.62f - bob - sw * 0.4f),
                    QPointF(R * 1.15f + sw + i * R * 0.16f, -R * 0.30f - bob - sw * 0.4f));
    // голова
    float hy = -R * 2.05f - bob;
    blob(p, QPointF(0, hy), R * 0.48f, R * 0.50f, c.light, c.dark, 1.5);
    // рога
    p->setPen(QPen(c.dark.lighter(130), R * 0.16f, Qt::SolidLine, Qt::RoundCap));
    p->drawLine(QPointF(-R * 0.34f, hy - R * 0.22f), QPointF(-R * 0.72f, hy - R * 0.80f));
    p->drawLine(QPointF(R * 0.34f, hy - R * 0.22f), QPointF(R * 0.72f, hy - R * 0.80f));
    // глаза и пасть (моргание)
    if (fmodf(t * 0.4f + 0.7f, 4.0f) > 3.8f) {
        p->setPen(QPen(c.dark, 2.0));
        p->drawLine(QPointF(-R * 0.28f, hy - R * 0.04f), QPointF(-R * 0.08f, hy - R * 0.04f));
        p->drawLine(QPointF(R * 0.08f, hy - R * 0.04f), QPointF(R * 0.28f, hy - R * 0.04f));
    } else {
        glowDot(p, QPointF(-R * 0.18f, hy - R * 0.04f), R * 0.11f, c.eye, 1.8f);
        glowDot(p, QPointF(R * 0.18f, hy - R * 0.04f), R * 0.11f, c.eye, 1.8f);
    }
    polyf(p, QVector<QPointF>() << QPointF(-R * 0.26f, hy + R * 0.20f) << QPointF(R * 0.26f, hy + R * 0.20f)
          << QPointF(0, hy + R * 0.52f), QColor(20, 8, 8), c.dark, 1.0);
    p->setPen(QPen(C::bone, 1.4));
    for (int i = 0; i < 4; ++i)
        p->drawLine(QPointF(-R * 0.20f + i * R * 0.13f, hy + R * 0.20f), QPointF(-R * 0.20f + i * R * 0.13f, hy + R * 0.36f));
    // аура
    Style::glow(p, QPointF(0, -R * 1.3f), R * 2.6f, c.accent, 0.35f + 0.15f * float(qSin(t * 2.f)));
}

// ============================================================
//  ПЕРСОНАЖ ИГРОКА
// ============================================================
void drawPlayer(QPainter *p, const Actor &a, float t, int classId, int weaponGlyph, bool shieldEquipped)
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->translate(a.pos);

    float R = a.radius;
    // тень
    p->save();
    p->translate(-a.pos.x(), -a.pos.y());
    drawShadow(p, a.pos, R * 0.95f, R * 0.40f, 0.40f);
    p->restore();

    // лист героя («Пепельный странник»): если есть - заменяет процедурную отрисовку
    const SprBank::Sheet *sheetPl = SprBank::get(QStringLiteral("player"));
    bool useSheetPl = sheetPl && sheetPl->valid && !sheetPl->anims.isEmpty();
    if (a.rollT > 0.f && !useSheetPl) {
        // процедурный кувырок крутит всё тело; у листа свои кадры roll
        float k = 1.f - a.rollT;
        p->rotate(k * 360.f * (qCos(a.rollDir) >= 0 ? 1 : -1));
        p->scale(1.f, 0.72f);
    }

    float spdP = qSqrt(a.vel.x() * a.vel.x() + a.vel.y() * a.vel.y());
    float walk = qSin(a.animT * 10.f) * (spdP > 5.f ? 1.f : 0.12f);
    float bob = qAbs(qSin(a.animT * 10.f)) * 1.6f;
    float breatheP = 1.f + qSin(t * 2.1f) * (spdP < 5.f ? 0.018f : 0.006f);
    float sway = qSin(a.animT * 8.f) * R * 0.14f + walk * R * 0.10f + clampf(spdP * 0.02f, 0.f, R * 0.22f);
    float blinkP = fmodf(t * 0.5f + 1.3f, 3.6f) > 3.42f;
    p->rotate(clampf(spdP * 0.028f, 0.f, 6.f) * (qCos(a.facing) >= 0.f ? 1.f : -1.f));
    // тёплый контур героя, чтобы он не терялся в темноте
    Style::glow(p, QPointF(0, -R * 1.05f), R * 2.3f, QColor(255, 200, 120), 0.20f);
    QColor cloak[4] = { QColor(176, 74, 48), QColor(70, 96, 176), QColor(96, 150, 92), QColor(140, 96, 160) };
    QColor cl = cloak[qBound(0, classId, 3)];
    QColor clDark = cl.darker(190);
    QColor skin(226, 196, 164);
    QColor armorC(96, 88, 104);
    QColor armorL(146, 136, 154);

    // опциональный спрайт-лист героя
    if (useSheetPl) {
        const SprBank::Sheet *sh = sheetPl;
        {
            float spd = qSqrt(a.vel.x() * a.vel.x() + a.vel.y() * a.vel.y());
            QString an;
            float prog = -1.f;   // >= 0: кадры ключуются прогрессом действия, не часами
            if (a.dead) {
                an = QStringLiteral("death");
            } else if (a.rollT > 0.f) {
                an = QStringLiteral("roll");
                prog = 1.f - a.rollT;
            } else if (a.attackAnim > 0.f) {
                // разные атаки - разные анимации листа:
                // 1 магия «cast», 2 выстрел «shoot», 3 силовой приём «attack2»,
                // 0 обычные удары чередуют «attack»/«attack2»
                if (a.attackKind == 1)      an = QStringLiteral("cast");
                else if (a.attackKind == 2) an = QStringLiteral("shoot");
                else if (a.attackKind == 3) an = QStringLiteral("attack2");
                else an = (a.attackSeq & 1) ? QStringLiteral("attack2") : QStringLiteral("attack");
                if (!sh->anims.contains(an)) an = QStringLiteral("attack");
                prog = 1.f - a.attackAnim;
            } else if (spd > 5.f) {
                an = QStringLiteral("walk");
            } else {
                an = QStringLiteral("idle");
            }
            if (!sh->anims.contains(an)) an = QStringLiteral("idle");
            const SprBank::Anim am = sh->anims.value(an);
            int n = am.to - am.from + 1;
            if (n > 0) {
                int fr;
                if (prog >= 0.f) {
                    fr = am.from + qMin(n - 1, int(clampf(prog, 0.f, 1.f) * n));
                } else if (a.dead) {
                    fr = am.from + qMin(n - 1, int(a.deathT * sh->fps));
                } else if (an == QLatin1String("idle")) {
                    fr = am.from + (int(t * qMin(sh->fps, 4.f)) % n);   // спокойное дыхание
                } else {
                    fr = am.from + (int(a.animT * sh->fps) % n);
                }
                int cols = qMax(1, sh->img.width() / sh->fw);
                QRectF sr((fr % cols) * sh->fw, (fr / cols) * sh->fh, sh->fw, sh->fh);
                float k = (R * 4.1f) / float(sh->fh);
                bool flip = qCos(a.facing) < 0.f;
                p->save();
                p->scale(k * (flip ? -1.f : 1.f), k);
                p->setRenderHint(QPainter::SmoothPixmapTransform, true);
                p->drawImage(QRectF(-sh->ax, -sh->ay, sh->fw, sh->fh), sh->img, sr);
                p->restore();
                if (a.st.shield > 0.f) {
                    p->setPen(QPen(QColor(140, 200, 255, 170), 2.0));
                    p->setBrush(QColor(120, 190, 255, 34));
                    p->drawEllipse(QPointF(0, -R * 0.9f), R * 1.5f, R * 1.6f);
                }
                p->restore();
                return;
            }
        }
    }

    // плащ сзади
    p->save();
    QPainterPath cape;
    cape.moveTo(QPointF(-R * 0.55f, -R * 1.50f - bob));
    cape.cubicTo(QPointF(-R * 0.95f - sway, -R * 0.60f - bob + walk * 3), QPointF(-R * 0.60f - sway * 1.4f, -R * 0.10f), QPointF(-R * 0.10f - sway, -R * 0.30f));
    cape.lineTo(QPointF(R * 0.10f - sway, -R * 0.30f));
    cape.cubicTo(QPointF(R * 0.60f - sway * 1.4f, -R * 0.10f), QPointF(R * 0.95f - sway, -R * 0.60f - bob - walk * 3), QPointF(R * 0.55f, -R * 1.50f - bob));
    cape.closeSubpath();
    p->setBrush(clDark);
    p->setPen(QPen(cl.darker(240), 1.2));
    p->drawPath(cape);
    p->restore();

    // ноги
    limb(p, QPointF(-R * 0.26f, -R * 0.40f), QPointF(-R * 0.32f + walk * 5, 0), R * 0.26f, QColor(58, 50, 62));
    limb(p, QPointF(R * 0.26f, -R * 0.40f), QPointF(R * 0.32f - walk * 5, 0), R * 0.26f, QColor(58, 50, 62));
    // сапоги
    blob(p, QPointF(-R * 0.32f + walk * 5, -R * 0.04f), R * 0.20f, R * 0.12f, QColor(40, 34, 44));
    blob(p, QPointF(R * 0.32f - walk * 5, -R * 0.04f), R * 0.20f, R * 0.12f, QColor(40, 34, 44));

    // торс
    polyf(p, QVector<QPointF>() << QPointF(-R * 0.48f, (-R * 0.46f - bob) * breatheP) << QPointF(-R * 0.42f, (-R * 1.42f - bob) * breatheP)
          << QPointF(R * 0.42f, (-R * 1.42f - bob) * breatheP) << QPointF(R * 0.48f, (-R * 0.46f - bob) * breatheP), armorC, QColor(30, 26, 36), 1.3);
    // перевязь
    p->setPen(QPen(cl, R * 0.14f));
    p->drawLine(QPointF(-R * 0.40f, -R * 1.36f - bob), QPointF(R * 0.34f, -R * 0.56f - bob));
    // пряжка
    glowDot(p, QPointF(R * 0.02f, -R * 0.96f - bob), R * 0.09f, C::amber, 0.8f);

    // голова + капюшон
    float hy = -R * 1.72f - bob;
    blob(p, QPointF(0, hy), R * 0.34f, R * 0.36f, skin, QColor(90, 60, 48), 1.0);
    QPainterPath hood;
    hood.moveTo(QPointF(-R * 0.44f, hy + R * 0.16f));
    hood.cubicTo(QPointF(-R * 0.52f, hy - R * 0.62f), QPointF(R * 0.52f, hy - R * 0.62f), QPointF(R * 0.44f, hy + R * 0.16f));
    hood.cubicTo(QPointF(R * 0.24f, hy - R * 0.10f), QPointF(-R * 0.24f, hy - R * 0.10f), QPointF(-R * 0.44f, hy + R * 0.16f));
    p->setBrush(cl);
    p->setPen(QPen(clDark, 1.3));
    p->drawPath(hood);
    // глаза (моргание)
    float ex = qCos(a.facing) * R * 0.10f;
    if (blinkP) {
        p->setPen(QPen(QColor(90, 60, 48), 1.2));
        p->drawLine(QPointF(-R * 0.17f + ex, hy + R * 0.02f), QPointF(-R * 0.06f + ex, hy + R * 0.02f));
        p->drawLine(QPointF(R * 0.06f + ex, hy + R * 0.02f), QPointF(R * 0.17f + ex, hy + R * 0.02f));
    } else {
        glowDot(p, QPointF(-R * 0.12f + ex, hy + R * 0.02f), R * 0.055f, QColor(255, 230, 180), 0.8f);
        glowDot(p, QPointF(R * 0.12f + ex, hy + R * 0.02f), R * 0.055f, QColor(255, 230, 180), 0.8f);
    }

    // щит
    if (shieldEquipped) {
        p->save();
        p->translate(-R * 0.72f, -R * 0.96f - bob);
        p->rotate(-14.f);
        polyf(p, QVector<QPointF>() << QPointF(-R * 0.34f, -R * 0.44f) << QPointF(R * 0.34f, -R * 0.44f)
              << QPointF(R * 0.28f, R * 0.28f) << QPointF(0, R * 0.56f) << QPointF(-R * 0.28f, R * 0.28f),
              armorL, QColor(28, 24, 32), 1.4);
        blob(p, QPointF(0, -R * 0.04f), R * 0.12f, R * 0.12f, C::amber);
        p->restore();
    }

    // оружие в руке (поворот по направлению взгляда)
    p->save();
    p->translate(R * 0.52f, (-R * 1.05f - bob) * breatheP);
    float swing = 0.f;
    if (a.attackAnim > 0.f) swing = qRadiansToDegrees(swingCurve(1.f - a.attackAnim)) * 1.15f;
    p->rotate(qRadiansToDegrees(a.facing) + 90.f + swing);
    // трейл оружия в фазе удара
    if (a.attackAnim > 0.f) {
        float tt = 1.f - a.attackAnim;
        if (tt > 0.22f && tt < 0.66f) {
            p->save();
            p->setCompositionMode(QPainter::CompositionMode_Plus);
            for (int g = 1; g <= 3; ++g) {
                p->save();
                p->rotate(-g * 17.f);
                QColor gc(255, 226, 170, int(80.f / float(g)));
                polyf(p, QVector<QPointF>() << QPointF(0, -R * 1.6f) << QPointF(R * 0.14f, -R * 1.15f)
                      << QPointF(R * 0.14f, -R * 0.3f) << QPointF(-R * 0.14f, -R * 0.3f)
                      << QPointF(-R * 0.14f, -R * 1.15f), gc);
                p->restore();
            }
            p->restore();
        }
    }
    QColor steel(206, 214, 226);
    switch (weaponGlyph) {
    case Gl_Bow:
        p->setPen(QPen(QColor(120, 88, 54), R * 0.11f, Qt::SolidLine, Qt::RoundCap));
        {
            QPainterPath bw;
            bw.moveTo(QPointF(-R * 0.10f, -R * 0.85f));
            bw.quadTo(QPointF(R * 0.55f, 0), QPointF(-R * 0.10f, R * 0.85f));
            p->drawPath(bw);
            p->setPen(QPen(C::boneDim, 1.0));
            p->drawLine(QPointF(-R * 0.10f, -R * 0.85f), QPointF(-R * 0.10f, R * 0.85f));
        }
        break;
    case Gl_Staff:
        p->setPen(QPen(QColor(96, 70, 48), R * 0.13f, Qt::SolidLine, Qt::RoundCap));
        p->drawLine(QPointF(0, R * 0.70f), QPointF(0, -R * 1.35f));
        glowDot(p, QPointF(0, -R * 1.45f), R * 0.20f, classId == 1 ? C::ember : C::arcane, 1.3f);
        break;
    case Gl_Wand:
        p->setPen(QPen(QColor(70, 56, 88), R * 0.10f, Qt::SolidLine, Qt::RoundCap));
        p->drawLine(QPointF(0, R * 0.40f), QPointF(0, -R * 0.95f));
        glowDot(p, QPointF(0, -R * 1.02f), R * 0.14f, C::lightning, 1.4f);
        break;
    case Gl_Tome:
        polyf(p, QVector<QPointF>() << QPointF(-R * 0.34f, -R * 0.44f) << QPointF(R * 0.34f, -R * 0.44f)
              << QPointF(R * 0.34f, R * 0.30f) << QPointF(-R * 0.34f, R * 0.30f), QColor(96, 48, 48), QColor(40, 20, 24), 1.2);
        glowDot(p, QPointF(0, -R * 0.06f), R * 0.12f, C::arcane, 1.0f);
        break;
    case Gl_Axe: case Gl_GreatAxe: {
        float s = (weaponGlyph == Gl_GreatAxe) ? 1.35f : 1.0f;
        p->setPen(QPen(QColor(96, 70, 48), R * 0.12f, Qt::SolidLine, Qt::RoundCap));
        p->drawLine(QPointF(0, R * 0.55f), QPointF(0, -R * 1.10f * s));
        polyf(p, QVector<QPointF>() << QPointF(0, -R * 0.95f * s) << QPointF(R * 0.52f * s, -R * 0.78f * s)
              << QPointF(R * 0.56f * s, -R * 1.22f * s) << QPointF(0, -R * 1.32f * s), steel, QColor(40, 44, 54), 1.2);
        break;
    }
    case Gl_Spear:
        p->setPen(QPen(QColor(96, 70, 48), R * 0.10f, Qt::SolidLine, Qt::RoundCap));
        p->drawLine(QPointF(0, R * 0.80f), QPointF(0, -R * 1.50f));
        polyf(p, QVector<QPointF>() << QPointF(0, -R * 1.80f) << QPointF(R * 0.16f, -R * 1.42f)
              << QPointF(0, -R * 1.32f) << QPointF(-R * 0.16f, -R * 1.42f), steel, QColor(40, 44, 54), 1.0);
        break;
    case Gl_Dagger:
        polyf(p, QVector<QPointF>() << QPointF(0, -R * 0.95f) << QPointF(R * 0.13f, -R * 0.30f)
              << QPointF(-R * 0.13f, -R * 0.30f), steel, QColor(40, 44, 54), 1.0);
        p->setPen(QPen(QColor(120, 90, 54), R * 0.16f));
        p->drawLine(QPointF(-R * 0.20f, -R * 0.26f), QPointF(R * 0.20f, -R * 0.26f));
        break;
    case Gl_Mace:
        p->setPen(QPen(QColor(96, 70, 48), R * 0.13f, Qt::SolidLine, Qt::RoundCap));
        p->drawLine(QPointF(0, R * 0.45f), QPointF(0, -R * 0.80f));
        blob(p, QPointF(0, -R * 0.98f), R * 0.30f, R * 0.30f, armorL, QColor(30, 28, 36), 1.2);
        for (int i = 0; i < 4; ++i) {
            float ang = i * float(M_PI) / 2.f + 0.4f;
            polyf(p, QVector<QPointF>() << QPointF(qCos(ang) * R * 0.26f, -R * 0.98f + qSin(ang) * R * 0.26f)
                  << QPointF(qCos(ang) * R * 0.48f, -R * 0.98f + qSin(ang) * R * 0.48f)
                  << QPointF(qCos(ang + 0.7f) * R * 0.26f, -R * 0.98f + qSin(ang + 0.7f) * R * 0.26f), armorL);
        }
        break;
    default: { // меч / двуручник
        float s = (weaponGlyph == Gl_GreatSword) ? 1.40f : 1.0f;
        polyf(p, QVector<QPointF>() << QPointF(0, -R * 1.55f * s) << QPointF(R * 0.13f, -R * 1.20f * s)
              << QPointF(R * 0.13f, -R * 0.34f) << QPointF(-R * 0.13f, -R * 0.34f) << QPointF(-R * 0.13f, -R * 1.20f * s),
              steel, QColor(40, 44, 54), 1.2);
        p->setPen(QPen(C::boneDim.lighter(110), 1.0));
        p->drawLine(QPointF(0, -R * 1.40f * s), QPointF(0, -R * 0.40f));
        p->setPen(QPen(C::edgeHi, R * 0.13f, Qt::SolidLine, Qt::RoundCap));
        p->drawLine(QPointF(-R * 0.32f, -R * 0.30f), QPointF(R * 0.32f, -R * 0.30f));
        p->setPen(QPen(QColor(96, 70, 48), R * 0.12f, Qt::SolidLine, Qt::RoundCap));
        p->drawLine(QPointF(0, -R * 0.28f), QPointF(0, R * 0.10f));
        break;
    }
    }
    p->restore();

    // след рывка
    if (a.rollT > 0.f) {
        QColor gc = C::arcane;
        gc.setAlpha(int(140 * a.rollT));
        p->setPen(Qt::NoPen);
        p->setBrush(gc);
        p->drawEllipse(QPointF(0, 0), R * 1.2f * (1.2f - a.rollT), R * 0.5f);
    }
    // барьер
    if (a.st.shield > 0.f) {
        p->setPen(QPen(QColor(140, 200, 255, 170), 2.0));
        p->setBrush(QColor(120, 190, 255, 34));
        p->drawEllipse(QPointF(0, -R * 0.9f), R * 1.5f, R * 1.6f);
    }
    // ярость/жажда крови
    if (a.st.bloodlust > 0.f) {
        Style::glow(p, QPointF(0, -R * 1.0f), R * 2.4f, C::bloodHi, 0.45f + 0.2f * float(qSin(t * 8.f)));
    }
    p->restore();
}

// ============================================================
//  ОБЩИЙ ВЫЗОВ ОТРИСОВКИ СУЩЕСТВА
// ============================================================
// Мягкая контактная тень: готовая текстура (тёмное ядро под ногами + плавный
// спад к краям) вместо жёсткого градиентного овала. Дёшево и правдоподобнее.
static const QImage &shadowTex()
{
    static QImage img;
    if (!img.isNull()) return img;
    const int S = 96;
    QImage t(S, S, QImage::Format_ARGB32_Premultiplied);
    const float h = S * 0.5f;
    for (int y = 0; y < S; ++y) {
        QRgb *ln = reinterpret_cast<QRgb *>(t.scanLine(y));
        for (int x = 0; x < S; ++x) {
            float dx = (x + 0.5f - h) / h, dy = (y + 0.5f - h) / h;
            float d = qSqrt(dx * dx + dy * dy);
            float a = 0.f;
            if (d < 0.35f) {
                a = 1.f - 0.15f * (d / 0.35f);                    // плотное ядро
            } else if (d < 1.f) {
                float tt = (d - 0.35f) / 0.65f;
                a = 0.85f * (1.f - tt * tt * (3.f - 2.f * tt));   // smoothstep-спад
            }
            ln[x] = qPremultiply(qRgba(0, 0, 0, int(a * 255.f)));
        }
    }
    img = t;
    return img;
}

void drawShadow(QPainter *p, const QPointF &pos, float rx, float ry, float alpha)
{
    if (!g_blobShadows || alpha <= 0.01f || rx <= 0.5f || ry <= 0.5f) return;
    // чуть уже тела и со сдвигом вниз - тень «лежит» на земле, а не висит ореолом
    const float k = 0.90f;
    QRectF tgt(pos.x() - rx * k, pos.y() - ry * k + ry * 0.25f, rx * 2.f * k, ry * 2.f * k);
    p->save();
    p->setOpacity(qMin(0.62f, clampf(alpha, 0.f, 1.f) * 1.1f));
    p->setRenderHint(QPainter::SmoothPixmapTransform, true);
    p->drawImage(tgt, shadowTex());
    p->restore();
}

void drawActor(QPainter *p, const Actor &a, float time)
{
    if (a.radius <= 0.5f) return;
    Pal c = palFor(a.tint);
    int arch = archFor(a.tint);

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);

    float vr = a.baseRadius * a.drawScale;                 // визуальный радиус
    float sc = vr / qMax(1.f, a.radius);                   // коллизия и картинка раздельны
    drawShadow(p, a.pos, vr * (a.flying ? 0.7f : 1.0f), vr * 0.38f, a.flying ? 0.22f : 0.40f);

    p->translate(a.pos);
    if (a.dead) {
        float k = easeOutCubic(clampf(a.deathT, 0.f, 1.f));
        p->setOpacity(1.f - k);
        float dir = qCos(a.tiltDir) >= 0.f ? 1.f : -1.f;
        p->rotate(qSin(k * 1.5708f) * 74.f * dir);          // падает на бок
        p->scale(1.f + k * 0.16f, 1.f - k * 0.52f);
        p->translate(0, k * a.radius * 0.30f);
    } else {
        if (a.spawnT < 1.f) {                                 // появление с упругим scale-in
            float sp = easeOutBack(a.spawnT);
            p->scale(qMax(0.05f, sp), qMax(0.05f, sp));
            p->setOpacity(clampf(a.spawnT * 2.5f, 0.f, 1.f));
        }
        if (a.hitTilt > 0.01f) {                              // флинч от удара
            float f = a.hitTilt * a.hitTilt;
            p->rotate((qCos(a.tiltDir) >= 0.f ? 1.f : -1.f) * f * 10.f);
            p->scale(1.f + f * 0.10f, 1.f - f * 0.13f);
        }
    }
    p->scale(sc, sc);

    if (a.dead && a.boss) {
        // имплозия: сходящееся руническое кольцо + восходящие искры
        float k = clampf(a.deathT, 0.f, 1.f);
        p->save();
        p->setCompositionMode(QPainter::CompositionMode_Plus);
        p->scale(1.0, 0.45);
        QColor rc = c.accent;
        rc.setAlpha(int(200 * (1.f - k)));
        p->setPen(QPen(rc, 3.0));
        p->setBrush(Qt::NoBrush);
        p->drawPath(runeRing(QPointF(0, 0), vr * (2.2f - 1.7f * easeOutCubic(k)), 14, vr * 0.24f, time * 2.2f));
        p->restore();
    }

    // аура элитника
    if (a.elite && !a.dead) {
        float pulse = 0.5f + 0.5f * qSin(time * 3.f + a.eliteAuraT);
        QColor ac = a.eliteAffixes.isEmpty() ? C::amber : C::amber;
        Style::glow(p, QPointF(0, -a.radius), a.radius * 2.6f, ac, 0.20f + 0.16f * pulse);
        p->setPen(QPen(QColor(ac.red(), ac.green(), ac.blue(), int(90 + 70 * pulse)), 1.6));
        p->setBrush(Qt::NoBrush);
        p->drawEllipse(QPointF(0, 0), a.radius * 1.25f, a.radius * 0.5f);
    }
    if (a.boss && !a.dead) {
        Style::glow(p, QPointF(0, -vr * 1.2f), vr * 2.6f, c.accent, 0.28f + 0.10f * float(qSin(time * 2.f)));
        // вращающееся руническое кольцо на земле
        p->save();
        p->setCompositionMode(QPainter::CompositionMode_Plus);
        p->translate(0, 0);
        p->scale(1.0, 0.42);
        QColor rc = c.accent;
        rc.setAlpha(120);
        p->setPen(QPen(rc, 2.2));
        p->setBrush(Qt::NoBrush);
        p->drawPath(runeRing(QPointF(0, 0), vr * 1.85f, 12, vr * 0.22f, time * 0.7f));
        rc.setAlpha(70);
        p->setPen(QPen(rc, 1.4));
        p->drawPath(runeRing(QPointF(0, 0), vr * 1.35f, 8, vr * 0.16f, -time * 1.1f));
        p->restore();
    }

    // опциональный спрайт-лист вместо процедурного тела
    bool drewSheet = false;
    {
        const EnemyDef *ed = Data::enemy(a.defId);
        QString key = ed ? ed->id_str : QString();
        const SprBank::Sheet *sh = key.isEmpty() ? 0 : SprBank::get(key);
        if (!sh) sh = SprBank::get(QStringLiteral("mob%1").arg(a.tint));
        if (sh && sh->valid && !sh->anims.isEmpty()) {
            float spd = qSqrt(a.vel.x() * a.vel.x() + a.vel.y() * a.vel.y());
            QString an;
            float prog = -1.f;   // >= 0: кадры по прогрессу действия
            if (a.dead) {
                an = QStringLiteral("death");
            } else if (a.attackAnim > 0.f) {
                // разные атаки - разные анимации листа (как у героя):
                // 1 магия «cast», 2 выстрел «shoot», 3 тяжёлый «attack2»,
                // 0 обычные удары чередуют «attack»/«attack2»
                if (a.attackKind == 1)       an = QStringLiteral("cast");
                else if (a.attackKind == 2)  an = QStringLiteral("shoot");
                else if (a.attackKind == 3)  an = QStringLiteral("attack2");
                else an = (a.attackSeq & 1) ? QStringLiteral("attack2") : QStringLiteral("attack");
                if (!sh->anims.contains(an)) an = QStringLiteral("attack");
                prog = 1.f - a.attackAnim;
            } else if (a.windup > 0.05f && sh->anims.contains(QStringLiteral("cast"))) {
                // изготовление кастера: круг/посох заряжается до выстрела
                an = QStringLiteral("cast");
                prog = clampf(a.windup, 0.f, 1.f) * 0.85f;
            } else if (spd > 5.f) {
                an = QStringLiteral("walk");
            } else {
                an = QStringLiteral("idle");
            }
            if (!sh->anims.contains(an)) an = QStringLiteral("idle");
            if (!sh->anims.contains(an)) an = sh->anims.keys().first();
            const SprBank::Anim am = sh->anims.value(an);
            int n = am.to - am.from + 1;
            if (n > 0) {
                int fr;
                if (prog >= 0.f) {
                    fr = am.from + qMin(n - 1, int(clampf(prog, 0.f, 1.f) * n));
                } else if (a.dead) {
                    fr = am.from + qMin(n - 1, int(a.deathT * sh->fps));
                } else {
                    fr = am.from + (int(a.animT * sh->fps) % n);
                }
                int cols = qMax(1, sh->img.width() / sh->fw);
                QRectF sr((fr % cols) * sh->fw, (fr / cols) * sh->fh, sh->fw, sh->fh);
                float k = (vr * 3.1f) / (float(sh->fh) * qMax(0.001f, sc));
                bool flip = qCos(a.facing) < 0.f;
                p->save();
                p->scale(k * (flip ? -1.f : 1.f), k);
                p->setRenderHint(QPainter::SmoothPixmapTransform, true);
                p->drawImage(QRectF(-sh->ax, -sh->ay, sh->fw, sh->fh), sh->img, sr);
                p->restore();
                drewSheet = true;
            }
        }
    }
    if (!drewSheet) switch (arch) {
    case A_Humanoid:  bodyHumanoid(p, a, c, time); break;
    case A_Skeleton:  bodySkeleton(p, a, c, time); break;
    case A_Beast:     bodyBeast(p, a, c, time); break;
    case A_Spider:    bodySpider(p, a, c, time); break;
    case A_Golem:     bodyGolem(p, a, c, time); break;
    case A_Robed:     bodyRobed(p, a, c, time); break;
    case A_Blob:      bodyBlob(p, a, c, time); break;
    case A_Hive:      bodyHive(p, a, c, time); break;
    case A_Eye:       bodyEye(p, a, c, time); break;
    case A_Elemental: bodyElemental(p, a, c, time); break;
    case A_Horror:    bodyHorror(p, a, c, time); break;
    case A_Plant:     bodyPlant(p, a, c, time); break;
    case A_Wisp:      bodyWisp(p, a, c, time); break;
    case A_Demon:     bodyDemon(p, a, c, time); break;
    default:          bodyHumanoid(p, a, c, time); break;
    }

    // рим-подсветка со стороны источника света (читаемость силуэта)
    if (!a.dead) {
        p->save();
        p->setCompositionMode(QPainter::CompositionMode_Plus);
        QColor rim(255, 214, 150, 70);
        p->setPen(QPen(rim, qMax(1.4f, vr * 0.10f), Qt::SolidLine, Qt::RoundCap));
        p->setBrush(Qt::NoBrush);
        float a0 = qRadiansToDegrees(a.rimAng);
        p->drawArc(QRectF(-vr * 0.85f, -vr * 1.7f, vr * 1.7f, vr * 1.7f), int((a0 - 46) * 16), 92 * 16);
        p->restore();
    }

    // вспышка урона
    if (a.hitFlash > 0.f) {
        p->setCompositionMode(QPainter::CompositionMode_Plus);
        QColor w(255, 200, 190, int(160 * clampf(a.hitFlash, 0.f, 1.f)));
        p->setPen(Qt::NoPen);
        p->setBrush(w);
        p->drawEllipse(QPointF(0, -a.radius), a.radius * 1.25f, a.radius * 1.5f);
        p->setCompositionMode(QPainter::CompositionMode_SourceOver);
    }
    p->restore();

    if (!a.dead) drawStatusFx(p, a, time);
}

void drawStatusFx(QPainter *p, const Actor &a, float t)
{
    if (!a.st.any()) return;
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->translate(a.pos);
    float R = a.baseRadius * a.drawScale;
    if (a.st.burn > 0.f) {
        for (int i = 0; i < 5; ++i) {
            float ph = t * 6.f + i * 1.3f;
            float y = -R * (0.4f + fmodf(ph, 1.6f));
            float x = qSin(ph * 1.7f) * R * 0.6f;
            float s = R * 0.22f * (1.6f - fmodf(ph, 1.6f));
            QColor fc(255, int(140 + 80 * (1.6f - fmodf(ph, 1.6f))), 60, int(200 * clampf(s / (R * 0.3f), 0.f, 1.f)));
            p->setPen(Qt::NoPen);
            p->setBrush(fc);
            p->drawEllipse(QPointF(x, y), s, s * 1.4f);
        }
    }
    if (a.st.chill > 0.f) {
        p->setPen(QPen(QColor(160, 220, 255, 150), 1.4));
        for (int i = 0; i < 6; ++i) {
            float ang = i * float(M_PI) / 3.f + t * 0.6f;
            QPointF c(qCos(ang) * R * 0.9f, -R * 0.8f + qSin(ang) * R * 0.5f);
            p->drawLine(c - QPointF(4, 0), c + QPointF(4, 0));
            p->drawLine(c - QPointF(0, 4), c + QPointF(0, 4));
        }
    }
    if (a.st.poison > 0.f) {
        for (int i = 0; i < 3; ++i) {
            float ph = t * 2.4f + i * 2.1f;
            float y = -R * (1.2f + fmodf(ph, 1.2f));
            float x = qSin(ph) * R * 0.5f;
            blob(p, QPointF(x, y), R * 0.12f, R * 0.12f, QColor(150, 220, 90, 150));
        }
    }
    if (a.st.mark > 0.f) {
        p->setPen(QPen(QColor(255, 90, 80, 190), 2.0));
        p->setBrush(Qt::NoBrush);
        float rr = R * (1.35f + 0.08f * qSin(t * 6.f));
        p->drawEllipse(QPointF(0, -R * 0.9f), rr, rr * 1.1f);
        drawGlyph(p, Gl_Mark, QRectF(-R * 0.3f, -R * 2.6f, R * 0.6f, R * 0.6f), QColor(255, 120, 100));
    }
    if (a.st.shock > 0.f) {
        p->setPen(QPen(QColor(255, 240, 150, 200), 1.6));
        for (int i = 0; i < 3; ++i) {
            float x = qSin(t * 30.f + i * 2.f) * R * 0.8f;
            p->drawLine(QPointF(x, -R * 1.6f), QPointF(x * 0.4f, -R * 0.2f));
        }
    }
    if (a.st.stun > 0.f) {
        for (int i = 0; i < 3; ++i) {
            float ang = t * 4.f + i * 2.09f;
            QPointF c(qCos(ang) * R * 0.9f, -R * 2.1f + qSin(ang) * R * 0.24f);
            drawGlyph(p, Gl_Star, QRectF(c.x() - R * 0.18f, c.y() - R * 0.18f, R * 0.36f, R * 0.36f), C::lightning);
        }
    }
    p->restore();
}

void drawHealthPip(QPainter *p, const Actor &a)
{
    if (a.hp >= a.maxHp - 0.01f || a.dead || a.boss) return;
    float R = a.baseRadius * a.drawScale;
    float w = R * 2.2f;
    QRectF r(a.pos.x() - w * 0.5f, a.pos.y() - R * (a.boss ? 3.4f : 2.7f), w, 4.5f);
    p->save();
    p->setRenderHint(QPainter::Antialiasing, false);
    p->setPen(QPen(QColor(0, 0, 0, 180), 1));
    p->setBrush(QColor(10, 8, 12, 200));
    p->drawRect(r);
    float t = clampf(a.hp / qMax(1.f, a.maxHp), 0.f, 1.f);
    QRectF fr = r.adjusted(1, 1, -1, -1);
    QColor c = a.elite ? C::amber : (t > 0.5f ? C::hpHi : C::blood);
    p->fillRect(QRectF(fr.left(), fr.top(), fr.width() * t, fr.height()), c);
    p->restore();
}

// ============================================================
//  ПРОПСЫ
// ============================================================
void drawProp(QPainter *p, const Prop &pr, float t, Biome biome)
{
    BiomePal pal = biomePalette(biome);
    Rng r(quint64(pr.seed));
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->translate(pr.pos);
    float s = pr.scale;
    float h = pr.height;
    // ветер: качание крон/кустов/грибов с порывами
    if (pr.kind == Pk_Tree || pr.kind == Pk_DeadTree || pr.kind == Pk_Bush
        || pr.kind == Pk_Mushroom || pr.kind == Pk_Stump) {
        float gust = 0.55f + 0.45f * qSin(t * 0.23f + pr.seed % 7);
        float amp = (pr.kind == Pk_Tree ? 2.6f : pr.kind == Pk_DeadTree ? 2.0f : 3.2f);
        p->rotate(qSin(t * 1.35f + (pr.seed % 13) * 0.7f) * amp * gust);
    }

    // тень
    if (pr.solid || pr.kind == Pk_Tree || pr.kind == Pk_House) {
        QRadialGradient g(QPointF(0, 0), pr.radius * 1.6f);
        g.setColorAt(0, QColor(0, 0, 0, 110));
        g.setColorAt(1, QColor(0, 0, 0, 0));
        p->setBrush(g);
        p->setPen(Qt::NoPen);
        p->save();
        p->scale(1.0, 0.42);
        p->drawEllipse(QPointF(0, 0), pr.radius * 1.6f, pr.radius * 1.6f);
        p->restore();
    }

    switch (pr.kind) {
    case Pk_Rock: {
        QPolygonF pg;
        int n = 7;
        for (int i = 0; i < n; ++i) {
            float ang = float(i) / n * 2.f * float(M_PI);
            float rr = pr.radius * (0.7f + r.unit() * 0.5f);
            pg << QPointF(qCos(ang) * rr, -h * 0.30f + qSin(ang) * rr * 0.62f);
        }
        p->setBrush(pal.wallTop.darker(150));
        p->setPen(QPen(pal.wallDark, 1.4));
        p->drawPolygon(pg);
        p->setBrush(QColor(255, 255, 255, 26));
        p->setPen(Qt::NoPen);
        p->drawEllipse(QPointF(-pr.radius * 0.25f, -h * 0.48f), pr.radius * 0.35f, pr.radius * 0.20f);
        break;
    }
    case Pk_Tree: {
        p->setPen(QPen(QColor(58, 42, 30), pr.radius * 0.55f, Qt::SolidLine, Qt::RoundCap));
        p->drawLine(QPointF(0, 0), QPointF(r.gauss() * 3, -h * 0.52f));
        QColor leaf1 = pal.props.contains(Pk_Tree) ? QColor(74, 96, 50) : QColor(90, 80, 56);
        if (biome == Bio_Ember) leaf1 = QColor(96, 116, 58);
        if (biome == Bio_Ash) leaf1 = QColor(84, 74, 62);
        if (biome == Bio_Frost) leaf1 = QColor(150, 176, 190);
        if (biome == Bio_Swamp) leaf1 = QColor(70, 96, 56);
        for (int i = 0; i < 4; ++i) {
            float ang = i * 1.6f + r.unit();
            QPointF c(qCos(ang) * pr.radius * 0.55f, -h * (0.60f + i * 0.10f) + qSin(ang) * pr.radius * 0.3f);
            float rr = pr.radius * (1.35f - i * 0.16f) * s;
            blob(p, c, rr, rr * 0.82f, leaf1.darker(110 + i * 8), leaf1.darker(200), 1.0);
            blob(p, c - QPointF(rr * 0.25f, rr * 0.25f), rr * 0.45f, rr * 0.35f, leaf1.lighter(125));
        }
        break;
    }
    case Pk_DeadTree: {
        p->setPen(QPen(QColor(52, 44, 40), pr.radius * 0.42f, Qt::SolidLine, Qt::RoundCap));
        p->drawLine(QPointF(0, 0), QPointF(0, -h * 0.62f));
        p->setPen(QPen(QColor(52, 44, 40), pr.radius * 0.20f, Qt::SolidLine, Qt::RoundCap));
        for (int i = 0; i < 5; ++i) {
            float y = -h * (0.35f + i * 0.13f);
            float dir = (i % 2) ? 1.f : -1.f;
            p->drawLine(QPointF(0, y), QPointF(dir * pr.radius * (1.4f - i * 0.15f), y - h * 0.22f));
        }
        if (biome == Bio_Swamp || biome == Bio_Ash) {
            p->setPen(QPen(QColor(120, 140, 90, 90), pr.radius * 0.16f));
            p->drawLine(QPointF(-pr.radius * 0.4f, -h * 0.3f), QPointF(pr.radius * 0.6f, -h * 0.5f));
        }
        break;
    }
    case Pk_Crystal: {
        Style::glow(p, QPointF(0, -h * 0.45f), h * 1.5f, pal.accent, 0.30f + 0.12f * float(qSin(t * 2.f + pr.seed)));
        for (int i = 0; i < 4; ++i) {
            float dx = (i - 1.5f) * pr.radius * 0.55f;
            float hh = h * (0.55f + r.unit() * 0.6f);
            float ww = pr.radius * (0.28f + r.unit() * 0.20f);
            QPolygonF pg;
            pg << QPointF(dx, -hh) << QPointF(dx + ww, -hh * 0.35f) << QPointF(dx + ww * 0.5f, 0)
               << QPointF(dx - ww * 0.5f, 0) << QPointF(dx - ww, -hh * 0.35f);
            QColor cc = pal.accent.lighter(110 + i * 12);
            cc.setAlpha(215);
            p->setBrush(cc);
            p->setPen(QPen(pal.accent.lighter(160), 1.2));
            p->drawPolygon(pg);
            p->setPen(QPen(QColor(255, 255, 255, 90), 1.0));
            p->drawLine(QPointF(dx - ww * 0.3f, -hh * 0.8f), QPointF(dx - ww * 0.2f, -hh * 0.15f));
        }
        break;
    }
    case Pk_Bone: {
        p->setPen(QPen(C::boneDim, pr.radius * 0.24f, Qt::SolidLine, Qt::RoundCap));
        p->drawLine(QPointF(-pr.radius, -pr.radius * 0.2f), QPointF(pr.radius, -pr.radius * 0.5f));
        p->drawLine(QPointF(-pr.radius * 0.6f, -pr.radius * 0.8f), QPointF(pr.radius * 0.5f, pr.radius * 0.1f));
        blob(p, QPointF(pr.radius * 0.8f, -pr.radius * 0.9f), pr.radius * 0.55f, pr.radius * 0.45f, C::bone, C::boneDim.darker(140), 1.0);
        blob(p, QPointF(pr.radius * 0.72f, -pr.radius * 0.98f), pr.radius * 0.13f, pr.radius * 0.15f, QColor(20, 16, 18));
        blob(p, QPointF(pr.radius * 0.96f, -pr.radius * 0.98f), pr.radius * 0.13f, pr.radius * 0.15f, QColor(20, 16, 18));
        break;
    }
    case Pk_Pillar: {
        QRectF body(-pr.radius * 0.85f, -h, pr.radius * 1.7f, h);
        QLinearGradient g(body.topLeft(), body.topRight());
        g.setColorAt(0, pal.wallTop.darker(140));
        g.setColorAt(0.35, pal.wallTop);
        g.setColorAt(1, pal.wallDark);
        p->setBrush(g);
        p->setPen(QPen(pal.wallDark, 1.4));
        p->drawRect(body);
        p->drawRect(QRectF(-pr.radius * 1.1f, -h - 8 * s, pr.radius * 2.2f, 10 * s));
        p->drawRect(QRectF(-pr.radius * 1.1f, -10 * s, pr.radius * 2.2f, 10 * s));
        p->setPen(QPen(pal.wallDark.darker(120), 1.0));
        for (int i = 1; i < 4; ++i)
            p->drawLine(QPointF(-pr.radius * 0.85f + i * pr.radius * 0.42f, -h), QPointF(-pr.radius * 0.85f + i * pr.radius * 0.42f, 0));
        // трещина
        p->setPen(QPen(QColor(0, 0, 0, 120), 1.6));
        p->drawLine(QPointF(-pr.radius * 0.2f, -h * 0.85f), QPointF(pr.radius * 0.3f, -h * 0.5f));
        break;
    }
    case Pk_Bush: {
        for (int i = 0; i < 3; ++i) {
            blob(p, QPointF((i - 1) * pr.radius * 0.6f, -pr.radius * (0.3f + (i == 1 ? 0.25f : 0.f))),
                 pr.radius * 0.75f, pr.radius * 0.6f, pal.floor3.darker(120), pal.wallDark, 1.0);
        }
        break;
    }
    case Pk_Mushroom: {
        for (int i = 0; i < 3; ++i) {
            float dx = (i - 1) * pr.radius * 0.7f;
            float hh = pr.height * (0.6f + r.unit() * 0.6f);
            p->setPen(QPen(C::boneDim, pr.radius * 0.18f, Qt::SolidLine, Qt::RoundCap));
            p->drawLine(QPointF(dx, 0), QPointF(dx, -hh * 0.6f));
            QColor cap(150, 220, 130);
            if (biome == Bio_Swamp) cap = QColor(170, 230, 120);
            blob(p, QPointF(dx, -hh * 0.62f), pr.radius * 0.55f, pr.radius * 0.38f, cap, cap.darker(180), 1.0);
            Style::glow(p, QPointF(dx, -hh * 0.62f), pr.radius * 1.6f, cap, 0.28f + 0.12f * float(qSin(t * 2.5f + i)));
        }
        break;
    }
    case Pk_Ruin: {
        for (int i = 0; i < 3; ++i) {
            float bw = pr.radius * (1.2f - i * 0.2f);
            float bh = pr.height * (0.42f - i * 0.10f);
            QRectF rr(-bw * 0.5f + i * pr.radius * 0.3f, -bh - i * pr.height * 0.28f, bw, bh);
            p->setBrush(pal.wallTop.darker(130 + i * 12));
            p->setPen(QPen(pal.wallDark, 1.2));
            p->drawRect(rr);
        }
        break;
    }
    case Pk_Brazier: {
        p->setPen(QPen(QColor(60, 50, 42), pr.radius * 0.28f, Qt::SolidLine, Qt::RoundCap));
        p->drawLine(QPointF(0, 0), QPointF(0, -h * 0.62f));
        polyf(p, QVector<QPointF>() << QPointF(-pr.radius * 0.85f, -h * 0.62f) << QPointF(pr.radius * 0.85f, -h * 0.62f)
              << QPointF(pr.radius * 0.5f, -h * 0.85f) << QPointF(-pr.radius * 0.5f, -h * 0.85f),
              QColor(88, 74, 60), QColor(30, 24, 20), 1.2);
        float fl = 0.7f + 0.3f * qSin(t * 9.f + pr.seed) + 0.12f * qSin(t * 23.f + pr.seed * 3);
        Style::glow(p, QPointF(0, -h * 0.95f), pr.radius * 5.5f * fl, QColor(255, 160, 60), 0.55f);
        for (int i = 0; i < 4; ++i) {
            float fh = h * (0.34f + 0.24f * fl) * (1.f - i * 0.19f);
            float wx = qSin(t * (6.f + i * 2.3f) + i * 1.7f + pr.seed) * pr.radius * (0.10f + i * 0.05f);
            QColor fc = i == 0 ? QColor(214, 62, 16)
                      : (i == 1 ? QColor(255, 122, 30) : (i == 2 ? QColor(255, 190, 80) : QColor(255, 246, 200)));
            fc.setAlpha(i == 3 ? 200 : 235);
            QPolygonF pg;
            pg << QPointF(-pr.radius * 0.36f * (1.f - i * 0.18f), -h * 0.80f)
               << QPointF(wx - pr.radius * 0.10f, -h * 0.80f - fh * 0.55f)
               << QPointF(wx, -h * 0.80f - fh)
               << QPointF(wx + pr.radius * 0.10f, -h * 0.80f - fh * 0.55f)
               << QPointF(pr.radius * 0.36f * (1.f - i * 0.18f), -h * 0.80f);
            p->setBrush(fc);
            p->setPen(Qt::NoPen);
            p->drawPolygon(pg);
        }
        // искры
        p->setCompositionMode(QPainter::CompositionMode_Plus);
        for (int i = 0; i < 3; ++i) {
            float ph = fmodf(t * 1.3f + i * 0.37f + (pr.seed % 5) * 0.11f, 1.f);
            float sx = qSin(ph * 9.f + i * 2.f) * pr.radius * 0.5f;
            p->setBrush(QColor(255, 200, 90, int(190 * (1.f - ph))));
            p->drawEllipse(QPointF(sx, -h * 0.9f - ph * h * 1.4f), 1.6f * (1.f - ph), 1.6f * (1.f - ph));
        }
        p->setCompositionMode(QPainter::CompositionMode_SourceOver);
        break;
    }
    case Pk_Statue: {
        p->setBrush(pal.wallTop.darker(140));
        p->setPen(QPen(pal.wallDark, 1.3));
        p->drawRect(QRectF(-pr.radius * 1.1f, -10 * s, pr.radius * 2.2f, 10 * s));
        polyf(p, QVector<QPointF>() << QPointF(-pr.radius * 0.6f, -10 * s) << QPointF(-pr.radius * 0.45f, -h * 0.72f)
              << QPointF(pr.radius * 0.45f, -h * 0.72f) << QPointF(pr.radius * 0.6f, -10 * s),
              pal.wallTop.darker(120), pal.wallDark, 1.2);
        blob(p, QPointF(0, -h * 0.82f), pr.radius * 0.32f, pr.radius * 0.34f, pal.wallTop.darker(110), pal.wallDark, 1.2);
        glowDot(p, QPointF(0, -h * 0.82f), pr.radius * 0.10f, pal.accent, 0.5f + 0.3f * float(qSin(t * 1.4f)));
        break;
    }
    case Pk_Stump: {
        p->setBrush(QColor(70, 52, 38));
        p->setPen(QPen(QColor(36, 26, 20), 1.3));
        p->drawRect(QRectF(-pr.radius * 0.8f, -pr.height, pr.radius * 1.6f, pr.height));
        blob(p, QPointF(0, -pr.height), pr.radius * 0.8f, pr.radius * 0.34f, QColor(126, 96, 66), QColor(50, 36, 26), 1.2);
        p->setPen(QPen(QColor(90, 68, 48), 1.0));
        p->drawEllipse(QPointF(0, -pr.height), pr.radius * 0.45f, pr.radius * 0.19f);
        break;
    }
    case Pk_IceSpike: {
        for (int i = 0; i < 3; ++i) {
            float dx = (i - 1) * pr.radius * 0.6f;
            float hh = pr.height * (0.5f + r.unit() * 0.7f);
            QPolygonF pg;
            pg << QPointF(dx, -hh) << QPointF(dx + pr.radius * 0.34f, 0) << QPointF(dx - pr.radius * 0.34f, 0);
            QColor ic(190, 226, 250, 220);
            p->setBrush(ic);
            p->setPen(QPen(QColor(240, 250, 255, 200), 1.2));
            p->drawPolygon(pg);
            p->setPen(QPen(QColor(255, 255, 255, 140), 1.0));
            p->drawLine(QPointF(dx - pr.radius * 0.1f, -hh * 0.8f), QPointF(dx - pr.radius * 0.16f, -hh * 0.15f));
        }
        break;
    }
    case Pk_Vent: {
        blob(p, QPointF(0, 0), pr.radius, pr.radius * 0.42f, QColor(30, 18, 16), QColor(10, 6, 6), 1.4);
        blob(p, QPointF(0, 0), pr.radius * 0.62f, pr.radius * 0.26f, QColor(255, 120, 30));
        Style::glow(p, QPointF(0, -pr.radius * 0.2f), pr.radius * 4.f, QColor(255, 130, 40), 0.5f + 0.25f * float(qSin(t * 4.f + pr.seed)));
        for (int i = 0; i < 3; ++i) {
            float ph = fmodf(t * 1.6f + i * 0.7f, 1.f);
            blob(p, QPointF(qSin(ph * 8.f + i) * pr.radius * 0.4f, -ph * pr.height * 3.f),
                 pr.radius * 0.2f * (1.f - ph), pr.radius * 0.2f * (1.f - ph), QColor(255, 170, 70, int(200 * (1.f - ph))));
        }
        break;
    }
    case Pk_Obelisk: {
        QPolygonF pg;
        pg << QPointF(0, -h) << QPointF(pr.radius * 0.62f, -h * 0.86f) << QPointF(pr.radius * 0.75f, 0)
           << QPointF(-pr.radius * 0.75f, 0) << QPointF(-pr.radius * 0.62f, -h * 0.86f);
        p->setBrush(pal.wall.darker(120));
        p->setPen(QPen(pal.wallDark, 1.4));
        p->drawPolygon(pg);
        Style::glow(p, QPointF(0, -h * 0.55f), pr.radius * 3.f, pal.accent, 0.28f + 0.14f * float(qSin(t * 1.8f)));
        p->setPen(QPen(pal.accent, 1.6));
        for (int i = 0; i < 4; ++i) {
            float y = -h * (0.30f + i * 0.16f);
            p->drawLine(QPointF(-pr.radius * 0.34f, y), QPointF(pr.radius * 0.34f, y));
            p->drawLine(QPointF(0, y), QPointF(0, y - h * 0.07f));
        }
        break;
    }
    case Pk_House: {
        float w2 = pr.radius * 1.5f;
        QRectF body(-w2, -h * 0.62f, w2 * 2, h * 0.62f);
        p->setBrush(QColor(96, 78, 60));
        p->setPen(QPen(QColor(38, 30, 24), 1.6));
        p->drawRect(body);
        QPolygonF roof;
        roof << QPointF(-w2 * 1.18f, body.top()) << QPointF(0, body.top() - h * 0.42f) << QPointF(w2 * 1.18f, body.top());
        p->setBrush(QColor(126, 62, 48));
        p->drawPolygon(roof);
        p->setPen(QPen(QColor(60, 30, 24), 1.2));
        p->drawLine(roof.at(0), roof.at(2));
        // окно со светом
        QRectF win(-w2 * 0.42f, body.top() + h * 0.14f, w2 * 0.42f, h * 0.20f);
        p->fillRect(win, QColor(255, 200, 110));
        Style::glow(p, win.center(), w2 * 1.6f, QColor(255, 190, 100), 0.35f);
        p->setPen(QPen(QColor(60, 40, 26), 1.4));
        p->drawRect(win);
        QRectF win2(w2 * 0.10f, body.top() + h * 0.14f, w2 * 0.42f, h * 0.20f);
        p->fillRect(win2, QColor(255, 190, 90));
        p->drawRect(win2);
        // дверь
        p->fillRect(QRectF(-w2 * 0.16f, body.bottom() - h * 0.30f, w2 * 0.32f, h * 0.30f), QColor(58, 42, 32));
        p->drawRect(QRectF(-w2 * 0.16f, body.bottom() - h * 0.30f, w2 * 0.32f, h * 0.30f));
        break;
    }
    case Pk_Stall: {
        float w2 = pr.radius * 1.3f;
        p->setPen(QPen(QColor(70, 52, 38), 4.0));
        p->drawLine(QPointF(-w2, 0), QPointF(-w2, -h * 0.9f));
        p->drawLine(QPointF(w2, 0), QPointF(w2, -h * 0.9f));
        p->setBrush(QColor(120, 92, 62));
        p->setPen(QPen(QColor(46, 34, 24), 1.3));
        p->drawRect(QRectF(-w2, -h * 0.42f, w2 * 2, h * 0.14f));
        // навес в полоску
        for (int i = 0; i < 6; ++i) {
            p->setBrush(i % 2 ? QColor(176, 62, 52) : QColor(226, 210, 176));
            p->drawRect(QRectF(-w2 + i * w2 * 2 / 6.f, -h * 0.92f, w2 * 2 / 6.f, h * 0.20f));
        }
        p->setPen(QPen(QColor(46, 34, 24), 1.2));
        p->drawRect(QRectF(-w2, -h * 0.92f, w2 * 2, h * 0.20f));
        // товары
        for (int i = 0; i < 3; ++i) {
            drawGlyph(p, Gl_PotionHp + i, QRectF(-w2 * 0.7f + i * w2 * 0.55f, -h * 0.40f, w2 * 0.34f, w2 * 0.34f),
                      i == 0 ? C::hpHi : (i == 1 ? C::manaHi : C::gold));
        }
        break;
    }
    case Pk_Barrel: {
        QRectF b(-pr.radius * 0.7f, -pr.height, pr.radius * 1.4f, pr.height);
        p->setBrush(QColor(112, 82, 52));
        p->setPen(QPen(QColor(46, 32, 22), 1.4));
        p->drawRect(b);
        p->setPen(QPen(QColor(70, 56, 44), 2.4));
        p->drawLine(QPointF(b.left(), b.top() + b.height() * 0.25f), QPointF(b.right(), b.top() + b.height() * 0.25f));
        p->drawLine(QPointF(b.left(), b.top() + b.height() * 0.75f), QPointF(b.right(), b.top() + b.height() * 0.75f));
        blob(p, QPointF(0, b.top()), pr.radius * 0.7f, pr.radius * 0.28f, QColor(140, 104, 66), QColor(46, 32, 22), 1.2);
        break;
    }
    case Pk_Crate: {
        QRectF b(-pr.radius, -pr.height, pr.radius * 2, pr.height);
        p->setBrush(QColor(126, 98, 62));
        p->setPen(QPen(QColor(48, 34, 24), 1.4));
        p->drawRect(b);
        p->drawLine(b.topLeft(), b.bottomRight());
        p->drawLine(b.topRight(), b.bottomLeft());
        break;
    }
    case Pk_Chest:
    case Pk_ChestOpen: {
        bool open = (pr.kind == Pk_ChestOpen);
        float w = pr.radius * 1.5f;
        // корпус
        p->setBrush(QColor(96, 68, 42));
        p->setPen(QPen(QColor(40, 26, 18), 1.6));
        p->drawRect(QRectF(-w, -pr.height * 0.8f, w * 2, pr.height * 0.8f));
        p->setPen(QPen(QColor(60, 44, 30), 1.4));
        p->drawLine(QPointF(-w, -pr.height * 0.4f), QPointF(w, -pr.height * 0.4f));
        // оковка
        p->setPen(QPen(QColor(120, 108, 92), 2.4));
        p->drawLine(QPointF(-w * 0.45f, -pr.height * 0.8f), QPointF(-w * 0.45f, 0));
        p->drawLine(QPointF(w * 0.45f, -pr.height * 0.8f), QPointF(w * 0.45f, 0));
        // крышка
        p->save();
        p->translate(0, -pr.height * 0.8f);
        p->rotate(open ? -105.f : 0.f);
        p->setBrush(QColor(112, 80, 50));
        p->setPen(QPen(QColor(40, 26, 18), 1.6));
        QPainterPath lid;
        lid.moveTo(QPointF(-w, 0));
        lid.cubicTo(QPointF(-w, -pr.height * 0.55f), QPointF(w, -pr.height * 0.55f), QPointF(w, 0));
        lid.closeSubpath();
        p->drawPath(lid);
        p->setPen(QPen(QColor(120, 108, 92), 2.2));
        p->drawLine(QPointF(-w * 0.45f, 0), QPointF(-w * 0.45f, -pr.height * 0.42f));
        p->drawLine(QPointF(w * 0.45f, 0), QPointF(w * 0.45f, -pr.height * 0.42f));
        p->restore();
        if (open) {
            p->setBrush(QColor(10, 7, 12));
            p->setPen(Qt::NoPen);
            p->drawRect(QRectF(-w * 0.86f, -pr.height * 0.78f, w * 1.72f, pr.height * 0.30f));
            Style::glow(p, QPointF(0, -pr.height * 0.7f), pr.radius * 1.6f, C::gold, 0.18f + 0.08f * float(qSin(t * 2.f)));
        } else {
            // замок светится, намекая на содержимое
            glowDot(p, QPointF(0, -pr.height * 0.62f), pr.radius * 0.16f, C::gold, 0.7f + 0.3f * float(qSin(t * 2.6f + pr.seed)));
        }
        break;
    }
    case Pk_Altar: {
        Style::glow(p, QPointF(0, -pr.height * 0.9f), pr.radius * 5.f, pal.accent, 0.32f + 0.14f * float(qSin(t * 1.6f)));
        p->setBrush(pal.wallTop.darker(130));
        p->setPen(QPen(pal.wallDark, 1.4));
        p->drawRect(QRectF(-pr.radius * 1.2f, -pr.height * 0.35f, pr.radius * 2.4f, pr.height * 0.35f));
        p->drawRect(QRectF(-pr.radius * 0.85f, -pr.height * 0.85f, pr.radius * 1.7f, pr.height * 0.5f));
        p->drawRect(QRectF(-pr.radius * 1.05f, -pr.height, pr.radius * 2.1f, pr.height * 0.16f));
        p->setPen(QPen(pal.accent, 2.0));
        p->setBrush(Qt::NoBrush);
        p->drawEllipse(QPointF(0, -pr.height * 1.25f), pr.radius * 0.42f, pr.radius * 0.42f);
        p->drawLine(QPointF(0, -pr.height * 1.7f), QPointF(0, -pr.height * 0.85f));
        p->drawLine(QPointF(-pr.radius * 0.42f, -pr.height * 1.25f), QPointF(pr.radius * 0.42f, -pr.height * 1.25f));
        break;
    }
    default: break;
    }
    p->restore();
}

// ============================================================
//  ПРЕДМЕТЫ НА ЗЕМЛЕ / ЗОЛОТО / NPC
// ============================================================
void drawGroundItem(QPainter *p, const Item &it, const QPointF &pos, float t, Rarity rar, float pop)
{
    const ItemBaseDef *b = Data::base(it.baseId);
    if (!b) return;
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    float pk = easeOutBack(clampf(pop, 0.f, 1.f));
    p->translate(pos);
    p->scale(qMax(0.05f, pk), qMax(0.05f, pk));
    p->translate(-pos);
    float bob = qSin(t * 2.6f + pos.x() * 0.01f) * 3.0f;
    QColor rc = rarityColor(rar);
    Style::glow(p, pos + QPointF(0, -8 + bob), 26.f, rc, 0.45f);
    // тень
    p->setPen(Qt::NoPen);
    QRadialGradient g(pos, 14.f);
    g.setColorAt(0, QColor(0, 0, 0, 110));
    g.setColorAt(1, QColor(0, 0, 0, 0));
    p->setBrush(g);
    p->save();
    p->translate(pos); p->scale(1, 0.4); p->translate(-pos);
    p->drawEllipse(pos, 14.f, 14.f);
    p->restore();

    // луч света над ценной добычей
    if (rar >= Rar_Rare) {
        float pulse = 0.55f + 0.45f * qSin(t * 2.2f + pos.x() * 0.05f);
        QLinearGradient beam(QPointF(0, pos.y() - 130), QPointF(0, pos.y()));
        QColor bc = rarityColor(rar);
        bc.setAlphaF(0.30f * pulse);
        beam.setColorAt(0, QColor(bc.red(), bc.green(), bc.blue(), 0));
        beam.setColorAt(1, bc);
        p->save();
        p->setCompositionMode(QPainter::CompositionMode_Plus);
        p->setPen(Qt::NoPen);
        p->setBrush(beam);
        QPolygonF bp;
        bp << QPointF(pos.x() - 4, pos.y() - 6) << QPointF(pos.x() - 9, pos.y() - 130)
           << QPointF(pos.x() + 9, pos.y() - 130) << QPointF(pos.x() + 4, pos.y() - 6);
        p->drawPolygon(bp);
        p->restore();
    }
    QRectF r(pos.x() - 11, pos.y() - 26 + bob, 22, 22);
    p->setBrush(QColor(16, 13, 22, 190));
    p->setPen(QPen(rc, 1.6));
    p->drawRect(r);
    drawGlyph(p, b->glyph, r.adjusted(2, 2, -2, -2), rc.lighter(120), rc);
    if (it.count > 1) {
        p->setFont(fontNum(9, true));
        p->setPen(QColor(0, 0, 0, 200));
        p->drawText(QPointF(r.right() - 9, r.bottom() + 1), QString::number(it.count));
        p->setPen(C::bone);
        p->drawText(QPointF(r.right() - 10, r.bottom()), QString::number(it.count));
    }
    p->restore();
}

void drawGoldPile(QPainter *p, const QPointF &pos, float t, int amount)
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    float bob = qSin(t * 3.f + pos.y() * 0.02f) * 2.f;
    Style::glow(p, pos + QPointF(0, -5 + bob), 22.f, C::gold, 0.5f);
    int n = qBound(3, amount / 12, 8);
    for (int i = 0; i < n; ++i) {
        float ang = i * 1.7f;
        QPointF c(pos.x() + qCos(ang) * 6.f, pos.y() - 4 + qSin(ang) * 3.5f + bob);
        blob(p, c, 4.2f, 3.4f, C::gold, QColor(120, 86, 20), 0.9);
    }
    p->restore();
}

void drawNpc(QPainter *p, const Npc &n, float t)
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    drawShadow(p, n.pos, 15.f, 6.f, 0.42f);
    p->translate(n.pos);
    float bob = qSin(t * 1.6f) * 1.2f;

    // опциональный спрайт-лист: «npc<вид>», запасной вариант - общий «npc»
    {
        const SprBank::Sheet *sh = SprBank::get(QStringLiteral("npc%1").arg(n.kind));
        if (!sh) sh = SprBank::get(QStringLiteral("npc"));
        if (sh && sh->valid && !sh->anims.isEmpty()) {
            QString an = sh->anims.contains(QStringLiteral("idle"))
                       ? QStringLiteral("idle") : sh->anims.keys().first();
            const SprBank::Anim am = sh->anims.value(an);
            int cnt = am.to - am.from + 1;
            if (cnt > 0) {
                int fr = am.from + (int(n.animT * sh->fps) % cnt);
                int cols = qMax(1, sh->img.width() / sh->fw);
                QRectF sr((fr % cols) * sh->fw, (fr / cols) * sh->fh, sh->fw, sh->fh);
                float k = 60.f / float(qMax(8, sh->ay));   // рост ~60px по точке ног
                p->save();
                p->translate(0, bob);
                p->scale(k, k);
                p->setRenderHint(QPainter::SmoothPixmapTransform, true);
                p->drawImage(QRectF(-sh->ax, -sh->ay, sh->fw, sh->fh), sh->img, sr);
                p->restore();
                // значок над головой: торговец/кузнец/целительница/старейшина
                int gl = n.kind == 0 ? Gl_Coins : (n.kind == 1 ? Gl_Hammer : (n.kind == 2 ? Gl_Heart : Gl_Info));
                float pulse = 0.6f + 0.4f * qSin(t * 2.6f);
                float topY = -float(sh->ay) * k;
                drawGlyph(p, gl, QRectF(-9, topY - 20.f - pulse * 3.f, 18, 18), C::amber, C::gold, 0.5f);
                p->restore();
                return;
            }
        }
    }

    QColor robe = n.kind == 0 ? QColor(120, 92, 150) : (n.kind == 1 ? QColor(140, 96, 60) : QColor(90, 130, 110));
    // подол
    polyf(p, QVector<QPointF>() << QPointF(-13, 0) << QPointF(-10, -30 - bob) << QPointF(10, -30 - bob)
          << QPointF(13, 0), robe.darker(140), robe.darker(220), 1.2);
    // тело
    polyf(p, QVector<QPointF>() << QPointF(-9, -30 - bob) << QPointF(-8, -46 - bob) << QPointF(8, -46 - bob)
          << QPointF(9, -30 - bob), robe, robe.darker(220), 1.2);
    // голова
    blob(p, QPointF(0, -54 - bob), 8.5f, 9.f, QColor(226, 196, 164), QColor(90, 60, 48), 1.0);
    // борода / капюшон
    if (n.kind == 1) {
        blob(p, QPointF(0, -48 - bob), 7.f, 6.f, QColor(190, 186, 178), QColor(120, 116, 110), 0.9);
    } else {
        QPainterPath h;
        h.moveTo(QPointF(-10, -52 - bob));
        h.cubicTo(QPointF(-10, -68 - bob), QPointF(10, -68 - bob), QPointF(10, -52 - bob));
        p->setBrush(robe.darker(170));
        p->setPen(QPen(robe.darker(240), 1.0));
        p->drawPath(h);
    }
    blob(p, QPointF(-3, -55 - bob), 1.4f, 1.6f, QColor(30, 24, 28));
    blob(p, QPointF(3, -55 - bob), 1.4f, 1.6f, QColor(30, 24, 28));
    // значок над головой
    int gl = n.kind == 0 ? Gl_Coins : (n.kind == 1 ? Gl_Hammer : (n.kind == 2 ? Gl_Heart : Gl_Info));
    float pulse = 0.6f + 0.4f * qSin(t * 2.6f);
    drawGlyph(p, gl, QRectF(-9, -84 - bob - pulse * 3.f, 18, 18), C::amber, C::gold, 0.5f);
    p->restore();
}

// ============================================================
//  ЭФФЕКТЫ
// ============================================================

// ---- тонировка кадров fx-листов ----
// Листы эффектов рисуются в градациях серого; цвет задаётся в рантайме
// (по типу урона). Ядро пикселя уходит в белый, середина - в цвет.
static QImage tintFrame(const QImage &sheet, const QRect &sr, const QColor &c)
{
    quint64 key = (quint64(sheet.cacheKey()) * 1000003ull)
                ^ (quint64(sr.x() * 4096 + sr.y()) * 97ull)
                ^ quint64(c.rgb());
    static QHash<quint64, QImage> cache;
    QHash<quint64, QImage>::const_iterator hit = cache.constFind(key);
    if (hit != cache.constEnd()) return hit.value();
    QImage fr = sheet.copy(sr).convertToFormat(QImage::Format_ARGB32);
    int cr = c.red(), cg = c.green(), cb = c.blue();
    for (int y = 0; y < fr.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(fr.scanLine(y));
        for (int x = 0; x < fr.width(); ++x) {
            QRgb px = line[x];
            int a = qAlpha(px);
            if (a == 0) continue;
            int lum = (qRed(px) * 77 + qGreen(px) * 150 + qBlue(px) * 29) >> 8;
            float t = lum / 255.f;
            float tt = 0.45f + 0.55f * t;              // тело не проваливается в темноту
            float w = t * t * t * t * t * 0.6f;       // только ядро уходит в белый
            int rr = int(cr * tt + (255.f - cr * tt) * w);
            int gg = int(cg * tt + (255.f - cg * tt) * w);
            int bb = int(cb * tt + (255.f - cb * tt) * w);
            line[x] = qRgba(qBound(0, rr, 255), qBound(0, gg, 255), qBound(0, bb, 255),
                            qBound(0, int(a * (0.62f + 0.38f * t)), 255));
        }
    }
    if (cache.size() > 900) cache.clear();
    cache.insert(key, fr);
    return fr;
}

// один кадр листовой анимации fx_*: центр в (0,0) текущей системы координат
static bool drawFxFrame(QPainter *p, const SprBank::Sheet *sh, const SprBank::Anim &am,
                        int frameIdx, float sizePx, const QColor &c)
{
    int n = am.to - am.from + 1;
    if (n <= 0) return false;
    int fr = am.from + qBound(0, frameIdx, n - 1);
    int cols = qMax(1, sh->img.width() / sh->fw);
    QRect sr((fr % cols) * sh->fw, (fr / cols) * sh->fh, sh->fw, sh->fh);
    QImage img = tintFrame(sh->img, sr, c);
    float k = sizePx / float(sh->fw);
    p->setRenderHint(QPainter::SmoothPixmapTransform, true);
    p->drawImage(QRectF(-sh->ax * k, -sh->ay * k, sh->fw * k, sh->fh * k), img);
    return true;
}

// Зацикленная анимация снаряда из fx-листа. false = листа нет, рисуем процедурно.
static bool projectileSheet(QPainter *p, const Proj &pr, float t, const QColor &col)
{
    const char *key = 0;
    switch (pr.kind) {
    case 0: key = "fx_arrow"; break;
    case 1: key = "fx_bolt";  break;
    case 2: key = "fx_shard"; break;
    case 3: key = "fx_zap";   break;
    case 4: key = "fx_gob";   break;
    case 5: key = "fx_bolt";  break;
    case 7: key = "fx_soul";  break;
    default: key = "fx_rock"; break;
    }
    const SprBank::Sheet *sh = SprBank::get(QLatin1String(key));
    if (!sh || !sh->valid || !sh->anims.contains(QLatin1String("loop"))) return false;
    const SprBank::Anim am = sh->anims.value(QLatin1String("loop"));
    int n = am.to - am.from + 1;
    int seed = (pr.id * 37 + int(pr.startPos.x()) * 7 + int(pr.startPos.y()) * 13) & 0x3FFF;
    int fi = ((int(t * sh->fps) + seed) % n + n) % n;

    float sz;
    switch (pr.kind) {
    case 0: sz = 46.f * pr.scale; break;                                   // стрела с трейлом
    case 2: sz = qMax(30.f, pr.radius * 4.2f) * pr.scale; break;           // осколок
    case 3: sz = qMax(32.f, pr.radius * 4.5f) * pr.scale; break;           // молния
    case 7: sz = qMax(30.f, pr.radius * 5.0f) * pr.scale; break;           // душа
    case 4: sz = qMax(28.f, pr.radius * 5.0f) * pr.scale; break;           // капля яда
    case 6: sz = qMax(30.f, pr.radius * 4.5f) * pr.scale; break;           // камень
    default: sz = qMax(34.f, pr.radius * 4.8f) * pr.scale; break;          // сфера огня/тайны
    }

    // след-полоса за снарядом (в любую погоду, со спрайтом и без)
    float ang = qAtan2(pr.vel.y(), pr.vel.x());
    p->save();
    p->rotate(qRadiansToDegrees(ang));
    float tl = sz * (pr.kind == 0 ? 1.7f : 1.15f);
    QLinearGradient g(-tl, 0, sz * 0.2f, 0);
    QColor tc = col; tc.setAlpha(0);
    QColor tc2 = col; tc2.setAlpha(pr.kind == 0 ? 95 : 70);
    g.setColorAt(0, tc);
    g.setColorAt(1, tc2);
    p->setPen(Qt::NoPen);
    p->setBrush(g);
    float th = qMax(1.6f, sz * 0.07f);
    p->drawRect(QRectF(-tl, -th, tl + sz * 0.2f, th * 2));
    p->restore();

    Style::glow(p, QPointF(0, 0), sz * (pr.kind == 1 || pr.kind == 5 ? 0.85f : 0.55f), col,
                pr.kind == 1 ? 0.55f : 0.3f);

    float deg = 0.f;
    if (pr.kind == 0 || pr.kind == 2 || pr.kind == 3) deg = qRadiansToDegrees(ang);
    p->save();
    if (deg != 0.f) p->rotate(deg);
    drawFxFrame(p, sh, am, fi, sz, col);
    p->restore();
    return true;
}

// ---- процедурные росчерки (фолбэк и основа эффектов) ----
static void fxArcSlash(QPainter *p, float R, float k, const QColor &col, int kind)
{
    float grow = qMin(1.f, k * 2.4f);                    // дуга выметается вперёд
    float fade = 1.f - qPow(k, 2.4f);
    if (kind == 4) {
        // выпад: удлинённый узкий след колющего удара
        float L = R * (0.55f + 0.85f * grow);
        QPolygonF pg;
        pg << QPointF(L, 0) << QPointF(L * 0.25f, -R * 0.14f) << QPointF(-L * 0.45f, 0)
           << QPointF(L * 0.25f, R * 0.14f);
        QColor c0 = col; c0.setAlphaF(0.8f * fade);
        p->setPen(Qt::NoPen);
        p->setBrush(c0);
        p->drawPolygon(pg);
        QPolygonF in;
        in << QPointF(L * 0.9f, 0) << QPointF(L * 0.3f, -R * 0.045f) << QPointF(-L * 0.2f, 0)
           << QPointF(L * 0.3f, R * 0.045f);
        p->setBrush(QColor(255, 252, 240, int(215 * fade)));
        p->drawPolygon(in);
        return;
    }
    float a0 = -1.05f;
    float span = (kind == 1 ? 2.45f : 2.05f) * grow;
    for (int pass = 0; pass < 3; ++pass) {
        float rr = R * (pass == 0 ? 1.f : (pass == 1 ? 0.84f : 0.94f));
        float w  = (pass == 0 ? R * 0.30f : (pass == 1 ? R * 0.15f : R * 0.05f));
        QColor c = (pass == 0) ? col : (pass == 1 ? col.lighter(145) : QColor(255, 252, 240));
        c.setAlphaF(fade * (pass == 0 ? 0.42f : (pass == 1 ? 0.7f : 0.95f)));
        p->setPen(QPen(c, w, Qt::SolidLine, Qt::RoundCap));
        p->setBrush(Qt::NoBrush);
        p->drawArc(QRectF(-rr, -rr, rr * 2, rr * 2),
                   int(a0 * (180.f / float(M_PI)) * 16.f), int(span * (180.f / float(M_PI)) * 16.f));
    }
    if (kind == 1) {
        // секира: вторая, внутренняя дуга
        float rr = R * 0.68f;
        QColor c = col.lighter(125); c.setAlphaF(fade * 0.5f);
        p->setPen(QPen(c, R * 0.13f, Qt::SolidLine, Qt::RoundCap));
        p->drawArc(QRectF(-rr, -rr, rr * 2, rr * 2),
                   int((a0 + 0.22f) * (180.f / float(M_PI)) * 16.f),
                   int(span * 0.8f * (180.f / float(M_PI)) * 16.f));
    }
    if (kind == 3) {
        // магия: искры вдоль дуги
        for (int i = 0; i < 7; ++i) {
            float a = a0 + span * (float(i) / 6.f);
            float rr = R * (0.92f + 0.16f * qSin(float(i) * 2.3f));
            QPointF q(qCos(a) * rr, qSin(a) * rr);
            QColor w = col.lighter(160); w.setAlphaF(fade * 0.9f);
            p->setPen(Qt::NoPen);
            p->setBrush(w);
            p->drawEllipse(q, 1.8f + fade, 1.8f + fade);
        }
    }
}

static void fxClaw(QPainter *p, float R, float k, const QColor &col, int seed)
{
    float grow = qMin(1.f, k * 2.8f);
    float fade = 1.f - qPow(k, 1.9f);
    for (int i = 0; i < 3; ++i) {
        float off = (i - 1) * R * 0.30f + float((seed + i * 3) % 3 - 1) * R * 0.05f;
        float len = R * (0.8f + 0.12f * float((seed + i * 7) % 3)) * grow;
        QPointF a(-len * 0.5f, off - R * 0.20f);
        QPointF b(len * 0.55f, off + R * 0.20f);
        QPointF mid((a.x() + b.x()) * 0.5f + R * 0.08f, (a.y() + b.y()) * 0.5f - R * 0.12f);
        QPainterPath pa;
        pa.moveTo(a);
        pa.quadTo(mid, b);
        QColor c = col.lighter(110 + i * 18);
        c.setAlphaF(fade * 0.85f);
        p->setPen(QPen(c, R * 0.10f * (1.f - 0.16f * float(i)), Qt::SolidLine, Qt::RoundCap));
        p->setBrush(Qt::NoBrush);
        p->drawPath(pa);
        p->setPen(QPen(QColor(255, 250, 240, int(200 * fade)), R * 0.032f, Qt::SolidLine, Qt::RoundCap));
        p->drawPath(pa);
    }
}

static void fxSmash(QPainter *p, float R, float k, const QColor &col, int seed)
{
    float rr = R * (0.32f + 0.78f * k);
    float fade = 1.f - k;
    p->save();
    p->scale(1.f, 0.45f);                                // кольцо на земле
    QColor c = col; c.setAlphaF(0.8f * fade);
    p->setPen(QPen(c, qMax(2.f, R * 0.15f * fade + 1.f)));
    p->setBrush(Qt::NoBrush);
    p->drawEllipse(QPointF(0, 0), rr, rr);
    p->setPen(QPen(QColor(255, 250, 235, int(150 * fade)), qMax(1.f, R * 0.045f)));
    p->drawEllipse(QPointF(0, 0), rr * 0.86f, rr * 0.86f);
    p->restore();
    for (int i = 0; i < 7; ++i) {                        // разлёт осколков
        float a = float(seed) * 0.7f + float(i) * (2.f * float(M_PI) / 7.f);
        float d = rr * (0.9f + 0.3f * float((seed + i * 5) % 3) / 3.f);
        QPointF q(qCos(a) * d, qSin(a) * d * 0.45f - R * 0.3f * fade);
        QColor c2 = col.lighter(135); c2.setAlphaF(fade * 0.8f);
        p->setPen(Qt::NoPen);
        p->setBrush(c2);
        float s = 2.f + 2.f * fade;
        p->drawRect(QRectF(q.x() - s * 0.5f, q.y() - s * 0.5f, s, s));
    }
}

static void fxCast(QPainter *p, float R, float k, const QColor &col, int seed, float time)
{
    float fade = 1.f - k * k;
    float rr = R * (0.45f + 0.6f * qMin(1.f, k * 1.9f));
    p->save();
    p->scale(1.f, 0.5f);                                 // круг на земле
    QColor c = col; c.setAlphaF(0.85f * fade);
    p->setPen(QPen(c, 2.2));
    p->setBrush(Qt::NoBrush);
    p->drawPath(runeRing(QPointF(0, 0), rr, 8 + (seed % 4), rr * 0.16f, time * 2.4f + float(seed)));
    QColor c2 = col; c2.setAlphaF(0.5f * fade);
    p->setPen(QPen(c2, 1.3));
    p->drawEllipse(QPointF(0, 0), rr * 0.55f, rr * 0.55f);
    p->restore();
    Style::glow(p, QPointF(0, -R * 0.2f), R * 0.8f, col, 0.5f * fade);
    for (int i = 0; i < 5; ++i) {                        // искры вверх
        float ph = k * 2.f + float(i) * 0.37f + float(seed) * 0.11f;
        float yy = -R * 1.0f * (ph - float(int(ph)));
        float xx = qSin(ph * 5.f + float(i)) * R * 0.3f;
        QColor w = col.lighter(150); w.setAlphaF(fade * 0.9f);
        p->setPen(Qt::NoPen);
        p->setBrush(w);
        p->drawEllipse(QPointF(xx, yy), 2.f, 2.f);
    }
}

void drawAttackFx(QPainter *p, const AttackFx &fx, float time)
{
    float prog = clampf(fx.t / qMax(0.01f, fx.dur), 0.f, 1.f);
    if (prog >= 1.f) return;
    QColor col = fx.color.isValid() ? fx.color : C::bone;
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->translate(fx.pos);

    // 1) спрайт-лист fx_*: кадры идут по прогрессу, цвет - тонировкой
    if (SprBank::enabled()) {
        const char *key = 0, *anim = 0;
        switch (fx.kind) {
        case 0: key = "fx_slash"; anim = "slash";  break;
        case 1: key = "fx_slash"; anim = "cleave"; break;
        case 4: key = "fx_slash"; anim = "thrust"; break;
        case 2: key = "fx_claw";  anim = "claw";   break;
        case 3: key = "fx_smash"; anim = "smash";  break;
        case 5: key = "fx_cast";  anim = "cast";   break;
        default: break;
        }
        const SprBank::Sheet *sh = key ? SprBank::get(QLatin1String(key)) : 0;
        if (sh && sh->valid && sh->anims.contains(QLatin1String(anim))) {
            const SprBank::Anim am = sh->anims.value(QLatin1String(anim));
            int n = am.to - am.from + 1;
            int fi = qMin(n - 1, int(prog * float(n)));
            float k = (fx.scale * 1.9f) / float(sh->fw);   // арт занимает ~40% ячейки
            p->rotate(qRadiansToDegrees(fx.ang));
            if (fx.flip) p->scale(1.0, -1.0);
            int cols = qMax(1, sh->img.width() / sh->fw);
            int fr = am.from + fi;
            QRect sr((fr % cols) * sh->fw, (fr / cols) * sh->fh, sh->fw, sh->fh);
            QImage img = tintFrame(sh->img, sr, col);
            p->setCompositionMode(QPainter::CompositionMode_Plus);
            p->setRenderHint(QPainter::SmoothPixmapTransform, true);
            p->drawImage(QRectF(-sh->ax * k, -sh->ay * k, sh->fw * k, sh->fh * k), img);
            p->restore();
            return;
        }
    }

    // 2) процедурный росчерк
    p->rotate(qRadiansToDegrees(fx.ang));
    if (fx.flip) p->scale(1.0, -1.0);
    p->setCompositionMode(QPainter::CompositionMode_Plus);
    float R = fx.scale;
    switch (fx.kind) {
    case 2: fxClaw(p, R, prog, col, fx.seed); break;
    case 3: fxSmash(p, R, prog, col, fx.seed); break;
    case 5: fxCast(p, R, prog, col, fx.seed, time); break;
    default: fxArcSlash(p, R, prog, col, (fx.kind == 4) ? 4 : fx.kind); break;
    }
    p->restore();
}

void drawProjectile(QPainter *p, const Proj &pr, float t)
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    float ang = qAtan2(pr.vel.y(), pr.vel.x());
    QColor col = pr.color.isValid() ? pr.color : C::bone;
    p->translate(pr.pos);

    // спрайт-снаряды fx_*: анимированные листы вместо процедурной графики
    if (SprBank::enabled() && projectileSheet(p, pr, t, col)) {
        p->restore();
        return;
    }

    switch (pr.kind) {
    case 0: { // стрела
        Style::glow(p, QPointF(0, 0), 12.f, col, 0.25f);
        p->rotate(qRadiansToDegrees(ang));
        p->setPen(QPen(QColor(120, 92, 58), 2.4f, Qt::SolidLine, Qt::RoundCap));
        p->drawLine(QPointF(-13 * pr.scale, 0), QPointF(7 * pr.scale, 0));
        polyf(p, QVector<QPointF>() << QPointF(13 * pr.scale, 0) << QPointF(5 * pr.scale, -3.4f)
              << QPointF(5 * pr.scale, 3.4f), col.lighter(120));
        p->setPen(QPen(C::boneDim, 1.2f));
        p->drawLine(QPointF(-13 * pr.scale, 0), QPointF(-17 * pr.scale, -3.2f));
        p->drawLine(QPointF(-13 * pr.scale, 0), QPointF(-17 * pr.scale, 3.2f));
        break;
    }
    case 1: { // огненный шар
        Style::glow(p, QPointF(0, 0), 30.f * pr.scale, col, 0.85f);
        for (int i = 3; i >= 0; --i) {
            float k = 1.f - i * 0.2f;
            QColor c = (i == 0) ? QColor(255, 250, 220) : (i == 1 ? QColor(255, 200, 90) : (i == 2 ? col : col.darker(180)));
            c.setAlphaF(0.9f);
            float rr = 7.f * pr.scale * k * (1.f + 0.14f * qSin(t * 22.f + i));
            p->setPen(Qt::NoPen);
            p->setBrush(c);
            p->drawEllipse(QPointF(0, 0), rr, rr);
        }
        // хвост
        p->rotate(qRadiansToDegrees(ang));
        for (int i = 1; i <= 4; ++i) {
            QColor c = col; c.setAlpha(150 - i * 32);
            p->setBrush(c);
            p->drawEllipse(QPointF(-i * 6.f * pr.scale, qSin(t * 30.f + i) * 1.6f), 5.f * pr.scale * (1.f - i * 0.16f), 4.f * pr.scale);
        }
        break;
    }
    case 2: { // ледяной снаряд
        Style::glow(p, QPointF(0, 0), 22.f * pr.scale, col, 0.6f);
        p->rotate(qRadiansToDegrees(ang));
        polyf(p, QVector<QPointF>() << QPointF(12 * pr.scale, 0) << QPointF(2 * pr.scale, -5 * pr.scale)
              << QPointF(-10 * pr.scale, -2.5f * pr.scale) << QPointF(-10 * pr.scale, 2.5f * pr.scale)
              << QPointF(2 * pr.scale, 5 * pr.scale), col.lighter(130), QColor(255, 255, 255, 180), 1.0);
        p->setPen(QPen(QColor(255, 255, 255, 150), 1.0));
        p->drawLine(QPointF(-8 * pr.scale, 0), QPointF(9 * pr.scale, 0));
        break;
    }
    case 3: { // молния-снаряд
        Style::glow(p, QPointF(0, 0), 26.f * pr.scale, col, 0.8f);
        p->rotate(qRadiansToDegrees(ang));
        p->setPen(QPen(col, 2.6f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPolygonF bolt;
        for (int i = 0; i <= 5; ++i) {
            float x = -12.f * pr.scale + i * 5.f * pr.scale;
            bolt << QPointF(x, (i % 2 ? 3.4f : -3.4f) * pr.scale * (i == 0 || i == 5 ? 0.2f : 1.f));
        }
        p->setBrush(Qt::NoBrush);
        p->drawPolyline(bolt);
        p->setPen(QPen(QColor(255, 255, 255, 220), 1.2f));
        p->drawPolyline(bolt);
        break;
    }
    case 4: { // яд
        Style::glow(p, QPointF(0, 0), 18.f * pr.scale, col, 0.45f);
        blob(p, QPointF(0, 0), 6.f * pr.scale, 6.5f * pr.scale, col, col.darker(180), 1.0);
        for (int i = 0; i < 3; ++i) {
            float a = t * 8.f + i * 2.1f;
            blob(p, QPointF(qCos(a) * 7.f * pr.scale, qSin(a) * 7.f * pr.scale), 2.0f, 2.0f, col.lighter(140));
        }
        break;
    }
    case 5: { // тайная сфера
        Style::glow(p, QPointF(0, 0), 26.f * pr.scale, col, 0.7f);
        p->setBrush(col);
        p->setPen(QPen(col.lighter(170), 1.4));
        p->drawEllipse(QPointF(0, 0), 6.5f * pr.scale, 6.5f * pr.scale);
        p->setPen(QPen(QColor(255, 255, 255, 150), 1.2));
        p->setBrush(Qt::NoBrush);
        p->drawEllipse(QPointF(0, 0), 9.f * pr.scale * (0.8f + 0.2f * qSin(t * 14.f)), 4.f * pr.scale);
        p->drawEllipse(QPointF(0, 0), 4.f * pr.scale, 9.f * pr.scale * (0.8f + 0.2f * qCos(t * 14.f)));
        break;
    }
    default: { // камень/глыба
        Style::glow(p, QPointF(0, 0), 34.f * pr.scale, col, 0.7f);
        p->rotate(t * 200.f);
        QPolygonF pg;
        for (int i = 0; i < 7; ++i) {
            float a = i / 7.f * 2.f * float(M_PI);
            float rr = 9.f * pr.scale * (0.75f + ((i * 37) % 5) / 10.f);
            pg << QPointF(qCos(a) * rr, qSin(a) * rr);
        }
        p->setBrush(QColor(70, 44, 36));
        p->setPen(QPen(col, 2.0));
        p->drawPolygon(pg);
        break;
    }
    }
    p->restore();
}

void drawZone(QPainter *p, const Zone &z, float t)
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    float k = clampf(z.life / qMax(0.001f, z.maxLife), 0.f, 1.f);
    float fade = qMin(1.f, k * 3.f);
    QColor col = z.c.isValid() ? z.c : C::ember;
    p->translate(z.pos);

    if (z.kind == 0) { // огонь
        Style::glow(p, QPointF(0, 0), z.radius * 1.2f, col, 0.35f * fade);
        p->setPen(QPen(QColor(col.red(), col.green(), col.blue(), int(120 * fade)), 2.0));
        p->setBrush(QColor(col.red(), col.green(), col.blue(), int(46 * fade)));
        p->drawEllipse(QPointF(0, 0), z.radius, z.radius * 0.55f);
        for (int i = 0; i < 10; ++i) {
            float ph = t * 2.2f + i * 0.63f;
            float fr = fmodf(ph, 1.f);
            float ang = i * 2.399f;
            float rr = z.radius * (0.25f + 0.7f * ((i * 13) % 10) / 10.f);
            QPointF c(qCos(ang) * rr, qSin(ang) * rr * 0.55f);
            float hh = z.radius * 0.34f * (1.f - fr);
            QColor fc = fr < 0.4f ? QColor(255, 240, 170) : (fr < 0.7f ? col : col.darker(160));
            fc.setAlphaF((1.f - fr) * fade);
            QPolygonF pg;
            pg << c + QPointF(-4, 0) << c + QPointF(qSin(t * 8.f + i) * 3, -hh) << c + QPointF(4, 0);
            p->setBrush(fc);
            p->setPen(Qt::NoPen);
            p->drawPolygon(pg);
        }
    } else if (z.kind == 1) { // яд
        Style::glow(p, QPointF(0, 0), z.radius * 1.1f, col, 0.22f * fade);
        p->setBrush(QColor(col.red(), col.green(), col.blue(), int(70 * fade)));
        p->setPen(QPen(QColor(col.red(), col.green(), col.blue(), int(140 * fade)), 1.6));
        QPainterPath path;
        int N = 18;
        for (int i = 0; i <= N; ++i) {
            float a = float(i) / N * 2.f * float(M_PI);
            float rr = z.radius * (0.86f + 0.14f * qSin(a * 3.f + t * 2.f));
            QPointF pt(qCos(a) * rr, qSin(a) * rr * 0.55f);
            if (i == 0) path.moveTo(pt); else path.lineTo(pt);
        }
        path.closeSubpath();
        p->drawPath(path);
        for (int i = 0; i < 6; ++i) {
            float ph = fmodf(t * 1.1f + i * 0.37f, 1.f);
            float ang = i * 1.05f;
            QPointF c(qCos(ang) * z.radius * 0.6f, qSin(ang) * z.radius * 0.34f - ph * 14.f);
            blob(p, c, 3.4f * (1.f - ph), 3.4f * (1.f - ph), QColor(col.red(), col.green(), col.blue(), int(160 * (1.f - ph) * fade)));
        }
    } else if (z.kind == 2) { // лёд
        Style::glow(p, QPointF(0, 0), z.radius * 1.1f, col, 0.25f * fade);
        p->setBrush(QColor(col.red(), col.green(), col.blue(), int(80 * fade)));
        p->setPen(QPen(QColor(230, 250, 255, int(150 * fade)), 1.6));
        QPolygonF pg;
        for (int i = 0; i < 8; ++i) {
            float a = i * float(M_PI) / 4.f + t * 0.2f;
            pg << QPointF(qCos(a) * z.radius, qSin(a) * z.radius * 0.55f);
        }
        p->drawPolygon(pg);
        p->setPen(QPen(QColor(255, 255, 255, int(90 * fade)), 1.0));
        for (int i = 0; i < 4; ++i) {
            float a = i * float(M_PI) / 2.f + t * 0.2f;
            p->drawLine(QPointF(0, 0), QPointF(qCos(a) * z.radius, qSin(a) * z.radius * 0.55f));
        }
    } else if (z.kind == 4) { // свет/лечение
        Style::glow(p, QPointF(0, 0), z.radius * 1.3f, col, 0.30f * fade);
        p->setPen(QPen(QColor(col.red(), col.green(), col.blue(), int(150 * fade)), 1.6));
        p->setBrush(Qt::NoBrush);
        p->drawEllipse(QPointF(0, 0), z.radius * (0.6f + 0.4f * (1.f - k)), z.radius * 0.4f * (0.6f + 0.4f * (1.f - k)));
    } else { // тайная руна
        Style::glow(p, QPointF(0, 0), z.radius * 1.2f, col, 0.30f * fade);
        p->setPen(QPen(QColor(col.red(), col.green(), col.blue(), int(160 * fade)), 1.8));
        p->setBrush(QColor(col.red(), col.green(), col.blue(), int(40 * fade)));
        p->drawEllipse(QPointF(0, 0), z.radius, z.radius * 0.55f);
        p->save();
        p->scale(1.f, 0.55f);
        p->setBrush(Qt::NoBrush);
        p->drawPath(runeRing(QPointF(0, 0), z.radius * 0.82f, 12, z.radius * 0.14f, t * 0.8f));
        p->drawPath(runeRing(QPointF(0, 0), z.radius * 0.52f, 8, z.radius * 0.10f, -t * 1.3f));
        p->restore();
    }
    p->restore();
}

void drawTelegraph(QPainter *p, const Telegraph &t)
{
    float k = clampf(t.t / qMax(0.0001f, t.dur), 0.f, 1.f);
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    QColor c = t.c.isValid() ? t.c : C::bad;
    if (t.kind == 0) {
        p->setPen(QPen(QColor(c.red(), c.green(), c.blue(), 200), 2.2));
        p->setBrush(QColor(c.red(), c.green(), c.blue(), int(50 + 90 * k)));
        p->save();
        p->translate(t.pos);
        p->scale(1.0, 0.55);
        p->drawEllipse(QPointF(0, 0), t.radius, t.radius);
        p->setBrush(Qt::NoBrush);
        p->drawEllipse(QPointF(0, 0), t.radius * k, t.radius * k);
        p->restore();
        p->setPen(QPen(QColor(255, 255, 255, int(120 * k)), 1.4));
        p->save();
        p->translate(t.pos);
        p->scale(1.0, 0.55);
        p->drawPath(runeRing(QPointF(0, 0), t.radius * 0.7f, 10, t.radius * 0.12f, k * 6.f));
        p->restore();
    } else if (t.kind == 1) {
        QPointF d = t.pos2 - t.pos;
        float len = qSqrt(d.x() * d.x() + d.y() * d.y());
        if (len < 1.f) len = 1.f;
        QPointF n(-d.y() / len, d.x() / len);
        QPolygonF pg;
        pg << t.pos + n * t.width * 0.5f << t.pos2 + n * t.width * 0.5f
           << t.pos2 - n * t.width * 0.5f << t.pos - n * t.width * 0.5f;
        p->setBrush(QColor(c.red(), c.green(), c.blue(), int(40 + 100 * k)));
        p->setPen(QPen(QColor(c.red(), c.green(), c.blue(), 200), 2.0));
        p->drawPolygon(pg);
    } else {
        float ang = qAtan2(t.pos2.y() - t.pos.y(), t.pos2.x() - t.pos.x());
        p->setBrush(QColor(c.red(), c.green(), c.blue(), int(40 + 100 * k)));
        p->setPen(QPen(QColor(c.red(), c.green(), c.blue(), 200), 2.0));
        p->drawPie(QRectF(t.pos.x() - t.radius, t.pos.y() - t.radius, t.radius * 2, t.radius * 2),
                   int((qRadiansToDegrees(ang) - 35) * 16), int(70 * 16));
    }
    p->restore();
}

void drawParticle(QPainter *p, const Part &pt)
{
    float k = clampf(pt.life / qMax(0.0001f, pt.maxLife), 0.f, 1.f);
    QColor c = pt.c;
    c.setAlphaF(c.alphaF() * k);
    p->save();
    if (pt.additive) p->setCompositionMode(QPainter::CompositionMode_Plus);
    p->setPen(Qt::NoPen);
    p->setBrush(c);
    switch (pt.kind) {
    case 1: { // искра
        p->setRenderHint(QPainter::Antialiasing, true);
        float len = qMin(9.f, float(qSqrt(pt.vel.x() * pt.vel.x() + pt.vel.y() * pt.vel.y())) * 0.03f);
        QPointF dir = len > 0.01f ? QPointF(pt.vel.x(), pt.vel.y()) : QPointF(1, 0);
        float dl = qSqrt(dir.x() * dir.x() + dir.y() * dir.y());
        dir /= qMax(0.001f, dl);
        p->setPen(QPen(c, pt.size * k, Qt::SolidLine, Qt::RoundCap));
        p->drawLine(pt.pos - dir * len, pt.pos + dir * len * 0.4f);
        break;
    }
    case 2: { // дым
        p->setRenderHint(QPainter::Antialiasing, true);
        p->setCompositionMode(QPainter::CompositionMode_SourceOver);
        p->drawEllipse(pt.pos, pt.size * (2.f - k), pt.size * (2.f - k));
        break;
    }
    case 3: { // осколок
        p->setRenderHint(QPainter::Antialiasing, true);
        p->translate(pt.pos);
        p->rotate(pt.rot);
        QPolygonF pg;
        pg << QPointF(-pt.size, 0) << QPointF(0, -pt.size * 0.6f) << QPointF(pt.size, 0) << QPointF(0, pt.size * 0.6f);
        p->drawPolygon(pg);
        break;
    }
    case 6: { // вспышка попадания (звезда)
        p->setRenderHint(QPainter::Antialiasing, true);
        float gr = 1.f - k;
        float rr = pt.size * (0.5f + 2.8f * gr);
        p->translate(pt.pos);
        p->rotate(qRadiansToDegrees(pt.rot));
        p->drawPolygon(starPolygon(QPointF(0, 0), rr, rr * 0.30f, 4, 0.f));
        break;
    }
    case 5: { // руна
        p->setRenderHint(QPainter::Antialiasing, true);
        p->translate(pt.pos);
        p->rotate(pt.rot);
        p->setPen(QPen(c, 1.4));
        p->setBrush(Qt::NoBrush);
        p->drawRect(QRectF(-pt.size, -pt.size, pt.size * 2, pt.size * 2));
        p->drawLine(QPointF(-pt.size, -pt.size), QPointF(pt.size, pt.size));
        break;
    }
    default:
        p->setRenderHint(QPainter::Antialiasing, true);
        p->drawEllipse(pt.pos, pt.size * k, pt.size * k);
        break;
    }
    p->restore();
}

void drawPortal(QPainter *p, const QPointF &pos, float t, bool active)
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    QColor c1 = active ? C::arcane : C::ash;
    QColor c2 = active ? C::manaHi : C::ash.darker(150);
    float pulse = active ? (0.7f + 0.3f * qSin(t * 3.f)) : 0.35f;
    Style::glow(p, pos + QPointF(0, -26), 70.f, c1, 0.4f * pulse);
    // рама
    p->setPen(QPen(C::edgeHi.darker(130), 5.0));
    p->setBrush(QColor(6, 4, 12, 235));
    QPainterPath arch;
    arch.moveTo(pos.x() - 26, pos.y() + 4);
    arch.lineTo(pos.x() - 26, pos.y() - 40);
    arch.cubicTo(pos.x() - 26, pos.y() - 74, pos.x() + 26, pos.y() - 74, pos.x() + 26, pos.y() - 40);
    arch.lineTo(pos.x() + 26, pos.y() + 4);
    arch.closeSubpath();
    p->drawPath(arch);
    p->save();
    p->setClipPath(arch);
    QRadialGradient g(pos + QPointF(0, -34), 40.f);
    g.setColorAt(0, c2);
    g.setColorAt(0.5, c1);
    g.setColorAt(1, QColor(8, 4, 16));
    p->fillRect(QRectF(pos.x() - 30, pos.y() - 80, 60, 88), QBrush(g));
    // вихрь
    p->setPen(QPen(QColor(255, 255, 255, 90), 1.4));
    for (int i = 0; i < 3; ++i) {
        float rr = 10.f + i * 7.f + qSin(t * 2.f + i) * 2.f;
        p->save();
        p->translate(pos.x(), pos.y() - 34);
        p->scale(1.0, 0.55);
        p->rotate(t * (60.f + i * 30.f));
        p->setBrush(Qt::NoBrush);
        p->drawEllipse(QPointF(0, 0), rr, rr * 0.6f);
        p->restore();
    }
    p->restore();
    p->setPen(QPen(C::edgeHi, 2.0));
    p->setBrush(Qt::NoBrush);
    p->drawPath(arch);
    // руны на раме
    for (int i = 0; i < 4; ++i) {
        float y = pos.y() - 12 - i * 15;
        glowDot(p, QPointF(pos.x() - 26, y), 2.4f, c1, pulse);
        glowDot(p, QPointF(pos.x() + 26, y), 2.4f, c1, pulse);
    }
    p->restore();
}

void drawLightHalo(QPainter *p, const QPointF &pos, float radius, const QColor &c, float power)
{
    Style::glow(p, pos, radius, c, power);
}

} // namespace Spr
} // namespace aa
