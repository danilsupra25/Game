#include "Core/Data.h"
#include "Core/Style.h"
#include "Core/Rng.h"
#include "World/Dungeon.h"

#include <QtMath>
#include <QFile>

namespace aa {

// ---------- реализация методов Item из Types.h ----------
QString Item::name() const { return Data::itemNameFull(*this); }
Slot Item::slot() const { const ItemBaseDef *b = Data::base(baseId); return b ? b->slot : Slot_Weapon; }
bool Item::isEquip() const { const ItemBaseDef *b = Data::base(baseId); return b && b->bag == Bag_Equip; }
bool Item::stackable() const { const ItemBaseDef *b = Data::base(baseId); return b && b->bag != Bag_Equip; }

// ============================================================
//  ТАБЛИЦЫ ДАННЫХ
// ============================================================
namespace Data {

static QVector<ItemBaseDef>  g_bases;
static QVector<AffixDef>     g_affixes;
static QVector<UniqueDef>    g_uniques;
static QVector<ActiveSkill>  g_skills;
static QVector<SkillNode>    g_nodes;
static QVector<EnemyDef>     g_enemies;
static QVector<LocationDef>  g_locs;
static QVector<EliteAffix>   g_elites;
static bool g_loaded = false;

// объявлены ниже, но нужны уже в loadNodes()
float ringRadius(int ring);
float angDiff(float a, float b);

// компактный построитель модификаторов
struct MB {
    StatMods m;
    MB &s(float StatMods::*f, float v) { m.*f = v; return *this; }
    MB &a(float StatMods::*f, float v) { m.*f += v; return *this; }
    StatMods d() const { return m; }
};

static int tierReq(int tier) { return 1 + (tier - 1) * 6; }

static int slotMaskOf(Slot a) { return 1 << int(a); }
static const int MSK_WEAPON  = 1 << Slot_Weapon;
static const int MSK_OFF     = 1 << Slot_Offhand;
static const int MSK_ARMOR   = (1 << Slot_Helm) | (1 << Slot_Chest) | (1 << Slot_Gloves)
                               | (1 << Slot_Belt) | (1 << Slot_Boots);
static const int MSK_JEWEL   = (1 << Slot_Amulet) | (1 << Slot_Ring1) | (1 << Slot_Ring2);
static const int MSK_ALL_EQ  = MSK_WEAPON | MSK_OFF | MSK_ARMOR | MSK_JEWEL;
static const int MSK_NOBELT  = MSK_ALL_EQ ^ (1 << Slot_Belt);

// ----------------------------------------------------------------
//  БАЗЫ ПРЕДМЕТОВ
// ----------------------------------------------------------------
static void loadBases()
{
    g_bases.clear();
    int nextId = 0;
    auto add = [&](const QString &id, const QString &name, Slot slot, BagKind bag, int tier, int glyph,
                   float dmin, float dmax, float spd, DmgType dt, const StatMods &st, int sock, int price)
    {
        ItemBaseDef b;
        b.id = nextId++;
        b.id_str = id;
        b.name = name;
        b.slot = slot;
        b.bag = bag;
        b.tier = tier;
        b.reqLevel = bag == Bag_Equip ? tierReq(tier) : 1;
        b.base = st;
        b.dmgMin = dmin;
        b.dmgMax = dmax;
        b.atkSpeed = spd;
        b.dmgType = dt;
        Q_UNUSED(sock);
        b.maxSockets = 0;   // самоцветы убраны из игры: гнёзд больше нет
        b.glyph = glyph;
        b.priceBase = price;
        g_bases.append(b);
        return b.id;
    };
    auto eq = [&](const QString &id, const QString &n, Slot s, int t, int g, float dmin, float dmax,
                  float spd, DmgType dt, const StatMods &st, int sock, int price)
    { return add(id, n, s, Bag_Equip, t, g, dmin, dmax, spd, dt, st, sock, price); };

    // ---------- ОРУЖИЕ ----------
    eq("w_rusty",   QStringLiteral("Ржавый тесак"),        Slot_Weapon, 1, Gl_Sword,      4, 7,   1.15f, Dmg_Physical, StatMods(), 0, 12);
    eq("w_knife",   QStringLiteral("Охотничий нож"),       Slot_Weapon, 1, Gl_Dagger,     3, 5,   1.65f, Dmg_Physical, MB().s(&StatMods::agi, 1).d(), 0, 14);
    eq("w_club",    QStringLiteral("Дубина скитальца"),    Slot_Weapon, 1, Gl_Mace,       5, 9,   0.90f, Dmg_Physical, MB().s(&StatMods::vit, 1).d(), 0, 13);
    eq("w_sbow",    QStringLiteral("Короткий лук"),        Slot_Weapon, 1, Gl_Bow,        4, 8,   1.20f, Dmg_Physical, MB().s(&StatMods::agi, 1).d(), 0, 16);
    eq("w_appstf",  QStringLiteral("Посох ученика"),       Slot_Weapon, 1, Gl_Staff,      4, 8,   1.00f, Dmg_Arcane,   MB().s(&StatMods::focus, 2).d(), 1, 18);
    eq("w_steel",   QStringLiteral("Стальной клинок"),     Slot_Weapon, 2, Gl_Sword,      9, 15,  1.20f, Dmg_Physical, MB().s(&StatMods::might, 2).d(), 1, 46);
    eq("w_baxe",    QStringLiteral("Боевой топор"),        Slot_Weapon, 2, Gl_Axe,        12, 20, 0.95f, Dmg_Physical, MB().s(&StatMods::might, 3).d(), 1, 52);
    eq("w_spear",   QStringLiteral("Копьё дозора"),        Slot_Weapon, 2, Gl_Spear,      10, 17, 1.05f, Dmg_Physical, MB().s(&StatMods::agi, 2).d(), 1, 50);
    eq("w_storm",   QStringLiteral("Жезл бури"),           Slot_Weapon, 2, Gl_Wand,       9, 16,  1.10f, Dmg_Lightning,MB().s(&StatMods::focus, 3).d(), 1, 58);
    eq("w_windbow", QStringLiteral("Лук ветра"),           Slot_Weapon, 2, Gl_Bow,        10, 18, 1.25f, Dmg_Physical, MB().s(&StatMods::agi, 3).d(), 1, 56);
    eq("w_ashblade",QStringLiteral("Клинок пепла"),        Slot_Weapon, 3, Gl_Sword,      18, 28, 1.20f, Dmg_Fire,     MB().s(&StatMods::might, 4).s(&StatMods::firePct, 0.06f).d(), 2, 130);
    eq("w_exec",    QStringLiteral("Секира палача"),       Slot_Weapon, 3, Gl_GreatAxe,   26, 40, 0.80f, Dmg_Physical, MB().s(&StatMods::might, 6).d(), 2, 150);
    eq("w_emberstf",QStringLiteral("Посох углей"),         Slot_Weapon, 3, Gl_Staff,      19, 30, 1.00f, Dmg_Fire,     MB().s(&StatMods::spirit, 5).s(&StatMods::firePct, 0.08f).d(), 2, 145);
    eq("w_grimoire",QStringLiteral("Гримуар заката"),      Slot_Weapon, 3, Gl_Tome,       17, 27, 1.05f, Dmg_Arcane,   MB().s(&StatMods::focus, 5).s(&StatMods::castSpeed, 0.05f).d(), 2, 148);
    eq("w_frostspr",QStringLiteral("Копьё инея"),          Slot_Weapon, 3, Gl_Spear,      20, 31, 1.05f, Dmg_Frost,    MB().s(&StatMods::agi, 4).s(&StatMods::frostPct, 0.07f).d(), 2, 140);
    eq("w_longbow", QStringLiteral("Длинный лук странника"),Slot_Weapon,3, Gl_Bow,        19, 30, 1.25f, Dmg_Physical, MB().s(&StatMods::agi, 5).s(&StatMods::critChance, 0.02f).d(), 2, 142);
    eq("w_dawn",    QStringLiteral("Меч зари"),            Slot_Weapon, 4, Gl_GreatSword, 34, 52, 1.05f, Dmg_Physical, MB().s(&StatMods::might, 8).s(&StatMods::dmgPct, 0.05f).d(), 2, 320);
    eq("w_night",   QStringLiteral("Кинжал ночи"),         Slot_Weapon, 4, Gl_Dagger,     24, 38, 1.85f, Dmg_Poison,   MB().s(&StatMods::agi, 8).s(&StatMods::critChance, 0.04f).d(), 2, 330);
    eq("w_thunder", QStringLiteral("Скипетр грома"),       Slot_Weapon, 4, Gl_Wand,       32, 48, 1.15f, Dmg_Lightning,MB().s(&StatMods::spirit, 8).s(&StatMods::castSpeed, 0.08f).d(), 2, 340);
    eq("w_abyssstf",QStringLiteral("Посох бездны"),        Slot_Weapon, 4, Gl_Staff,      30, 50, 1.00f, Dmg_Arcane,   MB().s(&StatMods::focus, 9).s(&StatMods::arcanePct, 0.10f).d(), 2, 350);
    eq("w_doom",    QStringLiteral("Клинок рока"),         Slot_Weapon, 5, Gl_GreatSword, 52, 78, 1.05f, Dmg_Physical, MB().s(&StatMods::might, 12).s(&StatMods::critMult, 0.15f).d(), 3, 720);
    eq("w_reaper",  QStringLiteral("Коса жнеца"),          Slot_Weapon, 5, Gl_Spear,      48, 74, 1.15f, Dmg_Poison,   MB().s(&StatMods::spirit, 11).s(&StatMods::lifeSteal, 0.03f).d(), 3, 740);
    eq("w_voidwand",QStringLiteral("Жезл пустоты"),        Slot_Weapon, 5, Gl_Wand,       50, 76, 1.15f, Dmg_Arcane,   MB().s(&StatMods::focus, 12).s(&StatMods::arcanePct, 0.14f).d(), 3, 760);
    eq("w_devour",  QStringLiteral("Лук пожирателя"),      Slot_Weapon, 5, Gl_Bow,        48, 72, 1.30f, Dmg_Physical, MB().s(&StatMods::agi, 12).s(&StatMods::extraProj, 1).d(), 3, 750);
    eq("w_heart",   QStringLiteral("Меч сердца пепла"),    Slot_Weapon, 6, Gl_GreatSword, 96, 140,1.00f, Dmg_Fire,     MB().s(&StatMods::might, 18).s(&StatMods::firePct, 0.20f).s(&StatMods::dmgPct, 0.10f).d(), 3, 1600);
    eq("w_primord", QStringLiteral("Посох первозданной тьмы"),Slot_Weapon,6, Gl_Staff,     80, 124,1.05f, Dmg_Arcane,   MB().s(&StatMods::spirit, 18).s(&StatMods::focus, 10).s(&StatMods::arcanePct, 0.22f).d(), 3, 1650);

    // ---------- ВТОРАЯ РУКА ----------
    eq("o_wshield", QStringLiteral("Деревянный щит"),      Slot_Offhand, 1, Gl_Shield, 0, 0, 1, Dmg_Physical, MB().s(&StatMods::armor, 8).d(), 0, 15);
    eq("o_torch",   QStringLiteral("Факел"),               Slot_Offhand, 1, Gl_Torch, 2, 4, 1.0f, Dmg_Fire,   MB().s(&StatMods::firePct, 0.05f).d(), 0, 14);
    eq("o_totem1",  QStringLiteral("Тотем предков"),       Slot_Offhand, 2, Gl_Totem, 0, 0, 1, Dmg_Physical, MB().s(&StatMods::vit, 3).s(&StatMods::hpFlat, 15).d(), 1, 55);
    eq("o_steelsh", QStringLiteral("Стальной щит"),        Slot_Offhand, 2, Gl_Shield, 0, 0, 1, Dmg_Physical, MB().s(&StatMods::armor, 24).s(&StatMods::vit, 2).d(), 1, 60);
    eq("o_grimshad",QStringLiteral("Гримуар теней"),       Slot_Offhand, 2, Gl_Tome,  0, 0, 1, Dmg_Arcane,   MB().s(&StatMods::focus, 4).s(&StatMods::castSpeed, 0.04f).d(), 1, 62);
    eq("o_frostsh", QStringLiteral("Щит инея"),            Slot_Offhand, 3, Gl_Shield, 0, 0, 1, Dmg_Frost,   MB().s(&StatMods::armor, 48).s(&StatMods::resFrost, 0.10f).d(), 2, 150);
    eq("o_startome",QStringLiteral("Фолиант звёзд"),       Slot_Offhand, 3, Gl_Tome,  0, 0, 1, Dmg_Arcane,   MB().s(&StatMods::focus, 7).s(&StatMods::cdReduction, 0.05f).d(), 2, 155);
    eq("o_aegis",   QStringLiteral("Эгида заката"),        Slot_Offhand, 4, Gl_Shield, 0, 0, 1, Dmg_Physical, MB().s(&StatMods::armor, 90).s(&StatMods::resAll, 0.06f).s(&StatMods::shieldFlat, 25).d(), 2, 360);
    eq("o_abyssh",  QStringLiteral("Щит бездны"),          Slot_Offhand, 5, Gl_Shield, 0, 0, 1, Dmg_Arcane,  MB().s(&StatMods::armor, 160).s(&StatMods::resArcane, 0.15f).s(&StatMods::thorns, 20).d(), 3, 780);
    eq("o_forge",   QStringLiteral("Печать горнила"),      Slot_Offhand, 6, Gl_Totem, 0, 0, 1, Dmg_Fire,     MB().s(&StatMods::armor, 240).s(&StatMods::resFire, 0.25f).s(&StatMods::shieldFlat, 80).d(), 3, 1700);

    // ---------- ШЛЕМЫ ----------
    eq("h_hood1",  QStringLiteral("Капюшон скитальца"),  Slot_Helm, 1, Gl_Hood,  0,0,1,Dmg_Physical, MB().s(&StatMods::armor, 5).d(), 0, 12);
    eq("h_guard",  QStringLiteral("Шлем дозорного"),     Slot_Helm, 2, Gl_Helm,  0,0,1,Dmg_Physical, MB().s(&StatMods::armor, 18).s(&StatMods::vit, 2).d(), 1, 55);
    eq("h_ashmask",QStringLiteral("Маска пепла"),       Slot_Helm, 3, Gl_Helm,  0,0,1,Dmg_Fire,     MB().s(&StatMods::armor, 34).s(&StatMods::resFire, 0.08f).d(), 2, 140);
    eq("h_frostcr",QStringLiteral("Корона инея"),       Slot_Helm, 4, Gl_Crown, 0,0,1,Dmg_Frost,    MB().s(&StatMods::armor, 62).s(&StatMods::focus, 6).s(&StatMods::resFrost, 0.10f).d(), 2, 330);
    eq("h_voidcr", QStringLiteral("Венец бездны"),      Slot_Helm, 5, Gl_Crown, 0,0,1,Dmg_Arcane,   MB().s(&StatMods::armor, 110).s(&StatMods::spirit, 9).s(&StatMods::resArcane, 0.12f).d(), 3, 740);
    eq("h_forge",  QStringLiteral("Шлем горнила"),      Slot_Helm, 6, Gl_Helm,  0,0,1,Dmg_Fire,     MB().s(&StatMods::armor, 190).s(&StatMods::vit, 14).s(&StatMods::resAll, 0.08f).d(), 3, 1550);

    // ---------- НАГРУДНИКИ ----------
    eq("c_rags",   QStringLiteral("Рваная куртка"),      Slot_Chest, 1, Gl_Chest, 0,0,1,Dmg_Physical, MB().s(&StatMods::armor, 8).d(), 0, 14);
    eq("c_chain",  QStringLiteral("Кольчуга"),           Slot_Chest, 2, Gl_Chest, 0,0,1,Dmg_Physical, MB().s(&StatMods::armor, 30).s(&StatMods::vit, 2).d(), 1, 62);
    eq("c_scale",  QStringLiteral("Чешуйчатый доспех"),  Slot_Chest, 3, Gl_Plate, 0,0,1,Dmg_Physical, MB().s(&StatMods::armor, 58).s(&StatMods::vit, 4).d(), 2, 155);
    eq("c_dusk",   QStringLiteral("Плащ заката"),        Slot_Chest, 4, Gl_Robe,  0,0,1,Dmg_Arcane,   MB().s(&StatMods::armor, 96).s(&StatMods::focus, 7).s(&StatMods::resAll, 0.05f).d(), 2, 355);
    eq("c_void",   QStringLiteral("Латы пустоты"),       Slot_Chest, 5, Gl_Plate, 0,0,1,Dmg_Arcane,   MB().s(&StatMods::armor, 170).s(&StatMods::vit, 9).s(&StatMods::resAll, 0.07f).d(), 3, 770);
    eq("c_heart",  QStringLiteral("Панцирь сердца пепла"),Slot_Chest,6, Gl_Plate, 0,0,1,Dmg_Fire,     MB().s(&StatMods::armor, 300).s(&StatMods::vit, 16).s(&StatMods::resFire, 0.18f).d(), 3, 1700);

    // ---------- ПЕРЧАТКИ ----------
    eq("g_wander", QStringLiteral("Перчатки скитальца"), Slot_Gloves, 1, Gl_Gloves, 0,0,1,Dmg_Physical, MB().s(&StatMods::armor, 4).d(), 0, 10);
    eq("g_leather",QStringLiteral("Кожаные нарукавники"),Slot_Gloves, 2, Gl_Gloves, 0,0,1,Dmg_Physical, MB().s(&StatMods::armor, 14).s(&StatMods::attackSpeed, 0.03f).d(), 1, 48);
    eq("g_smith",  QStringLiteral("Рукавицы кузнеца"),   Slot_Gloves, 3, Gl_Gloves, 0,0,1,Dmg_Fire,     MB().s(&StatMods::armor, 26).s(&StatMods::might, 4).d(), 2, 125);
    eq("g_mage",   QStringLiteral("Перчатки чародея"),   Slot_Gloves, 4, Gl_Gloves, 0,0,1,Dmg_Arcane,   MB().s(&StatMods::armor, 44).s(&StatMods::castSpeed, 0.07f).d(), 2, 300);
    eq("g_claw",   QStringLiteral("Когти бездны"),       Slot_Gloves, 5, Gl_Gloves, 0,0,1,Dmg_Arcane,   MB().s(&StatMods::armor, 78).s(&StatMods::critChance, 0.03f).s(&StatMods::attackSpeed, 0.06f).d(), 3, 690);
    eq("g_forge",  QStringLiteral("Рукавицы горнила"),   Slot_Gloves, 6, Gl_Gloves, 0,0,1,Dmg_Fire,     MB().s(&StatMods::armor, 140).s(&StatMods::might, 14).s(&StatMods::attackSpeed, 0.08f).d(), 3, 1450);

    // ---------- ПОЯСА ----------
    eq("b_rope",   QStringLiteral("Верёвочный пояс"),    Slot_Belt, 1, Gl_Belt, 0,0,1,Dmg_Physical, MB().s(&StatMods::hpFlat, 8).d(), 0, 9);
    eq("b_leather",QStringLiteral("Кожаный ремень"),     Slot_Belt, 2, Gl_Belt, 0,0,1,Dmg_Physical, MB().s(&StatMods::hpFlat, 22).s(&StatMods::armor, 8).d(), 1, 46);
    eq("b_soldier",QStringLiteral("Пояс солдата"),       Slot_Belt, 3, Gl_Belt, 0,0,1,Dmg_Physical, MB().s(&StatMods::hpFlat, 45).s(&StatMods::might, 4).d(), 1, 120);
    eq("b_magist", QStringLiteral("Кушак магистра"),     Slot_Belt, 4, Gl_Belt, 0,0,1,Dmg_Arcane,   MB().s(&StatMods::manaFlat, 40).s(&StatMods::focus, 6).d(), 2, 290);
    eq("b_chain",  QStringLiteral("Цепь пустоты"),       Slot_Belt, 5, Gl_Belt, 0,0,1,Dmg_Arcane,   MB().s(&StatMods::hpFlat, 90).s(&StatMods::resAll, 0.05f).d(), 2, 660);
    eq("b_ash",    QStringLiteral("Опояс пепла"),        Slot_Belt, 6, Gl_Belt, 0,0,1,Dmg_Fire,     MB().s(&StatMods::hpFlat, 170).s(&StatMods::armor, 60).s(&StatMods::thorns, 25).d(), 3, 1400);

    // ---------- САПОГИ ----------
    eq("t_sandal", QStringLiteral("Сандали"),            Slot_Boots, 1, Gl_Boots, 0,0,1,Dmg_Physical, MB().s(&StatMods::moveSpeed, 0.03f).d(), 0, 10);
    eq("t_leather",QStringLiteral("Кожаные сапоги"),     Slot_Boots, 2, Gl_Boots, 0,0,1,Dmg_Physical, MB().s(&StatMods::armor, 12).s(&StatMods::moveSpeed, 0.04f).d(), 1, 47);
    eq("t_watch",  QStringLiteral("Сапоги дозора"),      Slot_Boots, 3, Gl_Boots, 0,0,1,Dmg_Physical, MB().s(&StatMods::armor, 22).s(&StatMods::agi, 4).d(), 2, 122);
    eq("t_wind",   QStringLiteral("Шаг ветра"),          Slot_Boots, 4, Gl_Boots, 0,0,1,Dmg_Physical, MB().s(&StatMods::armor, 40).s(&StatMods::moveSpeed, 0.08f).s(&StatMods::dodge, 0.03f).d(), 2, 305);
    eq("t_void",   QStringLiteral("Ступающие по пустоте"),Slot_Boots,5, Gl_Boots, 0,0,1,Dmg_Arcane,   MB().s(&StatMods::armor, 72).s(&StatMods::moveSpeed, 0.10f).s(&StatMods::dodge, 0.05f).d(), 3, 700);
    eq("t_forge",  QStringLiteral("Сапоги горнила"),     Slot_Boots, 6, Gl_Boots, 0,0,1,Dmg_Fire,     MB().s(&StatMods::armor, 130).s(&StatMods::moveSpeed, 0.12f).s(&StatMods::resFire, 0.15f).d(), 3, 1480);

    // ---------- АМУЛЕТЫ ----------
    eq("a_bone",   QStringLiteral("Амулет из кости"),    Slot_Amulet, 1, Gl_Amulet, 0,0,1,Dmg_Physical, MB().s(&StatMods::vit, 2).d(), 0, 20);
    eq("a_amber",  QStringLiteral("Янтарный кулон"),     Slot_Amulet, 2, Gl_Amulet, 0,0,1,Dmg_Fire,     MB().s(&StatMods::spirit, 3).s(&StatMods::firePct, 0.05f).d(), 1, 70);
    eq("a_moon",   QStringLiteral("Подвеска луны"),      Slot_Amulet, 3, Gl_Amulet, 0,0,1,Dmg_Frost,    MB().s(&StatMods::focus, 6).s(&StatMods::manaRegen, 1.2f).d(), 1, 175);
    eq("a_eye",    QStringLiteral("Око пустоты"),        Slot_Amulet, 4, Gl_Amulet, 0,0,1,Dmg_Arcane,   MB().s(&StatMods::critChance, 0.03f).s(&StatMods::critMult, 0.15f).d(), 2, 400);
    eq("a_heart",  QStringLiteral("Сердце пепла"),       Slot_Amulet, 5, Gl_Amulet, 0,0,1,Dmg_Fire,     MB().s(&StatMods::dmgPct, 0.10f).s(&StatMods::lifeSteal, 0.02f).d(), 2, 850);
    eq("a_star",   QStringLiteral("Осколок звезды"),     Slot_Amulet, 6, Gl_Amulet, 0,0,1,Dmg_Arcane,   MB().s(&StatMods::dmgPct, 0.16f).s(&StatMods::critMult, 0.30f).s(&StatMods::cdReduction, 0.06f).d(), 3, 1900);

    // ---------- КОЛЬЦА ----------
    eq("r_copper", QStringLiteral("Медное кольцо"),      Slot_Ring1, 1, Gl_Ring, 0,0,1,Dmg_Physical, MB().s(&StatMods::hpFlat, 6).d(), 0, 18);
    eq("r_amber",  QStringLiteral("Кольцо с янтарём"),   Slot_Ring1, 2, Gl_Ring, 0,0,1,Dmg_Fire,     MB().s(&StatMods::firePct, 0.06f).d(), 0, 68);
    eq("r_seal",   QStringLiteral("Перстень печати"),    Slot_Ring1, 3, Gl_Ring, 0,0,1,Dmg_Arcane,   MB().s(&StatMods::armor, 20).s(&StatMods::resAll, 0.04f).d(), 1, 170);
    eq("r_abyss",  QStringLiteral("Кольцо бездны"),      Slot_Ring1, 4, Gl_Ring, 0,0,1,Dmg_Arcane,   MB().s(&StatMods::arcanePct, 0.10f).s(&StatMods::manaFlat, 30).d(), 1, 390);
    eq("r_flame",  QStringLiteral("Кольцо вечного пламени"),Slot_Ring1,5, Gl_Ring,0,0,1,Dmg_Fire,    MB().s(&StatMods::firePct, 0.14f).s(&StatMods::critChance, 0.025f).d(), 2, 830);

    // ---------- ЗЕЛЬЯ / КАМНИ / ПРОЧЕЕ ----------
    add("p_hp1", QStringLiteral("Малое зелье исцеления"), Slot_Weapon, Bag_Potion, 1, Gl_PotionHp, 0,0,1,Dmg_Physical, MB().s(&StatMods::hpFlat, 60).d(), 0, 25);
    add("p_hp2", QStringLiteral("Зелье исцеления"),       Slot_Weapon, Bag_Potion, 2, Gl_PotionHp, 0,0,1,Dmg_Physical, MB().s(&StatMods::hpFlat, 160).d(), 0, 60);
    add("p_hp3", QStringLiteral("Большое зелье исцеления"),Slot_Weapon,Bag_Potion, 3, Gl_PotionHp, 0,0,1,Dmg_Physical, MB().s(&StatMods::hpFlat, 400).d(), 0, 150);
    add("p_mp1", QStringLiteral("Зелье маны"),            Slot_Weapon, Bag_Potion, 1, Gl_PotionMp, 0,0,1,Dmg_Physical, MB().s(&StatMods::manaFlat, 50).d(), 0, 25);
    add("p_mp2", QStringLiteral("Большое зелье маны"),    Slot_Weapon, Bag_Potion, 3, Gl_PotionMp, 0,0,1,Dmg_Physical, MB().s(&StatMods::manaFlat, 160).d(), 0, 70);
    add("p_rage",QStringLiteral("Эликсир ярости"),        Slot_Weapon, Bag_Potion, 2, Gl_Flask,    0,0,1,Dmg_Physical, MB().s(&StatMods::attackSpeed, 0.25f).s(&StatMods::dmgPct, 0.15f).d(), 0, 90);
    // САМОЦВЕТЫ убраны из игры: больше не падают, не продаются, гнёзд нет.
    // Записи оставлены в таблице ТОЛЬКО ради стабильности id старых сейвов;
    // при загрузке такие предметы вычищаются (см. Game::loadGame).
    add("g_ruby",QStringLiteral("Рубин"),                 Slot_Weapon, Bag_Gem, 1, Gl_GemRed,    0,0,1,Dmg_Physical, MB().s(&StatMods::hpFlat, 25).d(), 0, 40);
    add("g_saph",QStringLiteral("Сапфир"),                Slot_Weapon, Bag_Gem, 1, Gl_GemBlue,   0,0,1,Dmg_Physical, MB().s(&StatMods::manaFlat, 22).s(&StatMods::focus, 2).d(), 0, 40);
    add("g_topz",QStringLiteral("Топаз"),                 Slot_Weapon, Bag_Gem, 1, Gl_GemYellow, 0,0,1,Dmg_Physical, MB().s(&StatMods::might, 3).d(), 0, 45);
    add("g_emer",QStringLiteral("Изумруд"),               Slot_Weapon, Bag_Gem, 1, Gl_GemGreen,  0,0,1,Dmg_Physical, MB().s(&StatMods::critChance, 0.02f).s(&StatMods::agi, 2).d(), 0, 50);
    add("g_amet",QStringLiteral("Аметист"),               Slot_Weapon, Bag_Gem, 1, Gl_GemViolet, 0,0,1,Dmg_Physical, MB().s(&StatMods::resAll, 0.03f).s(&StatMods::spirit, 2).d(), 0, 48);
    add("m_tp",  QStringLiteral("Свиток возвращения"),    Slot_Weapon, Bag_Misc, 1, Gl_Scroll,   0,0,1,Dmg_Physical, StatMods(), 0, 30);
    add("m_soul",QStringLiteral("Осколок души"),          Slot_Weapon, Bag_Misc, 1, Gl_Star,     0,0,1,Dmg_Physical, StatMods(), 0, 250);
}

// ----------------------------------------------------------------
//  АФФИКСЫ
// ----------------------------------------------------------------
static void loadAffixes()
{
    g_affixes.clear();
    int id = 0;
    auto add = [&](const QString &sid, const QString &name, bool prefix, int minIlvl, int mask,
                   const StatMods &m, float rmin, float rmax, int w)
    {
        AffixDef a;
        a.id = id++;
        a.id_str = sid;
        a.name = name;
        a.prefix = prefix;
        a.minIlvl = minIlvl;
        a.slotMask = mask;
        a.mods = m;
        a.rollMin = rmin;
        a.rollMax = rmax;
        a.weight = w;
        g_affixes.append(a);
    };

    // --- префиксы урона (оружие) ---
    add("brutal",  QStringLiteral("Жестокий"),    true, 1,  MSK_WEAPON, MB().s(&StatMods::physPct, 0.10f).d(), 0.7f, 1.4f, 100);
    add("flaming", QStringLiteral("Пылающий"),    true, 2,  MSK_WEAPON | MSK_JEWEL, MB().s(&StatMods::firePct, 0.11f).d(), 0.7f, 1.4f, 90);
    add("frozen",  QStringLiteral("Морозный"),    true, 2,  MSK_WEAPON | MSK_JEWEL, MB().s(&StatMods::frostPct, 0.11f).d(), 0.7f, 1.4f, 90);
    add("storm",   QStringLiteral("Грозовой"),    true, 3,  MSK_WEAPON | MSK_JEWEL, MB().s(&StatMods::lightPct, 0.11f).d(), 0.7f, 1.4f, 85);
    add("venom",   QStringLiteral("Ядовитый"),    true, 3,  MSK_WEAPON, MB().s(&StatMods::poisonPct, 0.12f).d(), 0.7f, 1.4f, 80);
    add("hexed",   QStringLiteral("Заколдованны"),true, 4,  MSK_WEAPON | MSK_OFF, MB().s(&StatMods::arcanePct, 0.12f).d(), 0.7f, 1.4f, 80);
    add("keen",    QStringLiteral("Точный"),      true, 1,  MSK_WEAPON, MB().s(&StatMods::critChance, 0.03f).d(), 0.7f, 1.3f, 90);
    add("swift",   QStringLiteral("Стремительный"),true, 2, MSK_WEAPON | MSK_ARMOR, MB().s(&StatMods::attackSpeed, 0.07f).d(), 0.7f, 1.3f, 85);
    add("vampiric",QStringLiteral("Вампирский"),  true, 6,  MSK_WEAPON, MB().s(&StatMods::lifeSteal, 0.03f).d(), 0.7f, 1.4f, 55);
    add("crushing",QStringLiteral("Сокрушительный"),true,3, MSK_WEAPON, MB().s(&StatMods::might, 5).s(&StatMods::physPct, 0.05f).d(), 0.7f, 1.3f, 80);
    add("searing", QStringLiteral("Испепеляющий"),true, 8,  MSK_WEAPON, MB().s(&StatMods::firePct, 0.16f).s(&StatMods::dmgPct, 0.05f).d(), 0.7f, 1.3f, 45);
    add("piercing",QStringLiteral("Пронзающий"),  true, 5,  MSK_WEAPON, MB().s(&StatMods::projSpeed, 0.20f).s(&StatMods::critMult, 0.10f).d(), 0.7f, 1.3f, 70);
    add("echoing", QStringLiteral("Эхо"),         true, 12, MSK_WEAPON, MB().s(&StatMods::extraProj, 1).d(), 1.0f, 1.0f, 25);

    // --- защитные префиксы ---
    add("sturdy",  QStringLiteral("Крепкий"),     true, 1,  MSK_ARMOR | MSK_OFF, MB().s(&StatMods::hpFlat, 22).d(), 0.7f, 1.4f, 100);
    add("reinforced",QStringLiteral("Укреплённый"),true,2, MSK_ARMOR | MSK_OFF, MB().s(&StatMods::armor, 26).d(), 0.7f, 1.4f, 95);
    add("nimble",  QStringLiteral("Ловкий"),      true, 1,  MSK_NOBELT, MB().s(&StatMods::agi, 5).d(), 0.7f, 1.4f, 90);
    add("sage",    QStringLiteral("Мудрый"),      true, 1,  MSK_NOBELT, MB().s(&StatMods::focus, 5).d(), 0.7f, 1.4f, 90);
    add("spirit",  QStringLiteral("Духовный"),    true, 2,  MSK_NOBELT, MB().s(&StatMods::spirit, 5).d(), 0.7f, 1.4f, 88);
    add("mighty",  QStringLiteral("Могучий"),     true, 2,  MSK_NOBELT, MB().s(&StatMods::might, 5).d(), 0.7f, 1.4f, 88);
    add("hardy",   QStringLiteral("Живучий"),     true, 3,  MSK_ARMOR | MSK_JEWEL, MB().s(&StatMods::vit, 6).d(), 0.7f, 1.4f, 85);
    add("fleet",   QStringLiteral("Быстрый"),     true, 2,  MSK_ARMOR, MB().s(&StatMods::moveSpeed, 0.06f).d(), 0.7f, 1.3f, 70);
    add("warded",  QStringLiteral("Хранимый"),    true, 5,  MSK_ARMOR | MSK_OFF | MSK_JEWEL, MB().s(&StatMods::resAll, 0.05f).d(), 0.7f, 1.3f, 60);
    add("fireproof",QStringLiteral("Огнестойкий"),true, 3, MSK_ARMOR | MSK_OFF, MB().s(&StatMods::resFire, 0.12f).d(), 0.7f, 1.3f, 75);
    add("frostward",QStringLiteral("Морозостойкий"),true,3, MSK_ARMOR | MSK_OFF, MB().s(&StatMods::resFrost, 0.12f).d(), 0.7f, 1.3f, 75);
    add("grounded",QStringLiteral("Заземлённый"), true, 3,  MSK_ARMOR | MSK_OFF, MB().s(&StatMods::resLight, 0.12f).d(), 0.7f, 1.3f, 75);
    add("pure",    QStringLiteral("Очищенный"),   true, 4,  MSK_ARMOR | MSK_OFF, MB().s(&StatMods::resPoison, 0.12f).d(), 0.7f, 1.3f, 72);
    add("veiled",  QStringLiteral("Завуалированный"),true,6,MSK_ARMOR | MSK_JEWEL, MB().s(&StatMods::resArcane, 0.12f).d(), 0.7f, 1.3f, 68);
    add("glimmering",QString::fromUtf8("Мерцающий"),true,7,MSK_ARMOR|MSK_JEWEL, MB().s(&StatMods::shieldFlat, 22).d(), 0.7f, 1.4f, 55);
    add("runed",   QStringLiteral("Рунный"),      true, 6,  MSK_ALL_EQ, MB().s(&StatMods::cdReduction, 0.06f).d(), 0.7f, 1.3f, 55);

    // --- суффиксы ---
    add("bear",    QStringLiteral("медведя"),     false, 1, MSK_ALL_EQ, MB().s(&StatMods::vit, 4).s(&StatMods::hpFlat, 12).d(), 0.7f, 1.4f, 95);
    add("tiger",   QStringLiteral("тигра"),       false, 1, MSK_ALL_EQ, MB().s(&StatMods::might, 4).d(), 0.7f, 1.4f, 95);
    add("fox",     QStringLiteral("лисы"),        false, 2, MSK_ALL_EQ, MB().s(&StatMods::agi, 4).s(&StatMods::dodge, 0.01f).d(), 0.7f, 1.4f, 90);
    add("owl",     QStringLiteral("совы"),        false, 2, MSK_ALL_EQ, MB().s(&StatMods::focus, 4).s(&StatMods::manaFlat, 10).d(), 0.7f, 1.4f, 90);
    add("wolf",    QStringLiteral("волка"),       false, 3, MSK_ALL_EQ, MB().s(&StatMods::spirit, 4).s(&StatMods::dmgPct, 0.02f).d(), 0.7f, 1.4f, 88);
    add("whale",   QStringLiteral("кита"),        false, 5, MSK_ARMOR | MSK_JEWEL | MSK_OFF, MB().s(&StatMods::hpFlat, 40).d(), 0.7f, 1.4f, 75);
    add("golem",   QStringLiteral("голема"),      false, 4, MSK_ARMOR | MSK_OFF, MB().s(&StatMods::armor, 34).s(&StatMods::armorPct, 0.05f).d(), 0.7f, 1.4f, 78);
    add("eagle",   QStringLiteral("орла"),        false, 3, MSK_ALL_EQ, MB().s(&StatMods::critChance, 0.025f).d(), 0.7f, 1.3f, 82);
    add("wind",    QStringLiteral("ветра"),       false, 2, MSK_ARMOR | MSK_WEAPON, MB().s(&StatMods::moveSpeed, 0.05f).d(), 0.7f, 1.3f, 80);
    add("regen",   QStringLiteral("возрождения"), false, 3, MSK_ARMOR | MSK_JEWEL, MB().s(&StatMods::hpRegen, 2.2f).d(), 0.7f, 1.4f, 78);
    add("mana",    QStringLiteral("потока"),      false, 2, MSK_ARMOR | MSK_JEWEL | MSK_OFF, MB().s(&StatMods::manaRegen, 1.4f).d(), 0.7f, 1.4f, 80);
    add("leech",   QStringLiteral("пиявки"),      false, 7, MSK_WEAPON | MSK_JEWEL, MB().s(&StatMods::lifeSteal, 0.022f).d(), 0.7f, 1.4f, 50);
    add("luck",    QStringLiteral("удачи"),       false, 2, MSK_ALL_EQ, MB().s(&StatMods::magicFind, 0.10f).d(), 0.7f, 1.4f, 70);
    add("greed",   QStringLiteral("алчности"),    false, 2, MSK_ALL_EQ, MB().s(&StatMods::goldFind, 0.12f).d(), 0.7f, 1.4f, 72);
    add("ruin",    QStringLiteral("разрушения"),  false, 5, MSK_WEAPON | MSK_JEWEL, MB().s(&StatMods::critMult, 0.14f).d(), 0.7f, 1.3f, 65);
    add("thorns",  QStringLiteral("шипов"),       false, 4, MSK_ARMOR | MSK_OFF, MB().s(&StatMods::thorns, 14).d(), 0.7f, 1.4f, 68);
    add("wizard",  QStringLiteral("чародея"),     false, 3, MSK_WEAPON | MSK_OFF | MSK_JEWEL, MB().s(&StatMods::castSpeed, 0.06f).d(), 0.7f, 1.3f, 75);
    add("berserk", QStringLiteral("берсерка"),    false, 4, MSK_WEAPON | MSK_ARMOR, MB().s(&StatMods::attackSpeed, 0.06f).d(), 0.7f, 1.3f, 74);
    add("scholar", QStringLiteral("учёного"),     false, 3, MSK_ALL_EQ, MB().s(&StatMods::xpGain, 0.08f).d(), 0.7f, 1.4f, 70);
    add("medic",   QStringLiteral("лекаря"),      false, 3, MSK_ALL_EQ, MB().s(&StatMods::potionPotency, 0.20f).d(), 0.7f, 1.4f, 62);
    add("summon",  QStringLiteral("призывателя"), false, 6, MSK_WEAPON | MSK_OFF | MSK_JEWEL, MB().s(&StatMods::summonPct, 0.18f).d(), 0.7f, 1.4f, 55);
    add("elements",QStringLiteral("стихий"),      false, 6, MSK_ALL_EQ, MB().s(&StatMods::firePct, 0.06f).s(&StatMods::frostPct, 0.06f).s(&StatMods::lightPct, 0.06f).d(), 0.7f, 1.3f, 58);
    add("titan",   QStringLiteral("титана"),      false, 10, MSK_ARMOR | MSK_JEWEL, MB().s(&StatMods::hpPct, 0.07f).d(), 0.7f, 1.3f, 45);
    add("chain",   QStringLiteral("цепи"),        false, 9, MSK_WEAPON, MB().s(&StatMods::chainBonus, 1).d(), 1.0f, 2.0f, 30);
    add("flare",   QStringLiteral("вспышки"),     false, 5, MSK_WEAPON | MSK_JEWEL, MB().s(&StatMods::areaPct, 0.12f).d(), 0.7f, 1.3f, 66);
    // скоростные аффиксы дописаны в конец таблицы: индексы аффиксов хранятся в сейвах
    add("frenzied",QStringLiteral("Неистовый"),   true, 5,  MSK_WEAPON, MB().s(&StatMods::attackSpeed, 0.11f).d(), 0.7f, 1.3f, 62);
    add("gale",    QStringLiteral("урагана"),     false, 6, MSK_WEAPON | MSK_JEWEL, MB().s(&StatMods::attackSpeed, 0.09f).d(), 0.7f, 1.3f, 58);
}

// ----------------------------------------------------------------
//  УНИКАЛЬНЫЕ ПРЕДМЕТЫ
// ----------------------------------------------------------------
static void loadUniques()
{
    g_uniques.clear();
    int id = 0;
    auto add = [&](const QString &sid, const QString &name, const QString &flavor, const QString &baseSid,
                   int req, const QList<int> &aff, const QList<float> &rolls, const QColor &tint)
    {
        UniqueDef u;
        u.id = id++;
        u.id_str = sid;
        u.name = name;
        u.flavor = flavor;
        u.reqLevel = req;
        u.affixes = aff;
        u.rolls = rolls;
        u.tint = tint;
        for (int i = 0; i < g_bases.size(); ++i)
            if (g_bases[i].id_str == baseSid) { u.baseId = g_bases[i].id; break; }
        g_uniques.append(u);
    };
    auto idx = [](const QString &sid) -> int {
        for (int i = 0; i < g_affixes.size(); ++i) if (g_affixes[i].id_str == sid) return g_affixes[i].id;
        return -1;
    };

    add("u_ashfather", QStringLiteral("Клык Пепельного Отца"),
        QStringLiteral("Ещё тёплый. Он помнит вкус своего хозяина."), "w_exec", 8,
        QList<int>() << idx("brutal") << idx("searing") << idx("leech") << idx("ruin"),
        QList<float>() << 1.6f << 1.4f << 1.3f << 1.2f, C::ember);
    add("u_widow", QStringLiteral("Жало Вдовы"),
        QStringLiteral("Она плела из него паутину. Теперь плетёшь ты."), "w_night", 14,
        QList<int>() << idx("venom") << idx("keen") << idx("swift") << idx("chain"),
        QList<float>() << 1.5f << 1.6f << 1.3f << 1.0f, C::poison);
    add("u_vyug", QStringLiteral("Сердце Вьюговея"),
        QStringLiteral("Лёд, который бьётся. Медленно. Очень медленно."), "w_frostspr", 16,
        QList<int>() << idx("frozen") << idx("frostward") << idx("golem") << idx("flare"),
        QList<float>() << 1.7f << 1.5f << 1.4f << 1.3f, C::frost);
    add("u_colossus", QStringLiteral("Гнилая Печать"),
        QStringLiteral("Колосс носил её в груди. Ты носишь её в руке."), "o_abyssh", 20,
        QList<int>() << idx("reinforced") << idx("pure") << idx("whale") << idx("thorns"),
        QList<float>() << 1.8f << 1.6f << 1.5f << 1.5f, C::poison);
    add("u_star", QStringLiteral("Осколок Пожирателя"),
        QStringLiteral("Звезда, которую так и не успели съесть."), "a_star", 26,
        QList<int>() << idx("hexed") << idx("ruin") << idx("eagle") << idx("chain"),
        QList<float>() << 1.9f << 1.7f << 1.8f << 1.0f, C::arcane);
    add("u_crown", QStringLiteral("Корона Патриарха"),
        QStringLiteral("Пепел не знает царей. Но корону оставил."), "h_ashmask", 10,
        QList<int>() << idx("fireproof") << idx("hardy") << idx("spirit") << idx("scholar"),
        QList<float>() << 1.7f << 1.5f << 1.4f << 1.5f, C::gold);
    add("u_rootmother", QStringLiteral("Корень-Мать"),
        QStringLiteral("Прорастает сквозь кожу. Не выдирай."), "c_scale", 13,
        QList<int>() << idx("sturdy") << idx("regen") << idx("golem") << idx("titan"),
        QList<float>() << 1.6f << 1.8f << 1.4f << 1.5f, C::poison);
    add("u_forge", QStringLiteral("Рукавицы Горнильщика"),
        QStringLiteral("В них ковали первое оружие этого мира. И последнее."), "g_forge", 30,
        QList<int>() << idx("searing") << idx("berserk") << idx("crushing") << idx("ruin"),
        QList<float>() << 1.8f << 1.6f << 1.5f << 1.6f, C::ember);
    add("u_windstep", QStringLiteral("Поступь Ветра"),
        QStringLiteral("Следов не остаётся. Только пепел вздрагивает."), "t_wind", 18,
        QList<int>() << idx("fleet") << idx("wind") << idx("fox") << idx("eagle"),
        QList<float>() << 1.8f << 1.6f << 1.5f << 1.4f, C::frost);
    add("u_bloodseal", QStringLiteral("Печать Крови"),
        QStringLiteral("Каждый удар возвращает тебе немного чужой жизни."), "r_flame", 22,
        QList<int>() << idx("vampiric") << idx("leech") << idx("bear") << idx("titan"),
        QList<float>() << 1.9f << 1.8f << 1.5f << 1.6f, C::blood);
    add("u_voidtome", QStringLiteral("Гримуар Пустоты"),
        QStringLiteral("Читается сам. Ты только держи."), "o_startome", 24,
        QList<int>() << idx("hexed") << idx("wizard") << idx("mana") << idx("summon"),
        QList<float>() << 1.8f << 1.6f << 1.7f << 1.5f, C::arcane);
    add("u_ashbelt", QStringLiteral("Опояс Первого Угля"),
        QStringLiteral("С него начался пожар, поглотивший мир."), "b_ash", 28,
        QList<int>() << idx("sturdy") << idx("fireproof") << idx("thorns") << idx("regen"),
        QList<float>() << 1.9f << 1.8f << 1.7f << 1.8f, C::ember);
}

// ----------------------------------------------------------------
//  АКТИВНЫЕ УМЕНИЯ
// ----------------------------------------------------------------
static void loadSkills()
{
    g_skills.clear();
    int id = 0;
    auto add = [&](const QString &sid, const QString &name, const QString &desc, SkillFx fx, DmgType dt,
                   float cost, float cd, float dmgPct, float radius, float range, float projSpd,
                   float dur, int proj, int pierce, int chains, int req, int glyph, const QColor &col, float scale)
    {
        ActiveSkill s;
        s.id = id++;
        s.id_str = sid;
        s.name = name;
        s.desc = desc;
        s.fx = fx;
        s.dmgType = dt;
        s.cost = cost;
        s.cd = cd;
        s.dmgPct = dmgPct;
        s.radius = radius;
        s.range = range;
        s.projSpeed = projSpd;
        s.duration = dur;
        s.projectiles = proj;
        s.pierce = pierce;
        s.chains = chains;
        s.reqLevel = req;
        s.maxRank = 5;
        s.glyph = glyph;
        s.color = col;
        s.scale = scale;
        g_skills.append(s);
    };

    add("cleave", QStringLiteral("Рассекающий удар"),
        QStringLiteral("Широкий взмах, задевающий всех перед тобой."),
        Fx_Cleave, Dmg_Physical, 5, 2.4f, 1.35f, 78, 92, 0, 0, 1, 0, 0, 1, Gl_Cleave, C::bone, 0);
    add("whirl", QStringLiteral("Вихрь клинков"),
        QStringLiteral("Крутись, рубя всех вокруг. Можно двигаться."),
        Fx_Spin, Dmg_Physical, 3, 0.9f, 0.62f, 92, 0, 0, 3.0f, 1, 0, 0, 4, Gl_Spin, C::boneDim, 0);
    add("dash", QStringLiteral("Теневой рывок"),
        QStringLiteral("Бросок сквозь врагов с уроном и неуязвимостью."),
        Fx_Dash, Dmg_Physical, 8, 4.0f, 1.10f, 46, 240, 900, 0, 1, 99, 0, 2, Gl_Dash, C::arcane, 0);
    add("fireball", QStringLiteral("Огненный шар"),
        QStringLiteral("Снаряд, взрывающийся при попадании."),
        Fx_Projectile, Dmg_Fire, 12, 3.2f, 1.60f, 86, 480, 460, 0, 1, 0, 0, 1, Gl_Fireball, C::ember, 0.35f);
    add("meteor", QStringLiteral("Метеор"),
        QStringLiteral("Через миг с неба упадёт раскалённая глыба."),
        Fx_Meteor, Dmg_Fire, 26, 7.0f, 2.60f, 120, 520, 0, 0, 1, 0, 0, 8, Gl_Meteor, C::ember, 0.5f);
    add("iceshard", QStringLiteral("Ледяное копьё"),
        QStringLiteral("Пробивает насквозь и замедляет врагов."),
        Fx_Pierce, Dmg_Frost, 10, 2.6f, 1.30f, 40, 520, 620, 0, 1, 2, 0, 3, Gl_IceShard, C::frost, 0.4f);
    add("chain", QStringLiteral("Цепная молния"),
        QStringLiteral("Разряд перескакивает между врагами."),
        Fx_Chain, Dmg_Lightning, 16, 3.6f, 1.15f, 130, 460, 0, 0, 1, 0, 3, 6, Gl_Chain, C::lightning, 0.3f);
    add("nova", QStringLiteral("Кольцо гнева"),
        QStringLiteral("Взрывная волна во все стороны от тебя."),
        Fx_Nova, Dmg_Arcane, 18, 4.4f, 1.45f, 150, 0, 0, 0, 1, 0, 0, 5, Gl_Nova, C::arcane, 0);
    add("plague", QStringLiteral("Чумное облако"),
        QStringLiteral("Ядовитая зона, отравляющая всех внутри."),
        Fx_Zone, Dmg_Poison, 20, 6.0f, 0.55f, 110, 420, 0, 6.0f, 1, 0, 0, 7, Gl_Poison, C::poison, 0.6f);
    add("summon", QStringLiteral("Теневой клинок"),
        QStringLiteral("Призывает призрачного воина, сражающегося за тебя."),
        Fx_Summon, Dmg_Physical, 28, 12.0f, 0.85f, 0, 90, 0, 14.0f, 1, 0, 0, 9, Gl_Summon, C::arcane, 0);
    add("ward", QStringLiteral("Рунный оберег"),
        QStringLiteral("Поглощающий барьер, пока его не пробьют."),
        Fx_Shield, Dmg_Arcane, 22, 9.0f, 0, 0, 0, 0, 8.0f, 1, 0, 0, 4, Gl_Ward, C::manaHi, 0);
    add("mend", QStringLiteral("Исцеление пеплом"),
        QStringLiteral("Восстанавливает здоровье в течение времени."),
        Fx_Heal, Dmg_Pure, 24, 8.0f, 0, 0, 0, 0, 5.0f, 1, 0, 0, 3, Gl_Heal, C::good, 0);
    add("volley", QStringLiteral("Град стрел"),
        QStringLiteral("Веер из нескольких снарядов."),
        Fx_MultiShot, Dmg_Physical, 14, 3.0f, 0.72f, 34, 480, 560, 0, 5, 0, 0, 5, Gl_MultiArrow, C::bone, 0);
    add("quake", QStringLiteral("Землетрясение"),
        QStringLiteral("Ударные волны расходятся по земле."),
        Fx_Quake, Dmg_Physical, 24, 6.5f, 1.30f, 0, 0, 0, 0, 3, 0, 0, 10, Gl_Quake, C::ember, 0);
    add("mark", QStringLiteral("Метка смерти"),
        QStringLiteral("Отмеченный враг получает заметно больше урона."),
        Fx_Mark, Dmg_Arcane, 12, 5.0f, 0.20f, 0, 480, 900, 7.0f, 1, 0, 0, 6, Gl_Mark, C::blood, 0.30f);
    add("bloodlust", QStringLiteral("Жажда крови"),
        QStringLiteral("Ярость: скорость атаки, урон и вампиризм."),
        Fx_Buff, Dmg_Pure, 20, 14.0f, 0, 0, 0, 0, 7.0f, 1, 0, 0, 12, Gl_Blood, C::bloodHi, 0);
    add("blink", QStringLiteral("Скачок"),
        QStringLiteral("Мгновенное перемещение к курсору."),
        Fx_Dash, Dmg_Arcane, 10, 3.0f, 0, 0, 320, 0, 0, 1, 0, 0, 2, Gl_Blink, C::arcane, 0);
}

// ----------------------------------------------------------------
//  ПАУТИНА НАВЫКОВ
// ----------------------------------------------------------------
static void loadNodes()
{
    g_nodes.clear();

    // ветви: 0 Воин(верх), 1 Маг(право), 2 Странник(низ), 3 Жрец Пепла(лево)
    // диагонали - гибридные (branch = -1)
    struct Spec {
        int ring; int ang; NodeKind kind; int branch;
        const char *name; const char *desc;
        StatMods mods; int cost; int maxRank; int req; int active; const char *keystone; QColor col;
    };
    QVector<Spec> specs;
    auto S = [&](int ring, int ang, NodeKind k, int br, const char *nm, const char *ds,
                 const StatMods &m, int cost, int maxr, int req, int act, const char *ks, const QColor &col)
    {
        Spec s; s.ring = ring; s.ang = ang; s.kind = k; s.branch = br;
        s.name = nm; s.desc = ds; s.mods = m; s.cost = cost; s.maxRank = maxr;
        s.req = req; s.active = act; s.keystone = ks; s.col = col;
        specs.append(s);
    };

    const QColor cWar(226, 122,  74);
    const QColor cMag(122, 156, 246);
    const QColor cRog(146, 216, 140);
    const QColor cPri(206, 150, 236);
    const QColor cMix(216, 190, 130);

    // ---- ЦЕНТР ----
    S(0, 0, Nk_Origin, -1, "Исток", "Начало паутины. Отсюда расходятся все нити.",
      MB().s(&StatMods::hpFlat, 10).d(), 0, 1, 1, -1, "", C::amber);

    // ---- КОЛЬЦО 1 (8 спиц) ----
    S(1,  90, Nk_Minor, 0, "Крепость", "+12 к здоровью, +2 живучести", MB().s(&StatMods::hpFlat, 12).s(&StatMods::vit, 2).d(), 1, 3, 1, -1, "", cWar);
    S(1, 135, Nk_Active, 0, "Рассекающий удар", "Умение: широкий взмах перед собой.", StatMods(), 1, 1, 1, 0, "", cWar);
    S(1, 180, Nk_Minor, 1, "Ясность", "+10 маны, +2 концентрации", MB().s(&StatMods::manaFlat, 10).s(&StatMods::focus, 2).d(), 1, 3, 1, -1, "", cMag);
    S(1, 225, Nk_Active, 1, "Огненный шар", "Умение: взрывной снаряд.", StatMods(), 1, 1, 1, 3, "", cMag);
    S(1, 270, Nk_Minor, 2, "Лёгкость", "+4% скорости передвижения, +2 ловкости", MB().s(&StatMods::moveSpeed, 0.04f).s(&StatMods::agi, 2).d(), 1, 3, 1, -1, "", cRog);
    S(1, 315, Nk_Active, 2, "Скачок", "Умение: мгновенное перемещение.", StatMods(), 1, 1, 2, 16, "", cRog);
    S(1,   0, Nk_Minor, 3, "Тление", "+3% урона, +2 духа", MB().s(&StatMods::dmgPct, 0.03f).s(&StatMods::spirit, 2).d(), 1, 3, 1, -1, "", cPri);
    S(1,  45, Nk_Active, 3, "Исцеление пеплом", "Умение: лечение со временем.", StatMods(), 1, 1, 3, 11, "", cPri);

    // ---- КОЛЬЦО 2 ----
    S(2,  90, Nk_Minor, 0, "Стальной хват", "+4 мощи, +6% физ. урона", MB().s(&StatMods::might, 4).s(&StatMods::physPct, 0.06f).d(), 1, 3, 3, -1, "", cWar);
    S(2, 112, Nk_Minor, 0, "Толстая шкура", "+18 брони, +5 здоровья", MB().s(&StatMods::armor, 18).s(&StatMods::hpFlat, 5).d(), 1, 3, 3, -1, "", cWar);
    S(2, 135, Nk_Active, 0, "Вихрь клинков", "Умение: вращение с уроном вокруг.", StatMods(), 1, 1, 4, 1, "", cWar);
    S(2, 157, Nk_Notable, 0, "Рассечение", "+18% к области атак, +8% физ. урона", MB().s(&StatMods::areaPct, 0.18f).s(&StatMods::physPct, 0.08f).d(), 2, 1, 5, -1, "", cWar);
    S(2, 180, Nk_Minor, 1, "Поток", "+8% скорости заклинаний, +2 концентрации", MB().s(&StatMods::castSpeed, 0.08f).s(&StatMods::focus, 2).d(), 1, 3, 3, -1, "", cMag);
    S(2, 202, Nk_Minor, 1, "Руны отката", "+7% сокращения отката", MB().s(&StatMods::cdReduction, 0.07f).d(), 1, 3, 3, -1, "", cMag);
    S(2, 225, Nk_Active, 1, "Ледяное копьё", "Умение: пробивающий снаряд, замедляет.", StatMods(), 1, 1, 3, 5, "", cMag);
    S(2, 247, Nk_Notable, 1, "Средоточие", "+12% элементального урона, +14 маны", MB().s(&StatMods::firePct, 0.04f).s(&StatMods::frostPct, 0.04f).s(&StatMods::lightPct, 0.04f).s(&StatMods::manaFlat, 14).d(), 2, 1, 5, -1, "", cMag);
    S(2, 270, Nk_Minor, 2, "Точный глаз", "+2.5% шанса крита", MB().s(&StatMods::critChance, 0.025f).d(), 1, 3, 3, -1, "", cRog);
    S(2, 292, Nk_Minor, 2, "Быстрые руки", "+7% скорости атаки", MB().s(&StatMods::attackSpeed, 0.07f).d(), 1, 3, 3, -1, "", cRog);
    S(2, 315, Nk_Active, 2, "Град стрел", "Умение: веер снарядов.", StatMods(), 1, 1, 5, 12, "", cRog);
    S(2, 337, Nk_Notable, 2, "Уклонение", "+5% уклонения, +4% скорости", MB().s(&StatMods::dodge, 0.05f).s(&StatMods::moveSpeed, 0.04f).d(), 2, 1, 5, -1, "", cRog);
    S(2,   0, Nk_Minor, 3, "Жатва душ", "+3% вампиризма", MB().s(&StatMods::lifeSteal, 0.03f).d(), 1, 2, 3, -1, "", cPri);
    S(2,  22, Nk_Minor, 3, "Пепельная стойкость", "+6% ко всем сопротивлениям", MB().s(&StatMods::resAll, 0.06f).d(), 1, 3, 3, -1, "", cPri);
    S(2,  45, Nk_Active, 3, "Рунный оберег", "Умение: поглощающий барьер.", StatMods(), 1, 1, 4, 10, "", cPri);
    S(2,  67, Nk_Notable, 3, "Духовное восстановление", "+1.8 здоровья/сек, +1.2 маны/сек", MB().s(&StatMods::hpRegen, 1.8f).s(&StatMods::manaRegen, 1.2f).d(), 2, 1, 5, -1, "", cPri);

    // ---- КОЛЬЦО 3 ----
    S(3,  90, Nk_Notable, 0, "Мощь титана", "+8 мощи, +10% физ. урона, +20 здоровья", MB().s(&StatMods::might, 8).s(&StatMods::physPct, 0.10f).s(&StatMods::hpFlat, 20).d(), 2, 2, 8, -1, "", cWar);
    S(3, 120, Nk_Active, 0, "Теневой рывок", "Умение: рывок сквозь врагов.", StatMods(), 2, 1, 8, 2, "", cWar);
    S(3, 150, Nk_Notable, 0, "Разрушитель", "+25% множителя крит. урона", MB().s(&StatMods::critMult, 0.25f).d(), 2, 2, 9, -1, "", cWar);
    S(3, 180, Nk_Notable, 1, "Аркана", "+10 духа, +14% тайного урона", MB().s(&StatMods::spirit, 10).s(&StatMods::arcanePct, 0.14f).d(), 2, 2, 8, -1, "", cMag);
    S(3, 210, Nk_Active, 1, "Цепная молния", "Умение: разряд скачет между целями.", StatMods(), 2, 1, 6, 6, "", cMag);
    S(3, 240, Nk_Notable, 1, "Пиромант", "+18% огненного урона, +8% области", MB().s(&StatMods::firePct, 0.18f).s(&StatMods::areaPct, 0.08f).d(), 2, 2, 9, -1, "", cMag);
    S(3, 270, Nk_Notable, 2, "Тень ветра", "+8 ловкости, +8% скорости атаки, +3% уклонения", MB().s(&StatMods::agi, 8).s(&StatMods::attackSpeed, 0.08f).s(&StatMods::dodge, 0.03f).d(), 2, 2, 8, -1, "", cRog);
    S(3, 300, Nk_Active, 2, "Землетрясение", "Умение: ударные волны по земле.", StatMods(), 2, 1, 10, 13, "", cRog);
    S(3, 330, Nk_Notable, 2, "Смертельный удар", "+4% крита, +30% крит. урона", MB().s(&StatMods::critChance, 0.04f).s(&StatMods::critMult, 0.30f).d(), 2, 2, 9, -1, "", cRog);
    S(3,   0, Nk_Notable, 3, "Пожиратель", "+6% вампиризма, +10 духа", MB().s(&StatMods::lifeSteal, 0.06f).s(&StatMods::spirit, 10).d(), 2, 2, 8, -1, "", cPri);
    S(3,  30, Nk_Active, 3, "Чумное облако", "Умение: ядовитая зона.", StatMods(), 2, 1, 7, 8, "", cPri);
    S(3,  60, Nk_Notable, 3, "Призыватель теней", "+25% силы прислужников, +10 духа", MB().s(&StatMods::summonPct, 0.25f).s(&StatMods::spirit, 10).d(), 2, 2, 9, -1, "", cPri);

    // ---- КОЛЬЦО 3.5: сильные умения на диагоналях ----
    S(3, 105, Nk_Active, -1, "Метеор", "Умение: падение раскалённой глыбы.", StatMods(), 3, 1, 12, 4, "", cMix);
    S(3, 165, Nk_Active, -1, "Кольцо гнева", "Умение: взрывная волна вокруг.", StatMods(), 3, 1, 11, 7, "", cMix);
    S(3, 255, Nk_Active, -1, "Метка смерти", "Умение: враг получает больше урона.", StatMods(), 2, 1, 10, 14, "", cMix);
    S(3, 345, Nk_Active, -1, "Теневой клинок", "Умение: призывает призрачного воина.", StatMods(), 3, 1, 12, 9, "", cMix);
    S(3,  15, Nk_Active, -1, "Жажда крови", "Умение: ярость и вампиризм.", StatMods(), 3, 1, 13, 15, "", cMix);

    // ---- КОЛЬЦО 4: заметные узлы ----
    S(4,  90, Nk_Notable, 0, "Воля стали", "+35 брони, +40 здоровья, +6 живучести", MB().s(&StatMods::armor, 35).s(&StatMods::hpFlat, 40).s(&StatMods::vit, 6).d(), 3, 2, 14, -1, "", cWar);
    S(4, 180, Nk_Notable, 1, "Источник", "+50 маны, +15% скорости заклинаний, +8% отката", MB().s(&StatMods::manaFlat, 50).s(&StatMods::castSpeed, 0.15f).s(&StatMods::cdReduction, 0.08f).d(), 3, 2, 14, -1, "", cMag);
    S(4, 270, Nk_Notable, 2, "Призрачный шаг", "+12% скорости, +6% уклонения, +10% крита", MB().s(&StatMods::moveSpeed, 0.12f).s(&StatMods::dodge, 0.06f).s(&StatMods::critChance, 0.03f).d(), 3, 2, 14, -1, "", cRog);
    S(4,   0, Nk_Notable, 3, "Пепельная жатва", "+8% вампиризма, +30 здоровья, +8% ко всем сопротивлениям", MB().s(&StatMods::lifeSteal, 0.08f).s(&StatMods::hpFlat, 30).s(&StatMods::resAll, 0.08f).d(), 3, 2, 14, -1, "", cPri);

    // ---- КЛЮЧЕВЫЕ КАМНИ ----
    S(5,  90, Nk_Keystone, 0, "Несокрушимость",
      "Броня снижает и элементальный урон. +60% брони, +25 шипов.",
      MB().s(&StatMods::armorPct, 0.60f).s(&StatMods::thorns, 25).d(), 3, 1, 18, -1, "unbreakable", cWar);
    S(5, 180, Nk_Keystone, 1, "Перегрузка",
      "Крит заклинанием вызывает разряд по площади (45% урона). +40% крит. урона магией.",
      MB().s(&StatMods::critMult, 0.40f).d(), 3, 1, 18, -1, "overload", cMag);
    S(5, 270, Nk_Keystone, 2, "Танцующий с тенью",
      "Уклонение и рывок дают +35% скорости атаки на 4 сек. +8% уклонения.",
      MB().s(&StatMods::dodge, 0.08f).d(), 3, 1, 18, -1, "shadowdancer", cRog);
    S(5,   0, Nk_Keystone, 3, "Жатва пепла",
      "Убийство лечит 3% здоровья и даёт заряд ярости (+4% урона, до 10 зарядов).",
      MB().s(&StatMods::lifeSteal, 0.03f).d(), 3, 1, 18, -1, "ashenharvest", cPri);
    S(5, 135, Nk_Keystone, -1, "Раскалённая сталь",
      "Атаки добавляют огонь: +30% от физического урона.",
      MB().s(&StatMods::firePct, 0.15f).d(), 3, 1, 16, -1, "searedsteel", cMix);
    S(5, 225, Nk_Keystone, -1, "Сквозь время",
      "+20% сокращения отката, скорость атаки уменьшает откат умений.",
      MB().s(&StatMods::cdReduction, 0.20f).d(), 3, 1, 16, -1, "timewarp", cMix);
    S(5, 315, Nk_Keystone, -1, "Кровь и пепел",
      "При здоровье ниже 40%: +40% крита и +6% вампиризма.",
      MB().s(&StatMods::critChance, 0.05f).d(), 3, 1, 16, -1, "bloodash", cMix);
    S(5,  45, Nk_Keystone, -1, "Костяная корона",
      "+120 к барьеру, +40% силы прислужников.",
      MB().s(&StatMods::shieldFlat, 120).s(&StatMods::summonPct, 0.40f).d(), 3, 1, 16, -1, "bonecrown", cMix);

    // формируем узлы
    for (int i = 0; i < specs.size(); ++i) {
        const Spec &s = specs[i];
        SkillNode n;
        n.id = i;
        n.id_str = QStringLiteral("n%1").arg(i);
        n.name = QString::fromUtf8(s.name);
        n.desc = QString::fromUtf8(s.desc);
        n.kind = s.kind;
        n.ring = s.ring;
        n.branch = s.branch;
        n.ang = float(s.ang) * float(M_PI) / 180.0f;
        n.radius = ringRadius(s.ring);
        // сильные «смешанные» умения 3-го кольца вынесены на отдельный радиус
        // между 3-м и 4-м кругом - иначе они слипались с соседями по кольцу
        if (s.ring == 3 && s.branch == -1) n.radius = 0.775f;
        n.cost = s.cost;
        n.maxRank = s.maxRank;
        n.reqLevel = s.req;
        n.mods = s.mods;
        n.activeSkill = s.active;
        n.keystone = QString::fromUtf8(s.keystone);
        n.color = s.col;
        g_nodes.append(n);
    }

    // связи: к предыдущему кольцу на своей спице + соседям по кольцу
    for (int i = 0; i < g_nodes.size(); ++i) {
        SkillNode &n = g_nodes[i];
        if (n.ring == 0) continue;
        int bestPrev = -1;
        float bestD = 1e9f;
        int prevRing = n.ring == 5 ? 4 : n.ring - 1;
        for (int j = 0; j < g_nodes.size(); ++j) {
            if (g_nodes[j].ring != prevRing) continue;
            float d = angDiff(n.ang, g_nodes[j].ang);
            if (d < bestD) { bestD = d; bestPrev = j; }
        }
        if (bestPrev >= 0 && bestD < 0.9f) { n.links.append(bestPrev); g_nodes[bestPrev].links.append(i); }
        // соседи по кольцу
        for (int j = i + 1; j < g_nodes.size(); ++j) {
            if (g_nodes[j].ring != n.ring) continue;
            float d = angDiff(n.ang, g_nodes[j].ang);
            if (d < 0.42f) { n.links.append(j); g_nodes[j].links.append(i); }
        }
    }
    // центр связан со всем первым кольцом
    for (int j = 1; j < g_nodes.size(); ++j) {
        if (g_nodes[j].ring == 1 && !g_nodes[0].links.contains(j)) {
            g_nodes[0].links.append(j);
            g_nodes[j].links.append(0);
        }
    }
}

// ----------------------------------------------------------------
//  ВРАГИ
// ----------------------------------------------------------------
static void loadEnemies()
{
    g_enemies.clear();
    int id = 0;
    auto add = [&](const QString &sid, const QString &name, int lvl, AiKind ai, float hp, float dmin, float dmax,
                   DmgType dt, float armor, float speed, float radius, float atkRange, float atkCd,
                   int xp, int gmin, int gmax, float drop, int tint, int glyph, const EnemyDef &extra)
    {
        EnemyDef e = extra;
        e.id = id++;
        e.id_str = sid;
        e.name = name;
        e.level = lvl;
        e.ai = ai;
        e.hp = hp;
        e.dmgMin = dmin; e.dmgMax = dmax;
        e.dmgType = dt;
        e.armor = armor;
        e.speed = speed;
        e.radius = radius;
        e.atkRange = atkRange;
        e.atkCd = atkCd;
        e.xp = xp;
        e.goldMin = gmin; e.goldMax = gmax;
        e.dropChance = drop;
        e.tint = tint;
        e.glyph = glyph;
        g_enemies.append(e);
    };
    EnemyDef X; // пустой шаблон для доп. полей

    auto set = [&](int idx, const EnemyDef &patch) { EnemyDef &e = g_enemies[idx];
        e.detectRange = patch.detectRange ? patch.detectRange : e.detectRange;
        e.rangedProjSpeed = patch.rangedProjSpeed ? patch.rangedProjSpeed : e.rangedProjSpeed;
        e.flying = patch.flying; e.summoner = patch.summoner; e.summonId = patch.summonId;
        e.summonCount = patch.summonCount ? patch.summonCount : e.summonCount;
        e.explodes = patch.explodes; e.explodeRadius = patch.explodeRadius ? patch.explodeRadius : e.explodeRadius;
        e.boss = patch.boss; e.title = patch.title; e.phases = patch.phases ? patch.phases : e.phases;
        e.patternSet = patch.patternSet; e.scale = patch.scale ? patch.scale : e.scale;
    };

    // --- РЕГИОН 0: Пепельные равнины ---
    add("e_scav",  QStringLiteral("Пепельный падальщик"), 1, Ai_Chase,   34, 3, 5,  Dmg_Physical, 0,  92, 13, 26, 1.15f,  8, 1, 3, 0.10f, 0, Gl_Skull, X);
    add("e_arch",  QStringLiteral("Костяной лучник"),     2, Ai_Ranged,  28, 5, 8,  Dmg_Physical, 0,  80, 13, 250, 1.8f,  12, 2, 5, 0.12f, 1, Gl_Bone, X);
    add("e_beetle",QStringLiteral("Угольный жук"),        1, Ai_Swarm,   18, 2, 3,  Dmg_Fire,     0, 132,  9, 22, 0.9f,   5, 1, 2, 0.06f, 2, Gl_Spider, X);
    add("e_axer",  QStringLiteral("Топорщик пустоши"),    4, Ai_Charger, 82, 9, 14, Dmg_Physical, 6,  86, 17, 30, 1.6f,   26, 4, 9, 0.16f, 3, Gl_Axe, X);
    add("e_dustm", QStringLiteral("Праховый маг"),        5, Ai_Caster,  58, 12, 18,Dmg_Fire,     2,  72, 15, 300, 2.6f,  34, 6, 12, 0.18f, 4, Gl_Fireball, X);
    {
        EnemyDef p; p.rangedProjSpeed = 330; set(1, p);
        EnemyDef c; c.ai = Ai_Caster; c.detectRange = 380; set(4, c);
        EnemyDef b; b.boss = true; b.title = QStringLiteral("Пепельный Патриарх"); b.phases = 3; b.patternSet = 0; b.scale = 1.55f; b.detectRange = 900;
        add("b_ash", QStringLiteral("Патриарх Пепла"), 7, Ai_Boss, 1000, 14, 20, Dmg_Fire, 10, 96, 30, 46, 1.4f, 420, 60, 120, 1.0f, 5, Gl_Boss, b);
    }

    // --- РЕГИОН 1: Угольный лес ---
    add("e_fang",  QStringLiteral("Теневой клык"),        7, Ai_Chase,   76, 8, 12, Dmg_Physical, 4, 138, 14, 26, 0.85f, 30, 5, 10, 0.12f, 6, Gl_Wolf, X);
    add("e_treant",QStringLiteral("Древесный страж"),     8, Ai_Tank,   190, 12, 17,Dmg_Physical, 22, 54, 22, 34, 1.9f,  44, 8, 15, 0.18f, 7, Gl_Tree, X);
    add("e_hive",  QStringLiteral("Ройный улей"),         8, Ai_Turret, 120, 4, 6,  Dmg_Poison,   8,  0, 20, 210, 2.2f,  40, 7, 13, 0.16f, 8, Gl_Spider, X);
    add("e_savage",QStringLiteral("Отравленный дикарь"),  9, Ai_Ranged,  84, 10, 15,Dmg_Poison,   6,  88, 14, 260, 1.7f,  42, 8, 14, 0.14f, 9, Gl_PoisonDot, X);
    add("e_thornp",QStringLiteral("Жрица шипов"),        10, Ai_Summoner,110, 9, 14,Dmg_Poison,   8,  76, 15, 320, 2.4f,  62, 12, 20, 0.22f,10, Gl_Summon, X);
    {
        EnemyDef h; h.summoner = true; h.summonId = 30; h.summonCount = 2; h.detectRange = 420; set(8, h);
        EnemyDef s; s.summoner = true; s.summonId = 30; s.summonCount = 2; s.rangedProjSpeed = 300; set(10, s);
        EnemyDef r; r.rangedProjSpeed = 340; set(9, r);
        EnemyDef b; b.boss = true; b.title = QStringLiteral("Матка Терний"); b.phases = 3; b.patternSet = 1; b.scale = 1.65f; b.detectRange = 900; b.summoner = true; b.summonId = 30; b.summonCount = 2;
        add("b_thorn", QStringLiteral("Матка Терний"), 13, Ai_Boss, 2600, 20, 28, Dmg_Poison, 26, 84, 34, 48, 1.5f, 1100, 130, 240, 1.0f, 11, Gl_Boss, b);
    }

    // --- РЕГИОН 2: Ледяные пики ---
    add("e_icehunt",QStringLiteral("Ледяной охотник"),  13, Ai_Ranged,  150, 18, 26, Dmg_Frost,   14, 92, 15, 300, 1.7f,  80, 14, 24, 0.14f, 12, Gl_IceShard, X);
    add("e_golem", QStringLiteral("Морозный голем"),     14, Ai_Tank,    420, 26, 36, Dmg_Frost,   46, 52, 26, 38, 2.0f,  110, 20, 34, 0.20f, 13, Gl_Crystal, X);
    add("e_cspider",QStringLiteral("Кристальный паук"), 13, Ai_Charger,  180, 20, 28, Dmg_Frost,   18, 118,16, 30, 1.3f,   84, 15, 26, 0.15f, 14, Gl_Spider, X);
    add("e_frostm",QStringLiteral("Инейный чародей"),   15, Ai_Caster,   190, 28, 40, Dmg_Frost,   12, 74, 15, 340, 2.6f,  120, 22, 38, 0.20f, 15, Gl_Chill, X);
    add("e_swolf", QStringLiteral("Снежный волк"),      14, Ai_Swarm,    120, 14, 19, Dmg_Frost,    8, 156,13, 24, 0.8f,   62, 11, 19, 0.10f, 16, Gl_Wolf, X);
    {
        EnemyDef p; p.rangedProjSpeed = 400; set(12, p);
        EnemyDef c; c.detectRange = 400; set(15, c);
        EnemyDef b; b.boss = true; b.title = QStringLiteral("Вьюговей, Ледяной Тиран"); b.phases = 3; b.patternSet = 2; b.scale = 1.75f; b.detectRange = 950;
        add("b_frost", QStringLiteral("Вьюговей"), 19, Ai_Boss, 6200, 34, 48, Dmg_Frost, 60, 92, 38, 50, 1.5f, 2600, 260, 460, 1.0f, 17, Gl_Boss, b);
    }

    // --- РЕГИОН 3: Гнилые топи ---
    add("e_rot",   QStringLiteral("Болотная гниль"),     19, Ai_Caster,   320, 34, 48, Dmg_Poison,  24, 68, 18, 320, 2.4f,  180, 32, 54, 0.18f, 18, Gl_Poison, X);
    add("e_devour",QStringLiteral("Трупоед"),            18, Ai_Swarm,    240, 24, 32, Dmg_Physical,16, 150, 15, 26, 0.8f,  150, 28, 48, 0.12f, 19, Gl_Skull, X);
    add("e_morpri",QStringLiteral("Жрец морока"),        21, Ai_Summoner, 400, 38, 52, Dmg_Arcane,  30, 74, 17, 340, 2.4f,  240, 42, 70, 0.24f, 20, Gl_Curse, X);
    add("e_drown", QStringLiteral("Затонувший страж"),   20, Ai_Tank,     780, 44, 60, Dmg_Physical,72, 50, 28, 40, 2.0f,   220, 40, 66, 0.22f, 21, Gl_Shield, X);
    add("e_plague",QStringLiteral("Чумной стрелок"),     19, Ai_Ranged,   300, 36, 50, Dmg_Poison,  20, 92, 15, 320, 1.7f,  175, 32, 55, 0.16f, 22, Gl_Bone, X);
    {
        EnemyDef p; p.rangedProjSpeed = 420; set(22, p);
        EnemyDef s; s.summoner = true; s.summonId = 31; s.summonCount = 2; s.rangedProjSpeed = 380; set(20, s);
        EnemyDef c; c.detectRange = 420; set(18, c);
        EnemyDef b; b.boss = true; b.title = QStringLiteral("Гнилостный Колосс"); b.phases = 3; b.patternSet = 3; b.scale = 1.9f; b.detectRange = 950; b.summoner = true; b.summonId = 31; b.summonCount = 3;
        add("b_rot", QStringLiteral("Гнилостный Колосс"), 25, Ai_Boss, 15000, 60, 84, Dmg_Poison, 120, 78, 44, 54, 1.6f, 6200, 520, 900, 1.0f, 23, Gl_Boss, b);
    }

    // --- РЕГИОН 4: Горнило пустоты ---
    add("e_elem",  QStringLiteral("Огненный элементаль"),25, Ai_Chase,    620, 70, 96, Dmg_Fire,    50, 118,20, 34, 1.2f,  380, 70, 120, 0.20f, 24, Gl_Fireball, X);
    add("e_forge", QStringLiteral("Страж горнила"),      27, Ai_Tank,    1700, 90, 130,Dmg_Fire,   160,  56, 32, 46, 1.9f,  520, 96, 160, 0.26f, 25, Gl_Hammer, X);
    add("e_eye",   QStringLiteral("Пустотный глаз"),     28, Ai_Ranged,   760, 96, 140,Dmg_Arcane,  60,  84, 22, 420, 2.0f,  480, 92, 150, 0.22f, 26, Gl_Eye, X);
    add("e_warp",  QStringLiteral("Искажённый"),         26, Ai_Charger,  900, 84, 120,Dmg_Arcane,  70, 130, 19, 34, 1.2f,  420, 80, 130, 0.20f, 27, Gl_Blink, X);
    add("e_horror",QStringLiteral("Звёздный ужас"),      29, Ai_Summoner,1200, 100,150,Dmg_Arcane,  90,  78, 26, 400, 2.2f,  640, 120, 200,0.28f, 28, Gl_Curse, X);
    {
        EnemyDef e; e.explodes = true; e.explodeRadius = 96; set(24, e);
        EnemyDef p; p.rangedProjSpeed = 520; set(26, p);
        EnemyDef s; s.summoner = true; s.summonId = 32; s.summonCount = 2; s.rangedProjSpeed = 460; set(28, s);
        EnemyDef b; b.boss = true; b.title = QStringLiteral("Пожиратель Звёзд"); b.phases = 4; b.patternSet = 4; b.scale = 2.3f; b.detectRange = 1200;
        b.summoner = true; b.summonId = 32; b.summonCount = 2;
        add("b_star", QStringLiteral("Пожиратель Звёзд"), 33, Ai_Boss, 42000, 140, 200, Dmg_Arcane, 240, 96, 52, 60, 1.5f, 20000, 2000, 4000, 1.0f, 29, Gl_Boss, b);
    }

    // --- ПРИСЛУЖНИКИ (не встречаются на карте напрямую) ---
    add("m_thorn", QStringLiteral("Отпрыск терний"),      8, Ai_Swarm,   50, 6, 9,   Dmg_Poison,   2, 120, 11, 22, 0.9f,   6, 0, 1, 0.02f, 30, Gl_Tree, X);
    add("m_shade", QStringLiteral("Тень-прислужник"),    18, Ai_Swarm,  140, 16, 22,Dmg_Arcane,   8, 140, 13, 24, 0.8f,  14, 0, 2, 0.02f, 31, Gl_Summon, X);
    add("m_ash",   QStringLiteral("Пепельный отпрыск"),  26, Ai_Chase,  320, 34, 46,Dmg_Fire,    20, 130, 15, 26, 1.0f,  30, 0, 4, 0.02f, 32, Gl_Fireball, X);
}

// ----------------------------------------------------------------
//  ЭЛИТНЫЕ АФФИКСЫ
// ----------------------------------------------------------------
static void loadElites()
{
    g_elites.clear();
    auto add = [&](const QString &n, const QString &d, const QColor &c) {
        EliteAffix e; e.id = g_elites.size(); e.name = n; e.desc = d; e.color = c; g_elites.append(e);
    };
    add(QStringLiteral("Пылающий"),   QStringLiteral("Оставляет огненный след и поджигает."), C::ember);
    add(QStringLiteral("Ледяной"),    QStringLiteral("Замораживает землю и замедляет тебя."), C::frost);
    add(QStringLiteral("Грозовой"),   QStringLiteral("Бьёт молниями по площади."), C::lightning);
    add(QStringLiteral("Вампирский"), QStringLiteral("Восстанавливает здоровье от ударов."), C::bloodHi);
    add(QStringLiteral("Бронированный"),QStringLiteral("Получает вдвое меньше урона."), C::boneDim);
    add(QStringLiteral("Бешеный"),    QStringLiteral("Быстрее и бьёт чаще, но хрупче."), C::bad);
    add(QStringLiteral("Рой"),        QStringLiteral("При смерти выпускает отпрысков."), C::poison);
    add(QStringLiteral("Отражающий"), QStringLiteral("Возвращает часть урона атакующему."), C::arcane);
}

// ----------------------------------------------------------------
//  ЛОКАЦИИ МИРА
// ----------------------------------------------------------------
static void loadLocations()
{
    g_locs.clear();
    int id = 0;
    auto add = [&](const QString &sid, const QString &name, const QString &desc, LocType t, Biome b, int region,
                   int tier, const QPointF &pos, const QList<int> &links, const QVector<int> &pool,
                   int count, int elites, int boss, const QString &hint)
    {
        LocationDef L;
        L.id = id++;
        L.id_str = sid;
        L.name = name;
        L.desc = desc;
        L.type = t;
        L.biome = b;
        L.region = region;
        L.tier = tier;
        L.reqLevel = tierReq(tier);
        L.mapPos = pos;
        L.links = links;
        L.enemyPool = pool;
        L.enemyCount = count;
        L.eliteCount = elites;
        L.bossId = boss;
        L.seed = 1000 + L.id * 7919;
        L.rewardHint = hint;
        g_locs.append(L);
    };

    const QVector<int> P0 = QVector<int>() << 0 << 1 << 2 << 3;
    const QVector<int> P0b = QVector<int>() << 0 << 1 << 2 << 3 << 4;
    const QVector<int> P1 = QVector<int>() << 6 << 7 << 8 << 9;
    const QVector<int> P1b = QVector<int>() << 6 << 7 << 8 << 9 << 10;
    const QVector<int> P2 = QVector<int>() << 12 << 13 << 14 << 15 << 16;
    const QVector<int> P2b = QVector<int>() << 12 << 13 << 14 << 15 << 16;
    const QVector<int> P3 = QVector<int>() << 18 << 19 << 20 << 21 << 22;
    const QVector<int> P4 = QVector<int>() << 24 << 25 << 26 << 27 << 28;

    // ---- Регион 0: Пепельные равнины ----
    add("l00", QStringLiteral("Лагерь «Искра»"), QStringLiteral("Последний тёплый костёр на равнине. Здесь торгуют, лечат и позволяют переосмыслить путь."),
        Loc_Town, Bio_Ash, 0, 1, QPointF(0.070, 0.860), QList<int>() << 1, QVector<int>(), 0, 0, -1,
        QStringLiteral("Магазин, лечение, переброс навыков"));
    add("l01", QStringLiteral("Разбитая дорога"), QStringLiteral("Обломки повозок и кости тех, кто не дошёл."),
        Loc_Camp, Bio_Ash, 0, 1, QPointF(0.130, 0.780), QList<int>() << 0 << 2 << 3, P0, 7, 0, -1,
        QStringLiteral("Первое испытание"));
    add("l02", QStringLiteral("Пепельный курган"), QStringLiteral("Насыпь, в которой что-то шевелится."),
        Loc_Ruins, Bio_Ash, 0, 1, QPointF(0.090, 0.660), QList<int>() << 1 << 4, P0, 8, 0, -1,
        QStringLiteral("Плотный пак врагов"));
    add("l03", QStringLiteral("Руины дозора"), QStringLiteral("Башня, с которой давно никто не смотрит."),
        Loc_Ruins, Bio_Ash, 0, 1, QPointF(0.200, 0.860), QList<int>() << 1 << 4, P0b, 9, 1, -1,
        QStringLiteral("Возможен элитный враг"));
    add("l04", QStringLiteral("Логово стаи"), QStringLiteral("Здесь воют по ночам. И по дням тоже."),
        Loc_Elite, Bio_Ash, 0, 2, QPointF(0.170, 0.700), QList<int>() << 2 << 3 << 5, P0b, 10, 2, -1,
        QStringLiteral("Два элитных врага, хороший лут"));
    add("l05", QStringLiteral("Святилище углей"), QStringLiteral("Тлеющий алтарь. Он ещё помнит молитвы."),
        Loc_Shrine, Bio_Ash, 0, 2, QPointF(0.250, 0.760), QList<int>() << 4 << 6, P0, 6, 0, -1,
        QStringLiteral("Благословение после боя"));
    add("l06", QStringLiteral("Кузня пепла"), QStringLiteral("Городок кузнецов, которые куют из того, что осталось."),
        Loc_Town, Bio_Ash, 0, 2, QPointF(0.280, 0.880), QList<int>() << 5 << 7, QVector<int>(), 0, 0, -1,
        QStringLiteral("Магазин получше, лечение"));
    add("l07", QStringLiteral("Чертоги Патриарха"), QStringLiteral("Он сжёг этот край и остался править пеплом."),
        Loc_Boss, Bio_Ash, 0, 2, QPointF(0.330, 0.740), QList<int>() << 6 << 8, QVector<int>() << 0 << 1 << 2, 4, 0, 5,
        QStringLiteral("БОСС: Пепельный Патриарх"));

    // ---- Регион 1: Угольный лес ----
    add("l08", QStringLiteral("Привал корней"), QStringLiteral("Деревня на живых корнях. Корни недовольны."),
        Loc_Town, Bio_Ember, 1, 2, QPointF(0.300, 0.580), QList<int>() << 7 << 9 << 10, QVector<int>(), 0, 0, -1,
        QStringLiteral("Магазин, лечение, переброс"));
    add("l09", QStringLiteral("Тропа шипов"), QStringLiteral("Шипы растут в сторону идущего."),
        Loc_Forest, Bio_Ember, 1, 2, QPointF(0.240, 0.480), QList<int>() << 8 << 11, P1, 10, 0, -1,
        QStringLiteral("Лесной бой"));
    add("l10", QStringLiteral("Гнилая роща"), QStringLiteral("Деревья здесь дышат. Лучше не прислушиваться."),
        Loc_Forest, Bio_Ember, 1, 2, QPointF(0.360, 0.480), QList<int>() << 8 << 12, P1b, 11, 1, -1,
        QStringLiteral("Элитный страж"));
    add("l11", QStringLiteral("Пещера эха"), QStringLiteral("Эхо отвечает не твоим голосом."),
        Loc_Cave, Bio_Crystal, 1, 3, QPointF(0.200, 0.360), QList<int>() << 9 << 13, P1b, 11, 1, -1,
        QStringLiteral("Тёмный грот, кристаллы"));
    add("l12", QStringLiteral("Святилище светлячков"), QStringLiteral("Тысячи огней хранят одно воспоминание."),
        Loc_Shrine, Bio_Ember, 1, 3, QPointF(0.400, 0.360), QList<int>() << 10 << 13, P1, 7, 0, -1,
        QStringLiteral("Благословение"));
    add("l13", QStringLiteral("Чащоба вдовы"), QStringLiteral("Паутина натянутая между стволов. Она ждёт."),
        Loc_Elite, Bio_Ember, 1, 3, QPointF(0.300, 0.280), QList<int>() << 11 << 12 << 14, P1b, 12, 3, -1,
        QStringLiteral("Три элитных врага"));
    add("l14", QStringLiteral("Гнездо Матки"), QStringLiteral("Корни, коконы и бесконечный шёпот."),
        Loc_Boss, Bio_Ember, 1, 3, QPointF(0.380, 0.200), QList<int>() << 13 << 15, QVector<int>() << 6 << 8, 4, 0, 11,
        QStringLiteral("БОСС: Матка Терний"));

    // ---- Регион 2: Ледяные пики ----
    add("l15", QStringLiteral("Застава мороза"), QStringLiteral("Крепость, вмороженная в склон."),
        Loc_Town, Bio_Frost, 2, 3, QPointF(0.460, 0.280), QList<int>() << 14 << 16 << 17, QVector<int>(), 0, 0, -1,
        QStringLiteral("Магазин льда и стали"));
    add("l16", QStringLiteral("Тропа метели"), QStringLiteral("Ветер сдирает кожу и память."),
        Loc_Camp, Bio_Frost, 2, 3, QPointF(0.440, 0.160), QList<int>() << 15 << 18, P2, 12, 1, -1,
        QStringLiteral("Открытая местность"));
    add("l17", QStringLiteral("Замёрзший разлом"), QStringLiteral("Во льду видны те, кто не успел выбраться."),
        Loc_Cave, Bio_Frost, 2, 4, QPointF(0.530, 0.380), QList<int>() << 15 << 19, P2b, 13, 1, -1,
        QStringLiteral("Узкие проходы"));
    add("l18", QStringLiteral("Кристальные гроты"), QStringLiteral("Свет преломляется так, что видно завтра."),
        Loc_Cave, Bio_Crystal, 2, 4, QPointF(0.520, 0.120), QList<int>() << 16 << 20, P2b, 13, 2, -1,
        QStringLiteral("Много кристаллов, много добычи"));
    add("l19", QStringLiteral("Лагерь охотников"), QStringLiteral("Заброшен. Костёр ещё тёплый."),
        Loc_Camp, Bio_Frost, 2, 4, QPointF(0.600, 0.300), QList<int>() << 17 << 20, P2, 12, 2, -1,
        QStringLiteral("Быстрые враги"));
    add("l20", QStringLiteral("Цитадель инея"), QStringLiteral("Ворота не заперты. Это плохой знак."),
        Loc_Fortress, Bio_Frost, 2, 4, QPointF(0.580, 0.180), QList<int>() << 18 << 19 << 21, P2b, 15, 2, -1,
        QStringLiteral("Тяжёлый бой"));
    add("l21", QStringLiteral("Трон Вьюговея"), QStringLiteral("Тиран сидит неподвижно. Метель - его дыхание."),
        Loc_Boss, Bio_Frost, 2, 5, QPointF(0.660, 0.100), QList<int>() << 20 << 22, QVector<int>() << 16 << 12, 4, 0, 17,
        QStringLiteral("БОСС: Вьюговей, Ледяной Тиран"));

    // ---- Регион 3: Гнилые топи ----
    add("l22", QStringLiteral("Хижина на сваях"), QStringLiteral("Поселение над водой, которая не отражает."),
        Loc_Town, Bio_Swamp, 3, 4, QPointF(0.640, 0.560), QList<int>() << 21 << 23 << 24, QVector<int>(), 0, 0, -1,
        QStringLiteral("Магазин ядов и оберегов"));
    add("l23", QStringLiteral("Затопленный храм"), QStringLiteral("Алтарь под водой. Кто-то всё ещё молится."),
        Loc_Ruins, Bio_Ruins, 3, 4, QPointF(0.580, 0.680), QList<int>() << 22 << 25, P3, 14, 1, -1,
        QStringLiteral("Руины, вода"));
    add("l24", QStringLiteral("Болото костей"), QStringLiteral("Ступай осторожно - кости не любят, когда их топчут."),
        Loc_Swamp, Bio_Swamp, 3, 4, QPointF(0.720, 0.660), QList<int>() << 22 << 25, P3, 15, 2, -1,
        QStringLiteral("Много мелких врагов"));
    add("l25", QStringLiteral("Гнездо морока"), QStringLiteral("Тени здесь ходят сами."),
        Loc_Swamp, Bio_Swamp, 3, 5, QPointF(0.660, 0.780), QList<int>() << 23 << 24 << 26, P3, 15, 2, -1,
        QStringLiteral("Призыватели"));
    add("l26", QStringLiteral("Чумной курган"), QStringLiteral("Земля сочится. Лучше не копать."),
        Loc_Elite, Bio_Swamp, 3, 5, QPointF(0.760, 0.800), QList<int>() << 25 << 27, P3, 14, 3, -1,
        QStringLiteral("Три элитных врага"));
    add("l27", QStringLiteral("Чрево Колосса"), QStringLiteral("Он не умер. Он просто лёг."),
        Loc_Boss, Bio_Swamp, 3, 5, QPointF(0.800, 0.660), QList<int>() << 26 << 28, QVector<int>() << 19 << 22, 5, 0, 23,
        QStringLiteral("БОСС: Гнилостный Колосс"));

    // ---- Регион 4: Горнило пустоты ----
    add("l28", QStringLiteral("Последний приют"), QStringLiteral("Пятеро выживших, один кузнец, ноль надежды."),
        Loc_Town, Bio_Void, 4, 5, QPointF(0.840, 0.500), QList<int>() << 27 << 29 << 30, QVector<int>(), 0, 0, -1,
        QStringLiteral("Лучший магазин в мире"));
    add("l29", QStringLiteral("Залы расплава"), QStringLiteral("Реки металла и кости в них."),
        Loc_Fortress, Bio_Lava, 4, 5, QPointF(0.880, 0.340), QList<int>() << 28 << 31, P4, 16, 2, -1,
        QStringLiteral("Огненные враги"));
    add("l30", QStringLiteral("Разлом пустоты"), QStringLiteral("Трещина в мире. Из неё смотрят."),
        Loc_Cave, Bio_Void, 4, 6, QPointF(0.920, 0.600), QList<int>() << 28 << 31, P4, 16, 3, -1,
        QStringLiteral("Пустотные твари"));
    add("l31", QStringLiteral("Обсерватория бездны"), QStringLiteral("Звёзды здесь записывают, кого съели."),
        Loc_Fortress, Bio_Void, 4, 6, QPointF(0.900, 0.200), QList<int>() << 29 << 30 << 32, P4, 18, 3, -1,
        QStringLiteral("Финальный пак перед троном"));
    add("l32", QStringLiteral("Трон Пожирателя Звёзд"), QStringLiteral("Конец карты. Конец света. Твой ход."),
        Loc_Final, Bio_Void, 4, 6, QPointF(0.960, 0.100), QList<int>() << 31, QVector<int>() << 27 << 26, 4, 0, 29,
        QStringLiteral("ФИНАЛЬНЫЙ БОСС"));
}

// ----------------------------------------------------------------
//  ЗАГРУЗКА
// ----------------------------------------------------------------
void ensureLoaded()
{
    if (g_loaded) return;
    g_loaded = true;
    loadBases();
    loadAffixes();
    loadUniques();
    loadSkills();
    loadNodes();
    loadElites();
    loadEnemies();
    loadLocations();
}

const QVector<ItemBaseDef> &bases()     { ensureLoaded(); return g_bases; }
const QVector<AffixDef>    &affixes()   { ensureLoaded(); return g_affixes; }
const QVector<UniqueDef>   &uniques()   { ensureLoaded(); return g_uniques; }
const QVector<ActiveSkill> &skills()    { ensureLoaded(); return g_skills; }
const QVector<SkillNode>   &nodes()     { ensureLoaded(); return g_nodes; }
const QVector<EnemyDef>    &enemies()   { ensureLoaded(); return g_enemies; }
const QVector<LocationDef> &locations() { ensureLoaded(); return g_locs; }
const QVector<EliteAffix>  &eliteAffixes() { ensureLoaded(); return g_elites; }

const ItemBaseDef *base(int id)   { ensureLoaded(); return (id >= 0 && id < g_bases.size())   ? &g_bases[id]   : 0; }
const AffixDef    *affix(int id)  { ensureLoaded(); return (id >= 0 && id < g_affixes.size())  ? &g_affixes[id]  : 0; }
const UniqueDef   *unq(int id)    { ensureLoaded(); return (id >= 0 && id < g_uniques.size())  ? &g_uniques[id]  : 0; }
const ActiveSkill *skill(int id)  { ensureLoaded(); return (id >= 0 && id < g_skills.size())   ? &g_skills[id]   : 0; }
const SkillNode   *node(int id)   { ensureLoaded(); return (id >= 0 && id < g_nodes.size())    ? &g_nodes[id]    : 0; }
const EnemyDef    *enemy(int id)  { ensureLoaded(); return (id >= 0 && id < g_enemies.size())  ? &g_enemies[id]  : 0; }
const LocationDef *loc(int id)    { ensureLoaded(); return (id >= 0 && id < g_locs.size())     ? &g_locs[id]     : 0; }

float ringRadius(int ring)
{
    // кольца раздвинуты, чтобы узлы не слипались (паутина стала просторнее);
    // 4-й и 5-й круг разведены сильнее - раньше узлы на общих спицах накладывались
    switch (ring) {
    case 0: return 0.00f;
    case 1: return 0.24f;
    case 2: return 0.45f;
    case 3: return 0.645f;
    case 4: return 0.82f;
    case 5: return 1.02f;
    default: return 0.5f;
    }
}

float angDiff(float a, float b)
{
    float d = a - b;
    while (d > float(M_PI)) d -= 2.0f * float(M_PI);
    while (d < -float(M_PI)) d += 2.0f * float(M_PI);
    return qAbs(d);
}

int nodeReqLevel(int ring)
{
    switch (ring) {
    case 0: return 1;
    case 1: return 1;
    case 2: return 3;
    case 3: return 8;
    case 4: return 14;
    case 5: return 18;
    default: return 1;
    }
}

QVector<int> nodesOfRing(int ring)
{
    QVector<int> out;
    for (int i = 0; i < g_nodes.size(); ++i) if (g_nodes[i].ring == ring) out.append(i);
    return out;
}

QPointF nodePos(const SkillNode &n)
{
    return QPointF(qCos(n.ang) * n.radius, -qSin(n.ang) * n.radius);
}

QVector<int> basesForSlot(Slot s, int ilvl)
{
    ensureLoaded();
    QVector<int> out;
    for (int i = 0; i < g_bases.size(); ++i) {
        const ItemBaseDef &b = g_bases[i];
        if (b.bag != Bag_Equip || b.slot != s) continue;
        // кольца совместимы между Ring1/Ring2
        if (b.reqLevel <= ilvl + 3 && b.reqLevel >= ilvl - 14) out.append(i);
    }
    if (out.isEmpty()) {
        for (int i = 0; i < g_bases.size(); ++i)
            if (g_bases[i].bag == Bag_Equip && g_bases[i].slot == s) out.append(i);
    }
    return out;
}

QVector<int> affixesFor(Slot s, int ilvl, bool prefix)
{
    ensureLoaded();
    QVector<int> out;
    int bit = 1 << int(s);
    for (int i = 0; i < g_affixes.size(); ++i) {
        const AffixDef &a = g_affixes[i];
        if (a.prefix != prefix) continue;
        if (a.minIlvl > ilvl) continue;
        if (!(a.slotMask & bit)) continue;
        out.append(i);
    }
    return out;
}

QVector<int> basesOfBag(BagKind k)
{
    ensureLoaded();
    QVector<int> out;
    for (int i = 0; i < g_bases.size(); ++i) if (g_bases[i].bag == k) out.append(i);
    return out;
}

// ----------------------------------------------------------------
//  ИМЕНА И ОПИСАНИЯ
// ----------------------------------------------------------------
static const char *g_rarePrefixes[] = {
    "Проклятый", "Забытый", "Пепельный", "Грозный", "Древний", "Кровавый", "Звёздный",
    "Погибельный", "Шёпотный", "Полуночный", "Ржавый", "Вечный", "Горнильный", "Тенекрылый"
};
static const char *g_rareSuffixes[] = {
    "Пустоши", "Пепла", "Затмения", "Вдовы", "Тирана", "Бездны", "Последнего Дня",
    "Углей", "Морока", "Костей", "Первого Света", "Горнила", "Тихого Часа"
};

QString rareName(const Item &it)
{
    Rng r(quint64(it.seed) * 2246822519u + 13u);
    int np = int(sizeof(g_rarePrefixes) / sizeof(g_rarePrefixes[0]));
    int ns = int(sizeof(g_rareSuffixes) / sizeof(g_rareSuffixes[0]));
    const ItemBaseDef *b = base(it.baseId);
    if (!b) return QStringLiteral("Неведомое");
    return QString::fromUtf8(g_rarePrefixes[r.pick(np)]) + QLatin1Char(' ') + b->name
           + QLatin1Char(' ') + QString::fromUtf8(g_rareSuffixes[r.pick(ns)]);
}

QString itemNameFull(const Item &it)
{
    if (!it.valid) return QString();
    const ItemBaseDef *b = base(it.baseId);
    if (!b) return QString();
    if (it.rarity == Rar_Unique) {
        const UniqueDef *u = unq(it.uniqueId);
        return u ? u->name : b->name;
    }
    if (it.rarity == Rar_Normal) return b->name;
    if (it.rarity == Rar_Magic) {
        QString pre, suf;
        for (int i = 0; i < it.affixes.size(); ++i) {
            const AffixDef *a = affix(it.affixes[i]);
            if (!a) continue;
            if (a->prefix && pre.isEmpty()) pre = a->name;
            else if (!a->prefix && suf.isEmpty()) suf = a->name;
        }
        QString out;
        if (!pre.isEmpty()) out += pre + QLatin1Char(' ');
        out += b->name;
        if (!suf.isEmpty()) out += QLatin1Char(' ') + suf;
        return out;
    }
    if (it.rarity == Rar_Legend) {
        const AffixDef *a = it.affixes.isEmpty() ? 0 : affix(it.affixes.first());
        return (a ? a->name + QLatin1Char(' ') : QString()) + b->name + QStringLiteral(" ‹легендарный›");
    }
    return rareName(it);
}

QString describeMods(const StatMods &m, float mult)
{
    QStringList out;
    struct E { const char *label; float v; bool pct; int prec; };
    QVector<E> es;
    auto push = [&](const char *l, float v, bool pct, int prec) { if (qAbs(v) > 1e-4f) { E e; e.label = l; e.v = v; e.pct = pct; e.prec = prec; es.append(e); } };

    push("здоровье", m.hpFlat * mult, false, 0);
    push("здоровье", m.hpPct * mult, true, 0);
    push("мана", m.manaFlat * mult, false, 0);
    push("мана", m.manaPct * mult, true, 0);
    push("живучесть", m.vit * mult, false, 0);
    push("мощь", m.might * mult, false, 0);
    push("дух", m.spirit * mult, false, 0);
    push("ловкость", m.agi * mult, false, 0);
    push("концентрация", m.focus * mult, false, 0);
    push("броня", m.armor * mult, false, 0);
    push("броня", m.armorPct * mult, true, 0);
    push("шанс крита", m.critChance * mult, true, 1);
    push("крит. урон", m.critMult * mult, true, 0);
    push("уклонение", m.dodge * mult, true, 1);
    push("скорость", m.moveSpeed * mult, true, 0);
    push("скорость атаки", m.attackSpeed * mult, true, 0);
    push("скорость заклинаний", m.castSpeed * mult, true, 0);
    push("сокращение отката", m.cdReduction * mult, true, 0);
    push("восст. здоровья", m.hpRegen * mult, false, 1);
    push("восст. маны", m.manaRegen * mult, false, 1);
    push("вампиризм", m.lifeSteal * mult, true, 1);
    push("похищение маны", m.manaSteal * mult, true, 1);
    push("урон", m.dmgPct * mult, true, 0);
    push("физ. урон", m.physPct * mult, true, 0);
    push("огненный урон", m.firePct * mult, true, 0);
    push("ледяной урон", m.frostPct * mult, true, 0);
    push("урон молнией", m.lightPct * mult, true, 0);
    push("ядовитый урон", m.poisonPct * mult, true, 0);
    push("тайный урон", m.arcanePct * mult, true, 0);
    push("область", m.areaPct * mult, true, 0);
    push("скорость снарядов", m.projSpeed * mult, true, 0);
    push("шипы", m.thorns * mult, false, 0);
    push("поиск золота", m.goldFind * mult, true, 0);
    push("поиск редкостей", m.magicFind * mult, true, 0);
    push("получаемый опыт", m.xpGain * mult, true, 0);
    push("сила зелий", m.potionPotency * mult, true, 0);
    push("сила прислужников", m.summonPct * mult, true, 0);
    push("длительность эффектов", m.durationPct * mult, true, 0);
    push("доп. снаряд", m.extraProj * mult, false, 0);
    push("доп. цель цепи", m.chainBonus * mult, false, 0);
    push("сопр. огню", m.resFire * mult, true, 0);
    push("сопр. холоду", m.resFrost * mult, true, 0);
    push("сопр. молнии", m.resLight * mult, true, 0);
    push("сопр. яду", m.resPoison * mult, true, 0);
    push("сопр. тайной магии", m.resArcane * mult, true, 0);
    push("сопротивления", m.resAll * mult, true, 0);
    push("барьер", m.shieldFlat * mult, false, 0);

    for (int i = 0; i < es.size(); ++i) {
        const E &e = es[i];
        QString v;
        if (e.pct) v = QString::number(double(e.v * 100.0f), 'f', e.prec) + QLatin1Char('%');
        else       v = QString::number(double(qRound(e.v * 10.0f) / 10.0f), 'f', e.prec);
        if (e.v > 0) v.prepend(QLatin1Char('+'));
        out << v + QLatin1Char(' ') + QString::fromUtf8(e.label);
    }
    return out.join(QStringLiteral("\n"));
}

QString skillDesc(const ActiveSkill &s, int rank)
{
    float k = 1.0f + float(rank - 1) * 0.22f;
    QStringList l;
    l << s.desc;
    l << QString();
    if (s.dmgPct > 0.f)
        l << QStringLiteral("Урон: %1% от оружия").arg(int(s.dmgPct * k * 100.f));
    if (s.radius > 0.f)
        l << QStringLiteral("Радиус: %1").arg(int(s.radius * (1.0f + 0.06f * (rank - 1))));
    if (s.duration > 0.f)
        l << QStringLiteral("Длительность: %1 сек").arg(QString::number(double(s.duration * (1.0f + 0.08f * (rank - 1))), 'f', 1));
    l << QStringLiteral("Стоимость: %1 маны").arg(int(s.cost * (1.0f + 0.10f * (rank - 1))));
    l << QStringLiteral("Откат: %1 сек").arg(QString::number(double(s.cd * (1.0f - 0.06f * (rank - 1))), 'f', 2));
    return l.join(QStringLiteral("\n"));
}

// ----------------------------------------------------------------
//  МИР
// ----------------------------------------------------------------
int locationCount() { ensureLoaded(); return g_locs.size(); }
int regionCount()   { return 5; }

QString regionName(int region)
{
    switch (region) {
    case 0: return QStringLiteral("Пепельные равнины");
    case 1: return QStringLiteral("Угольный лес");
    case 2: return QStringLiteral("Ледяные пики");
    case 3: return QStringLiteral("Гнилые топи");
    case 4: return QStringLiteral("Горнило пустоты");
    default: return QStringLiteral("Неизвестные земли");
    }
}

QColor regionColor(int region)
{
    switch (region) {
    case 0: return QColor(196, 140,  80);
    case 1: return QColor(120, 186, 110);
    case 2: return QColor(140, 200, 236);
    case 3: return QColor(140, 190, 110);
    case 4: return QColor(200, 130, 220);
    default: return C::ash;
    }
}

QVector<int> locationsOfRegion(int region)
{
    ensureLoaded();
    QVector<int> out;
    for (int i = 0; i < g_locs.size(); ++i) if (g_locs[i].region == region) out.append(i);
    return out;
}

int firstLocation() { return 0; }

QVector<int> enemyPoolForLocation(int locId)
{
    const LocationDef *L = loc(locId);
    if (!L) return QVector<int>();
    return L->enemyPool;
}

QString enemyNameFor(int defId, int levelDelta)
{
    const EnemyDef *e = enemy(defId);
    if (!e) return QString();
    if (levelDelta == 0) return e->name;
    return e->name + QStringLiteral(" ") + romanNum(qBound(1, levelDelta, 20));
}

} // namespace Data
} // namespace aa

