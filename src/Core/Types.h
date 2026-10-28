#ifndef AA_TYPES_H
#define AA_TYPES_H

// ============================================================
//  «Пепел и Янтарь» - базовые типы и перечисления
//  Совместимо с Qt 5.9.1 / C++11
// ============================================================

#include <QString>
#include <QStringList>
#include <QList>
#include <QVector>
#include <QPointF>
#include <QColor>
#include <QHash>
#include <QSet>

namespace aa {

// ---------- Слоты экипировки ----------
enum Slot {
    Slot_Weapon = 0,
    Slot_Offhand,
    Slot_Helm,
    Slot_Chest,
    Slot_Gloves,
    Slot_Belt,
    Slot_Boots,
    Slot_Amulet,
    Slot_Ring1,
    Slot_Ring2,
    Slot_EquipCount
};

enum BagKind { Bag_Equip, Bag_Potion, Bag_Gem, Bag_Misc };

// ---------- Редкость ----------
enum Rarity {
    Rar_Normal = 0,
    Rar_Magic,
    Rar_Rare,
    Rar_Legend,
    Rar_Unique,
    Rar_Count
};

// ---------- Типы урона ----------
enum DmgType {
    Dmg_Physical = 0,
    Dmg_Fire,
    Dmg_Frost,
    Dmg_Lightning,
    Dmg_Poison,
    Dmg_Arcane,
    Dmg_Pure,
    Dmg_Count
};

// ---------- Биомы ----------
enum Biome {
    Bio_Ash = 0,      // пепельные равнины
    Bio_Ember,        // угольный лес
    Bio_Frost,        // ледяные пики
    Bio_Crystal,      // кристальные гроты
    Bio_Swamp,        // гнилые топи
    Bio_Ruins,        // затонувшие руины
    Bio_Lava,         // горнило
    Bio_Void,         // пустота
    Bio_Town,         // поселение
    Bio_Count
};

// ---------- Тип локации ----------
enum LocType {
    Loc_Town = 0,     // магазин, лечение, переброс навыков
    Loc_Camp,         // лёгкий бой
    Loc_Ruins,
    Loc_Cave,
    Loc_Forest,
    Loc_Swamp,
    Loc_Fortress,
    Loc_Shrine,       // бесплатное благословение
    Loc_Elite,        // элитный пак
    Loc_Boss,         // босс региона
    Loc_Final         // финальный босс
};

// ---------- Поведение врага ----------
enum AiKind {
    Ai_Chase = 0,     // бежит и бьёт вблизи
    Ai_Ranged,        // держит дистанцию, стреляет
    Ai_Charger,       // разгоняется и таранит
    Ai_Caster,        // телеграфирует круг и бьёт по площади
    Ai_Swarm,         // мелкий, быстрый, наваливается толпой
    Ai_Summoner,      // призывает minions, сам отступает
    Ai_Tank,          // медленный, толстый, аура
    Ai_Turret,        // стоит, стреляет очередями
    Ai_Boss           // скриптовый босс с фазами
};

// ---------- Активные умения (тип поведения) ----------
enum SkillFx {
    Fx_Cleave = 0,    // дуга ближнего боя
    Fx_Spin,          // вихрь вокруг себя
    Fx_Dash,          // рывок с уроном
    Fx_Projectile,    // одиночный снаряд
    Fx_MultiShot,     // веер снарядов
    Fx_Pierce,        // пробивающий снаряд
    Fx_Meteor,        // отложенный удар по площади
    Fx_Nova,          // мгновенное кольцо вокруг
    Fx_Chain,         // цепная молния
    Fx_Zone,          // зона с тикующим уроном
    Fx_Summon,        // призыв союзника
    Fx_Shield,        // барьер
    Fx_Heal,          // лечение
    Fx_Buff,          // усиление себя
    Fx_Mark,          // метка на врага
    Fx_Quake          // ударные волны по земле
};

// ---------- Вид узла паутины навыков ----------
enum NodeKind {
    Nk_Origin = 0,
    Nk_Minor,         // малый пассив
    Nk_Notable,       // заметный пассив
    Nk_Keystone,      // ключевой камень (меняет механику)
    Nk_Active         // активное умение
};

// ============================================================
//  Модификаторы характеристик - единая «валюта» бонусов
// ============================================================
struct StatMods
{
    // первичные атрибуты
    float vit = 0.f;      // живучесть   -> здоровье
    float might = 0.f;    // мощь        -> физ. урон
    float spirit = 0.f;   // дух         -> магич. урон
    float agi = 0.f;      // ловкость    -> скорость/крит
    float focus = 0.f;    // концентрация-> мана, откат

