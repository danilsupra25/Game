#ifndef AA_STYLE_H
#define AA_STYLE_H

// ============================================================
//  Визуальный стиль «Пепел и Янтарь».
//  Вся графика интерфейса и иконки рисуются процедурно (QPainter),
//  внешних картинок в проекте нет вообще.
// ============================================================

#include <QColor>
#include <QRect>
#include <QRectF>
#include <QFont>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace aa {

// ---------------- Палитра ----------------
namespace C {
const QColor voidBlack (  9,   8,  14);   // фон-бездна
const QColor ink       ( 16,  14,  24);
const QColor panel     ( 26,  22,  36);
const QColor panelHi   ( 40,  33,  52);
const QColor panelLo   ( 18,  15,  26);
const QColor edge      ( 74,  58,  38);   // тёмная бронза
const QColor edgeHi    (201, 146,  46);   // янтарь-рама
const QColor amber     (242, 181,  68);
const QColor ember     (255, 122,  46);
const QColor bone      (232, 220, 192);
const QColor boneDim   (168, 156, 134);
const QColor ash       (110, 104, 118);
const QColor blood     (196,  48,  42);
const QColor bloodHi   (240,  86,  70);
const QColor mana      ( 78, 142, 216);
const QColor manaHi    (136, 194, 255);
const QColor poison    (126, 196,  84);
const QColor frost     (134, 206, 236);
const QColor lightning (250, 226, 120);
const QColor arcane    (188, 120, 240);
const QColor gold      (240, 200,  90);
const QColor hp        (206,  58,  50);
const QColor hpHi      (255, 120, 100);
const QColor xp        (198, 158, 255);
const QColor good      (120, 200, 130);
const QColor bad       (226,  92,  78);
const QColor parchment ( 62,  50,  38);
const QColor parchHi   (112,  92,  62);
}

// ---------------- Иконки-глифы ----------------
enum Glyph {
    Gl_None = 0,
    // оружие / экипировка
    Gl_Sword, Gl_GreatSword, Gl_Axe, Gl_GreatAxe, Gl_Dagger, Gl_Mace, Gl_Spear,
    Gl_Bow, Gl_Staff, Gl_Wand, Gl_Tome, Gl_Shield, Gl_Torch, Gl_Totem,
    Gl_Helm, Gl_Hood, Gl_Crown, Gl_Chest, Gl_Plate, Gl_Robe, Gl_Gloves, Gl_Belt,
    Gl_Boots, Gl_Amulet, Gl_Ring,
    // камни / расходники
    Gl_GemRed, Gl_GemBlue, Gl_GemYellow, Gl_GemGreen, Gl_GemViolet,
    Gl_PotionHp, Gl_PotionMp, Gl_PotionXp, Gl_Scroll, Gl_Coin, Gl_Coins,
    // умения
    Gl_Cleave, Gl_Spin, Gl_Dash, Gl_Fireball, Gl_Meteor, Gl_IceShard, Gl_Chain,
    Gl_Nova, Gl_Poison, Gl_Summon, Gl_Ward, Gl_Heal, Gl_MultiArrow, Gl_Quake,
    Gl_Mark, Gl_Blood, Gl_Curse, Gl_Blink,
    // состояния
    Gl_Burn, Gl_Chill, Gl_Shock, Gl_PoisonDot, Gl_MarkDot, Gl_ShieldDot, Gl_Haste,
    Gl_Skull, Gl_Heart, Gl_Drop, Gl_Eye,
    // мир / карта
    Gl_Town, Gl_Camp, Gl_Ruins, Gl_Cave, Gl_Forest, Gl_Swamp, Gl_Fortress,
    Gl_Shrine, Gl_Elite, Gl_Boss, Gl_Portal, Gl_Anvil, Gl_Key, Gl_Tree,
    Gl_Crystal, Gl_Bone, Gl_Spider, Gl_Wolf, Gl_Volcano, Gl_Star,
    // системные
    Gl_Lock, Gl_Unlock, Gl_Save, Gl_Load, Gl_Settings, Gl_Play, Gl_Info, Gl_Quit,
    Gl_Respec, Gl_LevelUp, Gl_ArrowUp, Gl_ArrowRight, Gl_Check, Gl_Cross,
    Gl_Bag, Gl_Web, Gl_Map, Gl_Swords, Gl_Crown2, Gl_Flask, Gl_Hammer,
    Gl_Count
};

// ---------------- Шрифты ----------------
QFont fontTitle(int pt = 34, bool bold = true);
QFont fontHead (int pt = 17);
QFont fontBody (int pt = 12);
QFont fontSmall(int pt = 10);
QFont fontNum  (int pt = 15, bool bold = true);

