#ifndef AA_GAME_H
#define AA_GAME_H

#include <QObject>
#include <QString>
#include <QList>
#include <QVector>
#include <QSet>
#include <QHash>
#include <QJsonObject>
#include <QColor>

#include "Core/Types.h"

namespace aa {

// Итоговые (производные) характеристики персонажа
struct Derived
{
    float vit = 0, might = 0, spirit = 0, agi = 0, focus = 0;
    float maxHp = 100, maxMana = 40;
    float hpRegen = 0, manaRegen = 0;
    float armor = 0;
    float critChance = 0.05f, critMult = 1.5f;
    float dodge = 0.f;
    float moveSpeed = 175.f;
    float attackSpeed = 1.f, castSpeed = 1.f, cdReduction = 0.f;
    float lifeSteal = 0.f, manaSteal = 0.f;
    float dmgPct = 0.f, areaPct = 0.f, projSpeed = 0.f;
    float physPct = 0.f, firePct = 0.f, frostPct = 0.f, lightPct = 0.f, poisonPct = 0.f, arcanePct = 0.f;
    float thorns = 0.f;
    float goldFind = 0.f, magicFind = 0.f, xpGain = 0.f, potionPotency = 0.f;
    float summonPct = 0.f, durationPct = 0.f;
    int   extraProj = 0, chainBonus = 0;
    float shieldMax = 0.f;
    float res[Dmg_Count];
    float dmgMin = 2.f, dmgMax = 4.f;
    DmgType weaponType = Dmg_Physical;
    float weaponSpeed = 1.f;
    bool  twoHanded = false;
    float elemBonus = 0.f;   // «Раскалённая сталь» и подобное

    Derived() { for (int i = 0; i < Dmg_Count; ++i) res[i] = 0.f; }
    float typePct(DmgType t) const
    {
        switch (t) {
        case Dmg_Fire: return firePct;
        case Dmg_Frost: return frostPct;
        case Dmg_Lightning: return lightPct;
        case Dmg_Poison: return poisonPct;
        case Dmg_Arcane: return arcanePct;
        case Dmg_Physical: return physPct;
        default: return 0.f;
        }
    }
    float resistOf(DmgType t) const
    {
        switch (t) {
        case Dmg_Fire: return res[Dmg_Fire];
        case Dmg_Frost: return res[Dmg_Frost];
        case Dmg_Lightning: return res[Dmg_Lightning];
        case Dmg_Poison: return res[Dmg_Poison];
        case Dmg_Arcane: return res[Dmg_Arcane];
        default: return 0.f;
        }
    }
};

// Благословения святилищ
enum ShrineBuff {
    Shrine_None = 0,
    Shrine_Might,      // +урон
    Shrine_Ward,       // +броня и барьер
    Shrine_Fortune,    // +золото и редкости
    Shrine_Swift,      // +скорость и откат
    Shrine_Arcane      // +мана и скорость заклинаний
};

class Game : public QObject
{
    Q_OBJECT
public:
    explicit Game(QObject *parent = 0);

    // ---------- жизненный цикл ----------
    bool saveGame(const QString &path = QString());
    bool loadGame(const QString &path = QString());
    static QString defaultSavePath();
    static bool saveExists(const QString &path = QString());
    static QStringList allSaves();

    // ---------- сложность ----------
    enum Diff { D_Easy = 0, D_Medium = 1, D_Hard = 2, D_Doom = 3 };
    void newGame(const QString &heroName, int classId = 0, int diff = -1);
    int  difficulty() const { return m_diff; }
    static QString diffName(int d);
    static QString diffDesc(int d);
    float dmgTakenMul() const;    // во сколько раз больнее получать
    float dmgDealtMul() const;    // во сколько больше наносить
    float xpMul() const;          // множитель опыта
    float goldMul() const;        // множитель золота
    bool  doomPermadeath() const { return m_diff == D_Doom; }

    // ---------- персонаж ----------
    QString heroName() const { return m_hero; }
    int classId() const { return m_class; }
    void setClassId(int c) { m_class = c; }
    int   level() const { return m_level; }
    float xp() const { return m_xp; }
    static float xpForLevel(int lvl);
    float xpProgress() const;

    float hp() const { return m_hp; }
    float mana() const { return m_mana; }
    float shield() const { return m_shield; }
    float maxHp() const { return m_d.maxHp; }
    float maxMana() const { return m_d.maxMana; }
    int   gold() const { return m_gold; }
    int   deaths() const { return m_deaths; }
    int   kills() const { return m_kills; }
    float playTime() const { return m_playTime; }

    void addHp(float v);
    void addMana(float v);
    void addShield(float v);
    void drainShield(float &dmg);   // сначала снимаем барьер
    void addGold(int v);
    void addXp(float v);
    void registerKill();
    void tick(float dt);             // регенерация, таймеры, бафф святилища

    const Derived &d() const { return m_d; }
    void recalc();