// ============================================================
//  ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ИЗ Types.h
// ============================================================
namespace aa {

QString slotName(Slot s)
{
    switch (s) {
    case Slot_Weapon:  return QStringLiteral("Оружие");
    case Slot_Offhand: return QStringLiteral("Вторая рука");
    case Slot_Helm:    return QStringLiteral("Шлем");
    case Slot_Chest:   return QStringLiteral("Нагрудник");
    case Slot_Gloves:  return QStringLiteral("Перчатки");
    case Slot_Belt:    return QStringLiteral("Пояс");
    case Slot_Boots:   return QStringLiteral("Сапоги");
    case Slot_Amulet:  return QStringLiteral("Амулет");
    case Slot_Ring1:   return QStringLiteral("Кольцо");
    case Slot_Ring2:   return QStringLiteral("Кольцо");
    default:           return QStringLiteral("-");
    }
}

QString rarityName(Rarity r)
{
    switch (r) {
    case Rar_Normal:   return QStringLiteral("Обычный");
    case Rar_Magic:    return QStringLiteral("Магический");
    case Rar_Rare:     return QStringLiteral("Редкий");
    case Rar_Legend:   return QStringLiteral("Легендарный");
    case Rar_Unique:   return QStringLiteral("Уникальный");
    default:           return QString();
    }
}

QColor rarityColor(Rarity r)
{
    switch (r) {
    case Rar_Normal:   return C::boneDim;
    case Rar_Magic:    return QColor(110, 150, 240);
    case Rar_Rare:     return QColor(240, 205, 90);
    case Rar_Legend:   return QColor(216, 130, 240);
    case Rar_Unique:   return QColor(240, 120, 80);
    default:           return C::bone;
    }
}

QString dmgTypeName(DmgType t)
{
    switch (t) {
    case Dmg_Physical:  return QStringLiteral("физический");
    case Dmg_Fire:      return QStringLiteral("огонь");
    case Dmg_Frost:     return QStringLiteral("холод");
    case Dmg_Lightning: return QStringLiteral("молния");
    case Dmg_Poison:    return QStringLiteral("яд");
    case Dmg_Arcane:    return QStringLiteral("тайная магия");
    case Dmg_Pure:      return QStringLiteral("чистый");
    default:            return QString();
    }
}

QColor dmgTypeColor(DmgType t)
{
    switch (t) {
    case Dmg_Physical:  return C::bone;
    case Dmg_Fire:      return C::ember;
    case Dmg_Frost:     return C::frost;
    case Dmg_Lightning: return C::lightning;
    case Dmg_Poison:    return C::poison;
    case Dmg_Arcane:    return C::arcane;
    case Dmg_Pure:      return C::boneDim;
    default:            return C::bone;
    }
}

QString biomeName(Biome b)
{
    switch (b) {
    case Bio_Ash:     return QStringLiteral("Пепельные равнины");
    case Bio_Ember:   return QStringLiteral("Угольный лес");
    case Bio_Frost:   return QStringLiteral("Ледяные пики");
    case Bio_Crystal: return QStringLiteral("Кристальные гроты");
    case Bio_Swamp:   return QStringLiteral("Гнилые топи");
    case Bio_Ruins:   return QStringLiteral("Затонувшие руины");
    case Bio_Lava:    return QStringLiteral("Горнило");
    case Bio_Void:    return QStringLiteral("Пустота");
    case Bio_Town:    return QStringLiteral("Поселение");
    default:          return QString();
    }
}

QString locTypeName(LocType t)
{
    switch (t) {
    case Loc_Town:     return QStringLiteral("Поселение");
    case Loc_Camp:     return QStringLiteral("Лагерь");
    case Loc_Ruins:    return QStringLiteral("Руины");
    case Loc_Cave:     return QStringLiteral("Пещера");
    case Loc_Forest:   return QStringLiteral("Лес");
    case Loc_Swamp:    return QStringLiteral("Топи");
    case Loc_Fortress: return QStringLiteral("Крепость");
    case Loc_Shrine:   return QStringLiteral("Святилище");
    case Loc_Elite:    return QStringLiteral("Элитное логово");
    case Loc_Boss:     return QStringLiteral("Логово босса");
    case Loc_Final:    return QStringLiteral("Финал");
    default:           return QString();
    }
}

QColor biomeColor(Biome b)
{
    return biomePalette(b).accent;
}

QString aiName(AiKind a)
{
    switch (a) {
    case Ai_Chase:    return QStringLiteral("ближний бой");
    case Ai_Ranged:   return QStringLiteral("стрелок");
    case Ai_Charger:  return QStringLiteral("таран");
    case Ai_Caster:   return QStringLiteral("заклинатель");
    case Ai_Swarm:    return QStringLiteral("рой");
    case Ai_Summoner: return QStringLiteral("призыватель");
    case Ai_Tank:     return QStringLiteral("тяжёлый");
    case Ai_Turret:   return QStringLiteral("неподвижный");
    case Ai_Boss:     return QStringLiteral("босс");
    default:          return QString();
    }
}

int slotBit(Slot s) { return 1 << int(s); }

} // namespace aa