    // вторичные
    float hpFlat = 0.f;
    float manaFlat = 0.f;
    float hpPct = 0.f;         // в долях (0.1 = +10%)
    float manaPct = 0.f;
    float armor = 0.f;
    float armorPct = 0.f;
    float critChance = 0.f;    // в долях
    float critMult = 0.f;      // прибавка к множителю (0.5 = +50%)
    float dodge = 0.f;         // в долях
    float moveSpeed = 0.f;     // в долях
    float attackSpeed = 0.f;   // в долях
    float castSpeed = 0.f;     // в долях
    float cdReduction = 0.f;   // в долях
    float hpRegen = 0.f;       // ед/сек
    float manaRegen = 0.f;
    float lifeSteal = 0.f;     // в долях
    float manaSteal = 0.f;
    float dmgPct = 0.f;        // общий % урона
    float physPct = 0.f;
    float firePct = 0.f;
    float frostPct = 0.f;
    float lightPct = 0.f;
    float poisonPct = 0.f;
    float arcanePct = 0.f;
    float areaPct = 0.f;       // размер AoE
    float projSpeed = 0.f;
    float thorns = 0.f;        // ответный урон
    float goldFind = 0.f;
    float magicFind = 0.f;     // шанс лучшей редкости
    float xpGain = 0.f;
    float potionPotency = 0.f;
    float summonPct = 0.f;     // сила прислужников
    float durationPct = 0.f;   // длительность эффектов
    float extraProj = 0.f;     // +N снарядов
    float chainBonus = 0.f;    // +N целей цепи
    float resFire = 0.f;       // сопротивления (доли, капы применяются в Game)
    float resFrost = 0.f;
    float resLight = 0.f;
    float resPoison = 0.f;
    float resArcane = 0.f;
    float resAll = 0.f;
    float shieldFlat = 0.f;

    StatMods() {}