    // ---------- сумка и экипировка ----------
    static const int BAG_SIZE = 60;
    const QVector<Item> &bag() const { return m_bag; }
    QVector<Item> &bagRef() { return m_bag; }
    Item equipped(Slot s) const;
    const QVector<Item> &equips() const { return m_equip; }
    bool addItem(const Item &it, int *outIndex = 0);   // стек для расходников
    bool addItemAuto(const Item &it);
    void removeAt(int idx);
    bool equipFromBag(int idx);
    void unequipToBag(Slot s);
    void moveInBag(int from, int to);
    void sellItem(int idx);
    void dropItem(int idx);
    int  priceOf(const Item &it, bool selling) const;
    int  bagFreeSlots() const;
    int  countPotions() const;
    bool usePotion();               // авто-выбор: лечит то, что не полное
    bool usePotionAt(int bagIdx);   // конкретное зелье из сумки
    int  itemCount(int baseId) const;
    bool consumeItem(int baseId, int n = 1);
    bool useScrollOfReturn();
    bool useSoulShard();

    // ---------- генерация предметов ----------
    Item rollItem(int ilvl, float magicFindBonus = 0.f, int forcedRarity = -1, Slot forcedSlot = Slot_Weapon, bool slotForced = false);
    Item rollPotion(int ilvl);
    Item rollUnique(int ilvl);
    Item makeItem(int baseId, Rarity r, int ilvl);
    StatMods itemMods(const Item &it) const;
    float  itemDmgMin(const Item &it) const;
    float  itemDmgMax(const Item &it) const;
    int    itemScore(const Item &it) const;

    // ---------- паутина навыков ----------
    int  skillPoints() const { return m_points; }
    int  spentPoints() const;
    int  nodeRank(int id) const;
    bool nodeUnlocked(int id) const;              // взят хотя бы 1 ранг
    bool canTakeNode(int id, QString *reason = 0) const;
    bool takeNode(int id);
    bool canRefundNode(int id, QString *reason = 0) const;
    bool refundNode(int id);
    int  respecCost() const;
    bool respecAll();
    StatMods webMods() const;
    bool hasKeystone(const QString &id) const;
    QList<int> learnedSkills() const;             // активные умения из паутины
    int  skillRank(int skillId) const;            // ранг умения (из узла)
    QVector<int> takenNodes() const;

    // ---------- панель умений ----------
    int  hotbar(int i) const;
    bool setHotbar(int i, int skillId);
    float skillCost(int skillId) const;
    float skillCooldown(int skillId) const;
    float skillDamagePct(int skillId) const;

    // ---------- мир ----------
    int  currentLocation() const { return m_curLoc; }
    bool isUnlocked(int locId) const;
    bool isCleared(int locId) const;
    QSet<int> unlockedSet() const { return m_unlocked; }
    QSet<int> clearedSet() const { return m_cleared; }
    int  clearCount(int locId) const { return m_clears.value(locId, 0); }
    bool travelTo(int locId);
    void enterLocation(int locId);
    void visitLocation(int locId);   // посещение поселения: открывает соседние пути
    void completeLocation(int locId, bool bossKilled);
    int  lastTown() const { return m_lastTown; }
    void setLastTown(int id) { m_lastTown = id; }
    int  locationTier(int locId) const;

    // ---------- святилища ----------
    void applyShrine(ShrineBuff b, float seconds);
    ShrineBuff shrine() const { return m_shrine; }
    float shrineTime() const { return m_shrineTime; }
    StatMods shrineMods() const;

    // ---------- смерть ----------
    void onPlayerDeath();

    // ---------- лог ----------
    void log(const QString &msg, const QColor &c = QColor());

    // ---------- JSON ----------
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject &o);
    static QJsonObject itemToJson(const Item &it);
    static Item itemFromJson(const QJsonObject &o);

    int version() const { return m_version; }
    quint64 rngState() const { return m_rngState; }

signals:
    void statsChanged();
    void inventoryChanged();
    void resourcesChanged();
    void webChanged();
    void worldChanged();
    void leveledUp(int newLevel);
    void logMessage(QString msg, QColor color);
    void playerDied();
    void locationFinished(int locId);
    void goldChanged(int gold);

private:
    void unlockNeighbors(int locId);
    void addXpInternal(float v);

    QString m_hero;
    int     m_class = 0;
    int     m_diff = 1;                 // сложность: см. enum Diff
    int     m_level = 1;
    float   m_xp = 0.f;
    float   m_hp = 100.f, m_mana = 40.f, m_shield = 0.f;
    int     m_gold = 0;
    int     m_deaths = 0, m_kills = 0;
    float   m_playTime = 0.f;
    float   m_playSec = 0.f;
    int     m_version = 1;
    quint64 m_rngState = 0;

    QVector<Item> m_bag;
    QVector<Item> m_equip;              // Slot_EquipCount
    QHash<int, int> m_nodeRank;         // id узла -> ранг
    int     m_points = 0;
    int     m_hotbar[6];

    QSet<int> m_unlocked;
    QSet<int> m_cleared;
    QHash<int, int> m_clears;
    int     m_curLoc = 0;
    int     m_lastTown = 0;

    ShrineBuff m_shrine = Shrine_None;
    float   m_shrineTime = 0.f;

    Derived m_d;
};

// сериализация предмета
QJsonObject itemToJson(const Item &it);
Item itemFromJson(const QJsonObject &o);

} // namespace aa

#endif // AA_GAME_H