// ---------------- Примитивы интерфейса ----------------
namespace Style {

// Панель в стиле «тёмное дерево + бронзовая рама». style: 0 обычная, 1 тёмная, 2 свиток/пергамент
void panel(QPainter *p, const QRectF &r, int style = 0, float alpha = 1.0f);

// Угловые заклёпки/руны
void corners(QPainter *p, const QRectF &r, const QColor &c, float s = 6.0f);

// Рамка-обводка
void frame(QPainter *p, const QRectF &r, const QColor &c, float w = 1.5f);

// Кнопка (рисует состояние).
// hoverK - сглаженный ховер 0..1 (<0 - использовать hover), active - кнопка «включена/выбрана».
void button(QPainter *p, const QRectF &r, const QString &text, bool hover, bool down,
            bool enabled = true, int glyph = Gl_None, float hoverK = -1.f, bool active = false);

// Полоса (здоровье/опыт/кулдаун) с «стеклянным» бликом
void bar(QPainter *p, const QRectF &r, float t, const QColor &fill, const QColor &edgec,
         bool vertical = false, float glow = 0.f);

// Сфера (HP/MP орбы)
void orb(QPainter *p, const QRectF &r, float t, const QColor &c1, const QColor &c2, float pulse = 0.f);

// Текст с тенью
void text(QPainter *p, const QPointF &pos, const QString &s, const QColor &c,
          const QFont &f, int flags = Qt::AlignLeft | Qt::AlignTop);
void textShadow(QPainter *p, const QRectF &r, const QString &s, const QColor &c,
                const QFont &f, int flags = Qt::AlignCenter);

// Небольшая кафельная текстура шума (96x96, кэш по seed/base/amount).
// Дорогую генерацию по размеру больше не делаем - текстура тайлится кистью.
QImage noiseTile(int seed, const QColor &base, float amount);

// Радиальное свечение
void glow(QPainter *p, const QPointF &c, float radius, const QColor &col, float power = 1.0f);

// Мягкий эллиптический диск обычным смешиванием (туман, пятна) - кэшированная текстура
void disc(QPainter *p, const QPointF &c, float rx, float ry, const QColor &col, float opacity = 1.0f);

// «Дымка» виньетка поверх виджета
void vignette(QPainter *p, const QRectF &r, float strength = 0.55f, const QColor &col = C::voidBlack);

// Тонкая декоративная линия-разделитель с ромбом посередине
void divider(QPainter *p, qreal y, qreal x0, qreal x1, const QColor &c);

// Ячейка инвентаря
void slotCell(QPainter *p, const QRectF &r, bool hover, bool selected, bool filled);

} // namespace Style

// ---------------- Иконки ----------------
// Рисует глиф, вписанный в rect. glow>0 добавляет свечение.
void drawGlyph(QPainter *p, int glyph, const QRectF &rect, const QColor &main,
               const QColor &accent = QColor(), float glow = 0.f);

QColor glyphTint(int glyph);

// ---------------- Мелкие утилиты ----------------
QString fmtNum(float v, int prec = 0);
QString fmtPct(float v, int prec = 0);      // 0.12 -> "+12%"
QString romanNum(int n);

inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
inline float smoothstep(float t) { t = clampf(t, 0.f, 1.f); return t * t * (3 - 2 * t); }
inline float easeOutCubic(float t) { t = clampf(t, 0.f, 1.f); float u = 1.f - t; return 1.f - u * u * u; }
inline float easeInCubic(float t) { t = clampf(t, 0.f, 1.f); return t * t * t; }
inline float easeOutBack(float t)   // упругий «выстрел» за цель
{
    t = clampf(t, 0.f, 1.f);
    const float c = 1.70158f;
    float u = t - 1.f;
    return 1.f + u * u * ((c + 1.f) * u + c);
}
inline float easeOutElastic(float t)
{
    t = clampf(t, 0.f, 1.f);
    if (t < 0.001f) return 0.f;
    if (t > 0.999f) return 1.f;
    return qPow(2.f, -9.f * t) * qSin((t - 0.075f) * (2.f * float(M_PI)) / 0.3f) + 1.f;
}
// кривая взмаха: замах -> удар -> доводка (t: 0 начало .. 1 конец)
inline float swingCurve(float t)
{
    t = clampf(t, 0.f, 1.f);
    if (t < 0.24f) return -0.60f * easeOutCubic(t / 0.24f);
    if (t < 0.58f) return -0.60f + 2.45f * easeOutCubic((t - 0.24f) / 0.34f);
    return 1.85f * (1.f - smoothstep((t - 0.58f) / 0.42f));
}

QPolygonF starPolygon(const QPointF &c, float rOut, float rIn, int points, float rot = 0.f);
QPainterPath runeRing(const QPointF &c, float r, int teeth, float depth, float rot = 0.f);

} // namespace aa

#endif // AA_STYLE_H