    StatMods &operator+=(const StatMods &o)
    {
        vit += o.vit; might += o.might; spirit += o.spirit; agi += o.agi; focus += o.focus;
        hpFlat += o.hpFlat; manaFlat += o.manaFlat; hpPct += o.hpPct; manaPct += o.manaPct;
        armor += o.armor; armorPct += o.armorPct;
        critChance += o.critChance; critMult += o.critMult; dodge += o.dodge;
        moveSpeed += o.moveSpeed; attackSpeed += o.attackSpeed; castSpeed += o.castSpeed;
        cdReduction += o.cdReduction; hpRegen += o.hpRegen; manaRegen += o.manaRegen;
        lifeSteal += o.lifeSteal; manaSteal += o.manaSteal;
        dmgPct += o.dmgPct; physPct += o.physPct; firePct += o.firePct; frostPct += o.frostPct;
        lightPct += o.lightPct; poisonPct += o.poisonPct; arcanePct += o.arcanePct;
        areaPct += o.areaPct; projSpeed += o.projSpeed; thorns += o.thorns;
        goldFind += o.goldFind; magicFind += o.magicFind; xpGain += o.xpGain;
        potionPotency += o.potionPotency; summonPct += o.summonPct; durationPct += o.durationPct;
        extraProj += o.extraProj; chainBonus += o.chainBonus;
        resFire += o.resFire; resFrost += o.resFrost; resLight += o.resLight;
        resPoison += o.resPoison; resArcane += o.resArcane; resAll += o.resAll;
        shieldFlat += o.shieldFlat;
        return *this;
    }
    StatMods operator+(const StatMods &o) const { StatMods r = *this; r += o; return r; }
    StatMods operator*(float k) const
    {
        StatMods r = *this;
        r.vit *= k; r.might *= k; r.spirit *= k; r.agi *= k; r.focus *= k;
        r.hpFlat *= k; r.manaFlat *= k; r.hpPct *= k; r.manaPct *= k;
        r.armor *= k; r.armorPct *= k; r.critChance *= k; r.critMult *= k; r.dodge *= k;
        r.moveSpeed *= k; r.attackSpeed *= k; r.castSpeed *= k; r.cdReduction *= k;
        r.hpRegen *= k; r.manaRegen *= k; r.lifeSteal *= k; r.manaSteal *= k;
        r.dmgPct *= k; r.physPct *= k; r.firePct *= k; r.frostPct *= k; r.lightPct *= k;
        r.poisonPct *= k; r.arcanePct *= k; r.areaPct *= k; r.projSpeed *= k; r.thorns *= k;
        r.goldFind *= k; r.magicFind *= k; r.xpGain *= k; r.potionPotency *= k;
        r.summonPct *= k; r.durationPct *= k; r.extraProj *= k; r.chainBonus *= k;
        r.resFire *= k; r.resFrost *= k; r.resLight *= k; r.resPoison *= k;
        r.resArcane *= k; r.resAll *= k; r.shieldFlat *= k;
        return r;
    }
    bool empty() const
    {
        StatMods z;
        const float *a = reinterpret_cast<const float *>(this);
        const float *b = reinterpret_cast<const float *>(&z);
        int n = int(sizeof(StatMods) / sizeof(float));
        for (int i = 0; i < n; ++i) if (qAbs(a[i]) > 1e-6f) return false;
        return true;
    }
};

// ============================================================
//  Аффиксы и предметы
// ============================================================
struct AffixDef
{
    int id = -1;
    QString id_str;
    QString name;        // «...медведя», «...пламени»
    bool prefix = true;
    int minIlvl = 1;
    StatMods mods;       // шаблон: значения-«вес» 1.0 = полная величина
    float rollMin = 0.7f;
    float rollMax = 1.3f;
    int weight = 100;
    int slotMask = 0xFFFF; // битовая маска разрешённых слотов
};

struct ItemBaseDef
{
    int id = -1;
    QString id_str;
    QString name;
    Slot slot = Slot_Weapon;
    BagKind bag = Bag_Equip;
    int tier = 1;          // 1..6, определяет reqLevel и силу базы
    int reqLevel = 1;
    StatMods base;
    float dmgMin = 0.f;    // для оружия
    float dmgMax = 0.f;
    float atkSpeed = 1.f;  // атак/сек
    DmgType dmgType = Dmg_Physical;
    int maxSockets = 0;
    int glyph = 0;         // индекс иконки
    int priceBase = 20;
};

struct UniqueDef
{
    int id = -1;
    QString id_str;
    QString name;
    QString flavor;
    int baseId = 0;
    int reqLevel = 5;
    QList<int> affixes;
    QList<float> rolls;
    QColor tint;
};

struct Item
{
    bool valid = false;
    int baseId = -1;       // ItemBaseDef::id
    int uniqueId = -1;     // UniqueDef::id или -1
    Rarity rarity = Rar_Normal;
    int ilvl = 1;
    QList<int> affixes;
    QList<float> rolls;
    QList<int> gems;       // baseId камня
    int count = 1;
    int seed = 1;          // для генерации имени редкого предмета

    QString name() const;
    Slot slot() const;
    bool isEquip() const;
    bool stackable() const;
};

// ============================================================
//  Активные умения
// ============================================================
struct ActiveSkill
{
    int id = -1;
    QString id_str;
    QString name;
    QString desc;
    SkillFx fx = Fx_Cleave;
    DmgType dmgType = Dmg_Physical;
    float cost = 6.f;
    float cd = 4.f;
    float dmgPct = 1.0f;     // множитель урона оружия/базы
    float radius = 60.f;
    float range = 120.f;
    float projSpeed = 420.f;
    float duration = 0.f;
    int projectiles = 1;
    int pierce = 0;
    int chains = 0;
    int reqLevel = 1;
    int maxRank = 5;
    QColor color;
    int glyph = 0;
    float scale = 0.f;       // статус (заморозка/поджог/метка) сила
};

// ============================================================
//  Узел паутины навыков
// ============================================================
struct SkillNode
{
    int id = -1;
    QString id_str;
    QString name;
    QString desc;
    NodeKind kind = Nk_Minor;
    int ring = 0;            // 0 - центр, 1..4 - круги
    int branch = -1;         // 0..3 ветвь, -1 общая
    float ang = 0.f;         // угол на паутине (радианы)
    float radius = 0.f;      // радиус на паутине (0..1)
    int cost = 1;            // очков за ранг
    int maxRank = 1;
    int reqLevel = 1;
    StatMods mods;           // за 1 ранг
    int activeSkill = -1;    // если Nk_Active
    QString keystone;        // id спец-механики
    QList<int> links;        // связи с другими узлами
    QColor color;
};

// ============================================================
//  Враги
// ============================================================
struct EnemyDef
{
    int id = -1;
    QString id_str;
    QString name;
    int level = 1;
    AiKind ai = Ai_Chase;
    float hp = 40.f;
    float dmgMin = 4.f, dmgMax = 7.f;
    DmgType dmgType = Dmg_Physical;
    float armor = 0.f;
    float speed = 90.f;
    float radius = 14.f;
    float atkRange = 26.f;
    float atkCd = 1.2f;
    float detectRange = 420.f;
    int xp = 10;
    int goldMin = 2, goldMax = 6;
    float dropChance = 0.14f;
    int tint = 0;            // индекс палитры спрайта
    int glyph = 0;
    bool flying = false;
    bool summoner = false;
    int summonId = -1;
    int summonCount = 1;
    float rangedProjSpeed = 300.f;
    bool explodes = false;
    float explodeRadius = 70.f;
    // боссы
    bool boss = false;
    QString title;           // «Пепельный Патриарх»
    int phases = 1;
    int patternSet = 0;      // набор скриптовых атак
    float scale = 1.f;       // визуальный размер
};

struct EliteAffix
{
    int id = 0;
    QString name;
    QString desc;
    QColor color;
};

// ============================================================
//  Локации и карта мира
// ============================================================
struct LocationDef
{
    int id = -1;
    QString id_str;
    QString name;
    QString desc;
    LocType type = Loc_Camp;
    Biome biome = Bio_Ash;
    int region = 0;          // 0..4
    int tier = 1;            // уровень контента 1..6
    int reqLevel = 1;
    QPointF mapPos;          // нормализованные координаты на карте 0..1
    QList<int> links;
    QVector<int> enemyPool;  // EnemyDef::id
    int enemyCount = 8;
    int eliteCount = 0;
    int bossId = -1;
    int seed = 0;
    QString rewardHint;
};

// ============================================================
//  Вспомогательные функции-перечисления
// ============================================================
QString slotName(Slot s);
QString rarityName(Rarity r);
QColor  rarityColor(Rarity r);
QString dmgTypeName(DmgType t);
QColor  dmgTypeColor(DmgType t);
QString biomeName(Biome b);
QString locTypeName(LocType t);
QColor  biomeColor(Biome b);
QString aiName(AiKind a);
int     slotBit(Slot s);

} // namespace aa

#endif // AA_TYPES_H
