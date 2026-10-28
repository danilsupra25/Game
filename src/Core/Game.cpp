#include "Core/Game.h"
#include "Core/Data.h"
#include "Core/Style.h"
#include "Core/Rng.h"
#include "Core/Ach.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QSettings>
#include <QtMath>

namespace aa {

// -------------------------------------------------------------
static int bid(const QString &s)
{
    const QVector<ItemBaseDef> &b = Data::bases();
    for (int i = 0; i < b.size(); ++i) if (b[i].id_str == s) return i;
    return -1;
}
static int nid(const QString &s)
{
    const QVector<SkillNode> &n = Data::nodes();
    for (int i = 0; i < n.size(); ++i) if (n[i].id_str == s) return i;
    return -1;
}
static int skid(const QString &s)
{
    const QVector<ActiveSkill> &k = Data::skills();
    for (int i = 0; i < k.size(); ++i) if (k[i].id_str == s) return i;
    return -1;
}
static int uidByStr(const QString &s)
{
    const QVector<UniqueDef> &u = Data::uniques();
    for (int i = 0; i < u.size(); ++i) if (u[i].id_str == s) return i;
    return -1;
}

static float clampRes(float v) { return qBound(-0.50f, v, 0.75f); }

// -------------------------------------------------------------
Game::Game(QObject *parent) : QObject(parent)
{
    Data::ensureLoaded();
    m_bag.resize(BAG_SIZE);
    m_equip.resize(Slot_EquipCount);
    for (int i = 0; i < 6; ++i) m_hotbar[i] = -1;
    recalc();
    m_hp = m_d.maxHp;
    m_mana = m_d.maxMana;
}

QString Game::defaultSavePath()
{
    QString dir;
    QString app = QCoreApplication::applicationDirPath();
    if (!app.isEmpty()) dir = app + QStringLiteral("/saves");
    else dir = QDir::homePath() + QStringLiteral("/.ashamber");
    QDir().mkpath(dir);
    return dir + QStringLiteral("/pepel_i_yantar.json");
}

bool Game::saveExists(const QString &path)
{
    QString p = path.isEmpty() ? defaultSavePath() : path;
    return QFileInfo(p).exists();
}

void Game::deleteSave(const QString &path)
{
    QFile::remove(path.isEmpty() ? defaultSavePath() : path);
}

QStringList Game::allSaves()
{
    QDir d(QFileInfo(defaultSavePath()).absolutePath());
    QStringList out;
    QStringList fl = d.entryList(QStringList() << QStringLiteral("*.json"), QDir::Files, QDir::Name);
    for (int i = 0; i < fl.size(); ++i) out << d.absoluteFilePath(fl.at(i));
    return out;
}

// -------------------------------------------------------------
//  НОВАЯ ИГРА
// -------------------------------------------------------------
void Game::newGame(const QString &heroName, int classId, int diff)
{
    Data::ensureLoaded();
    m_hero = heroName.isEmpty() ? QStringLiteral("Безымянный") : heroName;
    m_class = qBound(0, classId, 3);
    // diff == -1: брать из настроек (TitleScreen их запоминает), иначе явный выбор
    if (diff < 0) {
        QSettings st(QStringLiteral("AshAndAmber"), QStringLiteral("AshAndAmber"));
        m_diff = qBound(0, st.value(QStringLiteral("game/difficulty"), 1).toInt(), 3);
    } else {
        m_diff = qBound(0, diff, 3);
        QSettings st(QStringLiteral("AshAndAmber"), QStringLiteral("AshAndAmber"));
        st.setValue(QStringLiteral("game/difficulty"), m_diff);
    }
    m_level = 1;
    m_xp = 0;
    m_gold = 60;
    m_deaths = 0;
    m_kills = 0;
    m_playTime = 0;
    m_points = 1;
    m_bag = QVector<Item>(BAG_SIZE);
    m_equip = QVector<Item>(Slot_EquipCount);
    m_nodeRank.clear();
    for (int i = 0; i < 6; ++i) m_hotbar[i] = -1;
    m_unlocked.clear();
    m_cleared.clear();
    m_clears.clear();
    m_curLoc = Data::firstLocation();
    m_lastTown = Data::firstLocation();
    m_unlocked.insert(Data::firstLocation());
    m_shrine = Shrine_None;
    m_shrineTime = 0;
    m_rngState = rng().state();

    // стартовый узел паутины
    m_nodeRank.insert(0, 1);

    Item it;
    int wid = -1, oid = -1, nodeToTake = -1, skillToBind = -1;
    switch (classId) {
    case 0: // Воин
        wid = bid(QStringLiteral("w_rusty")); oid = bid(QStringLiteral("o_wshield"));
        nodeToTake = nid(QStringLiteral("n2"));  // «Крепость»
        skillToBind = skid(QStringLiteral("cleave"));
        break;
    case 1: // Маг
        wid = bid(QStringLiteral("w_appstf")); oid = bid(QStringLiteral("o_torch"));
        nodeToTake = nid(QStringLiteral("n4"));  // «Ясность»
        skillToBind = skid(QStringLiteral("fireball"));
        break;
    case 2: // Странник
        wid = bid(QStringLiteral("w_sbow")); oid = -1;
        nodeToTake = nid(QStringLiteral("n6"));  // «Лёгкость»
        skillToBind = skid(QStringLiteral("volley"));
        break;
    default: // Жрец пепла
        wid = bid(QStringLiteral("w_club")); oid = bid(QStringLiteral("o_totem1"));
        nodeToTake = nid(QStringLiteral("n8"));  // «Тление»
        skillToBind = skid(QStringLiteral("mend"));
        break;
    }
    if (wid >= 0) { it = makeItem(wid, Rar_Normal, 1); m_equip[Slot_Weapon] = it; }
    if (oid >= 0) { it = makeItem(oid, Rar_Normal, 1); m_equip[Slot_Offhand] = it; }
    it = makeItem(bid(QStringLiteral("c_rags")), Rar_Normal, 1); m_equip[Slot_Chest] = it;
    it = makeItem(bid(QStringLiteral("t_sandal")), Rar_Normal, 1); m_equip[Slot_Boots] = it;

    // стартовое активное умение - бесплатно
    int startActive = -1;
    switch (classId) {
    case 0:  startActive = nid(QStringLiteral("n2")); break;
    case 1:  startActive = nid(QStringLiteral("n4")); break;
    case 2:  startActive = nid(QStringLiteral("n6")); break;
    default: startActive = nid(QStringLiteral("n8")); break;
    }
    Q_UNUSED(startActive);
    const QVector<SkillNode> &nodes = Data::nodes();
    for (int i = 0; i < nodes.size(); ++i) {
        if (nodes[i].kind == Nk_Active && nodes[i].activeSkill == skillToBind) {
            m_nodeRank.insert(i, 1);
            break;
        }
    }
    if (nodeToTake >= 0) m_nodeRank.insert(nodeToTake, 1);
    if (skillToBind >= 0) m_hotbar[0] = skillToBind;

    // стартовая сумка
    Item pot = makeItem(bid(QStringLiteral("p_hp1")), Rar_Normal, 1); pot.count = 5; addItem(pot);
    Item mp = makeItem(bid(QStringLiteral("p_mp1")), Rar_Normal, 1); mp.count = 3; addItem(mp);
    Item sc = makeItem(bid(QStringLiteral("m_tp")), Rar_Normal, 1); sc.count = 2; addItem(sc);

    recalc();
    m_hp = m_d.maxHp;
    m_mana = m_d.maxMana;
    m_shield = 0;

    emit statsChanged();
    emit inventoryChanged();
    emit webChanged();
    emit worldChanged();
    emit goldChanged(m_gold);
    log(QStringLiteral("Добро пожаловать в Пепел и Янтарь, %1.").arg(m_hero), C::amber);
    log(QStringLiteral("WASD - движение, ЛКМ - атака, ПКМ - кувырок, 1..6 - умения, I - сумка, K - паутина, M - карта."), C::boneDim);
}

// -------------------------------------------------------------
//  ПРОИЗВОДНЫЕ ХАРАКТЕРИСТИКИ
// -------------------------------------------------------------
StatMods Game::webMods() const
{
    StatMods s;
    const QVector<SkillNode> &nodes = Data::nodes();
    QHash<int, int>::const_iterator it = m_nodeRank.constBegin();
    for (; it != m_nodeRank.constEnd(); ++it) {
        int id = it.key();
        int rank = it.value();
        if (id < 0 || id >= nodes.size() || rank <= 0) continue;
        s += nodes[id].mods * float(rank);
    }
    return s;
}

StatMods Game::shrineMods() const
{
    StatMods s;
    if (m_shrine == Shrine_None || m_shrineTime <= 0.f) return s;
    switch (m_shrine) {
    case Shrine_Might:   s.dmgPct = 0.30f; s.physPct = 0.15f; break;
    case Shrine_Ward:    s.armor = 120.f; s.shieldFlat = 150.f; s.resAll = 0.10f; break;
    case Shrine_Fortune: s.goldFind = 0.60f; s.magicFind = 0.45f; break;
    case Shrine_Swift:   s.moveSpeed = 0.18f; s.cdReduction = 0.15f; s.attackSpeed = 0.12f; break;
    case Shrine_Arcane:  s.manaRegen = 6.f; s.castSpeed = 0.25f; s.manaFlat = 60.f; break;
    default: break;
    }
    return s;
}

StatMods Game::itemMods(const Item &it) const
{
    StatMods s;
    if (!it.valid) return s;
    const ItemBaseDef *b = Data::base(it.baseId);
    if (!b) return s;
    if (b->bag != Bag_Equip) return s;
    s += b->base;
    bool isUnq = (it.rarity == Rar_Unique);
    for (int i = 0; i < it.affixes.size(); ++i) {
        const AffixDef *a = Data::affix(it.affixes.at(i));
        if (!a) continue;
        float roll = i < it.rolls.size() ? it.rolls.at(i) : 1.0f;
        s += a->mods * roll;
    }
    if (!isUnq) {
        // небольшое усиление за уровень предмета
        float k = 1.0f + 0.02f * float(qMax(0, it.ilvl - b->reqLevel));
        s = s * k;
    }
    return s;
}

void Game::recalc()
{
    Derived d;
    const float lvl = float(m_level);

    // базовые атрибуты от уровня
    d.vit    = 8.f + 4.0f * lvl;
    d.might  = 5.f + 2.0f * lvl;
    d.spirit = 5.f + 2.0f * lvl;
    d.agi    = 5.f + 2.0f * lvl;
    d.focus  = 5.f + 2.0f * lvl;

    StatMods mods = webMods();
    for (int i = 0; i < m_equip.size(); ++i) mods += itemMods(m_equip[i]);
    mods += shrineMods();

    d.vit += mods.vit; d.might += mods.might; d.spirit += mods.spirit;
    d.agi += mods.agi; d.focus += mods.focus;

    float hpBase = 62.f + 15.f * lvl + d.vit * 6.0f;
    d.maxHp = hpBase * (1.f + mods.hpPct) + mods.hpFlat;
    float mpBase = 26.f + 7.f * lvl + d.focus * 4.0f;
    d.maxMana = mpBase * (1.f + mods.manaPct) + mods.manaFlat;

    d.hpRegen = 0.6f + d.vit * 0.055f + mods.hpRegen;
    d.manaRegen = 1.4f + d.focus * 0.09f + mods.manaRegen;

    d.armor = (mods.armor + d.might * 0.45f) * (1.f + mods.armorPct);
    if (hasKeystone(QStringLiteral("unbreakable"))) d.armor *= 1.35f;

    d.critChance = clampf(0.04f + d.agi * 0.0018f + mods.critChance, 0.f, 0.80f);
    d.critMult = 1.5f + mods.critMult;
    d.dodge = clampf(d.agi * 0.0013f + mods.dodge, 0.f, 0.50f);
    d.moveSpeed = 178.f * (1.f + mods.moveSpeed + d.agi * 0.0012f);
    d.attackSpeed = 1.f + mods.attackSpeed + d.agi * 0.0035f;
    d.castSpeed = 1.f + mods.castSpeed + d.focus * 0.0035f;

    float cdr = mods.cdReduction;
    if (hasKeystone(QStringLiteral("timewarp"))) cdr += 0.06f * (d.attackSpeed - 1.f) + 0.05f;
    d.cdReduction = clampf(cdr, 0.f, 0.65f);

    d.lifeSteal = clampf(mods.lifeSteal, 0.f, 0.45f);
    d.manaSteal = clampf(mods.manaSteal, 0.f, 0.40f);
    d.dmgPct = mods.dmgPct;
    d.areaPct = mods.areaPct;
    d.projSpeed = mods.projSpeed;
    d.thorns = mods.thorns + d.might * 0.25f;
    d.goldFind = mods.goldFind;
    d.magicFind = mods.magicFind;
    d.xpGain = mods.xpGain;
    d.potionPotency = mods.potionPotency;
    d.summonPct = mods.summonPct;
    d.durationPct = mods.durationPct;
    d.extraProj = int(mods.extraProj + 0.5f);
    d.chainBonus = int(mods.chainBonus + 0.5f);
    d.shieldMax = mods.shieldFlat;

    d.physPct = mods.physPct;
    d.firePct = mods.firePct;
    d.frostPct = mods.frostPct;
    d.lightPct = mods.lightPct;
    d.poisonPct = mods.poisonPct;
    d.arcanePct = mods.arcanePct;

    d.res[Dmg_Physical] = clampRes(d.armor / (d.armor + 260.f));
    d.res[Dmg_Fire] = clampRes(mods.resAll + mods.resFire + d.focus * 0.0006f);
    d.res[Dmg_Frost] = clampRes(mods.resAll + mods.resFrost + d.focus * 0.0006f);
    d.res[Dmg_Lightning] = clampRes(mods.resAll + mods.resLight + d.focus * 0.0006f);
    d.res[Dmg_Poison] = clampRes(mods.resAll + mods.resPoison + d.vit * 0.0008f);
    d.res[Dmg_Arcane] = clampRes(mods.resAll + mods.resArcane + d.focus * 0.0006f);
    d.res[Dmg_Pure] = 0.f;

    // оружие
    const Item &w = m_equip[Slot_Weapon];
    if (w.valid) {
        const ItemBaseDef *b = Data::base(w.baseId);
        if (b) {
            d.weaponType = b->dmgType;
            float scale = 1.f + 0.02f * float(qMax(0, w.ilvl - b->reqLevel));
            d.dmgMin = itemDmgMin(w) * scale;
            d.dmgMax = itemDmgMax(w) * scale;
            d.weaponSpeed = b->atkSpeed;
            d.twoHanded = (b->glyph == Gl_GreatSword || b->glyph == Gl_GreatAxe || b->glyph == Gl_Staff);
        }
    } else {
        d.weaponType = Dmg_Physical;
        d.dmgMin = 2.f + lvl * 0.6f;
        d.dmgMax = 4.f + lvl * 0.9f;
        d.weaponSpeed = 1.0f;
    }

    float attrBonus = 1.f + (d.weaponType == Dmg_Physical ? d.might : d.spirit) * 0.035f;
    float typePct = 1.f + d.dmgPct + d.typePct(d.weaponType);
    d.dmgMin *= attrBonus * typePct;
    d.dmgMax *= attrBonus * typePct;
    d.weaponSpeed *= (d.weaponType == Dmg_Physical ? d.attackSpeed : d.castSpeed);
    // общий разгон атаки: раньше бои затягивались, теперь темп живее (~x1.35)
    d.weaponSpeed *= 1.35f;
    // жёсткий потолок скорости атаки: даже с горой бонусов быстрее ~3.4 удара/с не выйдет
    d.weaponSpeed = qBound(0.55f, d.weaponSpeed, 3.4f);

    d.elemBonus = hasKeystone(QStringLiteral("searedsteel")) ? 0.30f : 0.f;

    m_d = d;
    m_hp = qMin(m_hp, m_d.maxHp);
    m_mana = qMin(m_mana, m_d.maxMana);
    m_shield = qMin(m_shield, m_d.shieldMax);
}

float Game::itemDmgMin(const Item &it) const
{
    const ItemBaseDef *b = Data::base(it.baseId);
    if (!b) return 0.f;
    float v = b->dmgMin;
    StatMods s = itemMods(it);
    v *= (1.f + s.physPct * 0.5f + s.dmgPct * 0.5f);
    return v;
}

float Game::itemDmgMax(const Item &it) const
{
    const ItemBaseDef *b = Data::base(it.baseId);
    if (!b) return 0.f;
    float v = b->dmgMax;
    StatMods s = itemMods(it);
    v *= (1.f + s.physPct * 0.5f + s.dmgPct * 0.5f);
    return v;
}

int Game::itemScore(const Item &it) const
{
    if (!it.valid) return 0;
    const ItemBaseDef *b = Data::base(it.baseId);
    if (!b) return 0;
    StatMods s = itemMods(it);
    float v = 0;
    v += (b->dmgMax + b->dmgMin) * 0.5f * b->atkSpeed * 1.2f;
    v += s.hpFlat * 0.20f + s.hpPct * 220.f + s.manaFlat * 0.15f;
    v += s.vit + s.might + s.spirit + s.agi + s.focus;
    v += s.armor * 0.22f + s.armorPct * 300.f;
    v += s.critChance * 320.f + s.critMult * 130.f + s.dodge * 300.f;
    v += s.moveSpeed * 260.f + s.attackSpeed * 300.f + s.castSpeed * 300.f + s.cdReduction * 320.f;
    v += s.dmgPct * 300.f + s.physPct * 200.f + s.firePct * 200.f + s.frostPct * 200.f
       + s.lightPct * 200.f + s.poisonPct * 200.f + s.arcanePct * 200.f;
    v += s.lifeSteal * 700.f + s.hpRegen * 12.f + s.manaRegen * 10.f + s.thorns * 0.8f;
    v += s.goldFind * 60.f + s.magicFind * 70.f + s.xpGain * 90.f + s.summonPct * 120.f;
    v += s.resAll * 420.f + (s.resFire + s.resFrost + s.resLight + s.resPoison + s.resArcane) * 90.f;
    v += s.shieldFlat * 0.6f + s.extraProj * 120.f + s.chainBonus * 90.f + s.areaPct * 130.f;
    v += float(it.rarity) * 25.f + float(it.ilvl) * 1.5f;
    return int(v);
}

int Game::priceOf(const Item &it, bool selling) const
{
    if (!it.valid) return 0;
    const ItemBaseDef *b = Data::base(it.baseId);
    if (!b) return 0;
    float v = float(b->priceBase);
    v *= (1.f + 0.30f * float(it.ilvl));
    v *= (1.f + 0.55f * float(it.affixes.size()));
    static const float rmul[Rar_Count] = { 1.f, 1.7f, 3.0f, 5.5f, 10.0f };
    v *= rmul[int(it.rarity)];
    v += itemScore(it) * 0.10f;
    v = qMin(v, 24000.f);
    if (it.rarity == Rar_Unique) v = qMax(v, 400.f);
    int price = qMax(1, int(v));
    return selling ? qMax(1, price / 4) : price;
}

// -------------------------------------------------------------
//  СУМКА
// -------------------------------------------------------------
Item Game::equipped(Slot s) const
{
    if (int(s) < 0 || int(s) >= m_equip.size()) return Item();
    return m_equip[int(s)];
}

bool Game::addItem(const Item &it, int *outIndex)
{
    if (!it.valid) return false;
    if (it.stackable()) {
        for (int i = 0; i < m_bag.size(); ++i) {
            if (m_bag[i].valid && m_bag[i].baseId == it.baseId && m_bag[i].count < 99) {
                m_bag[i].count += it.count;
                if (outIndex) *outIndex = i;
                emit inventoryChanged();
                return true;
            }
        }
    }
    for (int i = 0; i < m_bag.size(); ++i) {
        if (!m_bag[i].valid) {
            m_bag[i] = it;
            if (outIndex) *outIndex = i;
            emit inventoryChanged();
            return true;
        }
    }
    log(QStringLiteral("Сумка полна - %1 теряется в пепле.").arg(Data::itemNameFull(it)), C::bad);
    return false;
}

bool Game::addItemAuto(const Item &it) { return addItem(it); }

void Game::removeAt(int idx)
{
    if (idx < 0 || idx >= m_bag.size()) return;
    m_bag[idx] = Item();
    emit inventoryChanged();
}

bool Game::equipFromBag(int idx)
{
    if (idx < 0 || idx >= m_bag.size() || !m_bag[idx].valid) return false;
    const Item &it = m_bag[idx];
    if (!it.isEquip()) return false;
    const ItemBaseDef *b = Data::base(it.baseId);
    if (!b) return false;
    if (m_level < b->reqLevel) {
        log(QStringLiteral("Нужен %1 уровень для «%2».").arg(b->reqLevel).arg(Data::itemNameFull(it)), C::bad);
        return false;
    }
    Slot s = b->slot;
    if (s == Slot_Ring1) {
        // кольцо: в свободный слот
        if (!m_equip[Slot_Ring1].valid) s = Slot_Ring1;
        else if (!m_equip[Slot_Ring2].valid) s = Slot_Ring2;
        else s = Slot_Ring1;
    }
    Item old = m_equip[int(s)];
    m_equip[int(s)] = it;
    m_bag[idx] = old.valid ? old : Item();
    recalc();
    emit inventoryChanged();
    emit statsChanged();
    return true;
}

void Game::unequipToBag(Slot s)
{
    if (int(s) < 0 || int(s) >= m_equip.size()) return;
    if (!m_equip[int(s)].valid) return;
    Item it = m_equip[int(s)];
    int idx = -1;
    if (!addItem(it, &idx)) return;
    m_equip[int(s)] = Item();
    recalc();
    emit inventoryChanged();
    emit statsChanged();
}

void Game::moveInBag(int from, int to)
{
    if (from < 0 || from >= m_bag.size() || to < 0 || to >= m_bag.size() || from == to) return;
    Item a = m_bag[from], b = m_bag[to];
    if (a.valid && b.valid && a.stackable() && b.stackable() && a.baseId == b.baseId) {
        b.count += a.count;
        m_bag[to] = b;
        m_bag[from] = Item();
    } else {
        m_bag[to] = a;
        m_bag[from] = b;
    }
    emit inventoryChanged();
}

void Game::sellItem(int idx)
{
    if (idx < 0 || idx >= m_bag.size() || !m_bag[idx].valid) return;
    int p = priceOf(m_bag[idx], true);
    addGold(p);
    log(QStringLiteral("Продано: %1 (+%2 золота)").arg(Data::itemNameFull(m_bag[idx])).arg(p), C::gold);
    m_bag[idx] = Item();
    emit inventoryChanged();
}

void Game::dropItem(int idx)
{
    if (idx < 0 || idx >= m_bag.size()) return;
    m_bag[idx] = Item();
    emit inventoryChanged();
}

int Game::bagFreeSlots() const
{
    int n = 0;
    for (int i = 0; i < m_bag.size(); ++i) if (!m_bag[i].valid) ++n;
    return n;
}

int Game::countPotions() const
{
    int n = 0;
    for (int i = 0; i < m_bag.size(); ++i) {
        const ItemBaseDef *b = m_bag[i].valid ? Data::base(m_bag[i].baseId) : 0;
        if (b && b->bag == Bag_Potion && b->glyph == Gl_PotionHp) n += m_bag[i].count;
    }
    return n;
}

int Game::itemCount(int baseId) const
{
    int n = 0;
    for (int i = 0; i < m_bag.size(); ++i)
        if (m_bag[i].valid && m_bag[i].baseId == baseId) n += m_bag[i].count;
    return n;
}

bool Game::consumeItem(int baseId, int n)
{
    int left = n;
    for (int i = 0; i < m_bag.size() && left > 0; ++i) {
        if (m_bag[i].valid && m_bag[i].baseId == baseId) {
            int take = qMin(left, m_bag[i].count);
            m_bag[i].count -= take;
            left -= take;
            if (m_bag[i].count <= 0) m_bag[i] = Item();
        }
    }
    emit inventoryChanged();
    return left == 0;
}

bool Game::usePotion()
{
    float hpMiss = m_d.maxHp - m_hp;
    float mpMiss = m_d.maxMana - m_mana;
    bool needHp = hpMiss > m_d.maxHp * 0.015f;
    bool needMp = mpMiss > m_d.maxMana * 0.015f;
    if (!needHp && !needMp) {
        log(QStringLiteral("Пить не нужно: здоровье и мана полны."), C::boneDim);
        return false;
    }
    // ищем подходящее: сначала то, что нужнее
    int wantGlyph = (needHp && hpMiss / qMax(1.f, m_d.maxHp) >= mpMiss / qMax(1.f, m_d.maxMana))
                    ? Gl_PotionHp : Gl_PotionMp;
    if (wantGlyph == Gl_PotionHp && !needHp) wantGlyph = Gl_PotionMp;
    if (wantGlyph == Gl_PotionMp && !needMp) wantGlyph = Gl_PotionHp;
    for (int pass = 0; pass < 2; ++pass) {
        int glyph = pass == 0 ? wantGlyph : (wantGlyph == Gl_PotionHp ? Gl_PotionMp : Gl_PotionHp);
        bool need = (glyph == Gl_PotionHp) ? needHp : needMp;
        if (!need) continue;
        int best = -1, bestVal = 0;
        for (int i = 0; i < m_bag.size(); ++i) {
            if (!m_bag[i].valid) continue;
            const ItemBaseDef *b = Data::base(m_bag[i].baseId);
            if (!b || b->bag != Bag_Potion || b->glyph != glyph) continue;
            int v = (glyph == Gl_PotionHp) ? int(b->base.hpFlat) : int(b->base.manaFlat);
            if (v > bestVal) { bestVal = v; best = i; }
        }
        if (best < 0) continue;
        const ItemBaseDef *b = Data::base(m_bag[best].baseId);
        if (glyph == Gl_PotionHp) {
            addHp(b->base.hpFlat * (1.f + m_d.potionPotency));
            log(QStringLiteral("Выпито: %1").arg(b->name), C::hpHi);
        } else {
            addMana(b->base.manaFlat * (1.f + m_d.potionPotency));
            log(QStringLiteral("Выпито: %1").arg(b->name), C::manaHi);
        }
        m_bag[best].count -= 1;
        if (m_bag[best].count <= 0) m_bag[best] = Item();
        AchTracker::inst().bump(Ach::C_Potions);
        emit inventoryChanged();
        emit resourcesChanged();
        return true;
    }
    log(needHp ? QStringLiteral("Нет зелья здоровья!") : QStringLiteral("Нет зелья маны!"), C::bad);
    return false;
}

bool Game::usePotionAt(int bagIdx)
{
    if (bagIdx < 0 || bagIdx >= m_bag.size() || !m_bag[bagIdx].valid) return false;
    const ItemBaseDef *b = Data::base(m_bag[bagIdx].baseId);
    if (!b || b->bag != Bag_Potion) return false;
    if (b->glyph == Gl_PotionHp && m_hp >= m_d.maxHp - 0.5f) {
        log(QStringLiteral("Здоровье уже полное - зелье не потрачено."), C::boneDim);
        return false;
    }
    if (b->glyph == Gl_PotionMp && m_mana >= m_d.maxMana - 0.5f) {
        log(QStringLiteral("Мана уже полная - зелье не потрачено."), C::boneDim);
        return false;
    }
    if (b->glyph == Gl_PotionHp) {
        addHp(b->base.hpFlat * (1.f + m_d.potionPotency));
        log(QStringLiteral("Выпито: %1").arg(b->name), C::hpHi);
    } else if (b->glyph == Gl_PotionMp) {
        addMana(b->base.manaFlat * (1.f + m_d.potionPotency));
        log(QStringLiteral("Выпито: %1").arg(b->name), C::manaHi);
    } else {
        // эликсир ярости - бафф, пить можно всегда
        applyShrine(Shrine_Might, 8.f);
        log(QStringLiteral("Выпито: %1").arg(b->name), C::ember);
    }
    m_bag[bagIdx].count -= 1;
    if (m_bag[bagIdx].count <= 0) m_bag[bagIdx] = Item();
    AchTracker::inst().bump(Ach::C_Potions);
    emit inventoryChanged();
    emit resourcesChanged();
    return true;
}

bool Game::useScrollOfReturn() { return consumeItem(bid(QStringLiteral("m_tp")), 1); }
bool Game::useSoulShard()
{
    if (!consumeItem(bid(QStringLiteral("m_soul")), 1)) return false;
    respecAll();
    log(QStringLiteral("Осколок души рассыпался - паутина навыков очищена."), C::arcane);
    return true;
}

// самоцветы и гнёзда убраны из игры (см. Data.cpp)

// -------------------------------------------------------------
//  ГЕНЕРАЦИЯ ПРЕДМЕТОВ
// -------------------------------------------------------------
Item Game::makeItem(int baseId, Rarity r, int ilvl)
{
    Item it;
    it.valid = baseId >= 0;
    it.baseId = baseId;
    it.rarity = r;
    it.ilvl = qMax(1, ilvl);
    it.uniqueId = -1;
    it.count = 1;
    it.seed = int(rng().next() & 0x7FFFFFFF);
    return it;
}

Item Game::rollPotion(int ilvl)
{
    QVector<int> opt;
    if (ilvl >= 14) opt << bid(QStringLiteral("p_hp3")) << bid(QStringLiteral("p_mp2"));
    if (ilvl >= 6)  opt << bid(QStringLiteral("p_hp2")) << bid(QStringLiteral("p_mp1")) << bid(QStringLiteral("p_rage"));
    opt << bid(QStringLiteral("p_hp1")) << bid(QStringLiteral("p_mp1"));
    int b = opt.at(rng().pick(opt.size()));
    Item it = makeItem(b, Rar_Normal, ilvl);
    it.count = rng().range(1, 2);
    return it;
}

Item Game::rollUnique(int ilvl)
{
    const QVector<UniqueDef> &U = Data::uniques();
    QVector<int> pool;
    for (int i = 0; i < U.size(); ++i) if (U[i].reqLevel <= ilvl + 3) pool.append(i);
    if (pool.isEmpty()) pool.append(rng().pick(U.size()));
    const UniqueDef &u = U.at(pool.at(rng().pick(pool.size())));
    Item it = makeItem(u.baseId, Rar_Unique, qMax(u.reqLevel, ilvl));
    it.uniqueId = u.id;
    it.affixes = u.affixes;
    it.rolls = u.rolls;
    return it;
}

Item Game::rollItem(int ilvl, float magicFindBonus, int forcedRarity, Slot forcedSlot, bool slotForced)
{
    Data::ensureLoaded();
    ilvl = qMax(1, ilvl);

    // 1) редкость
    Rarity r = Rar_Normal;
    if (forcedRarity >= 0) r = Rarity(qBound(0, forcedRarity, int(Rar_Count) - 1));
    else {
        float mf = 1.f + magicFindBonus + m_d.magicFind * 0.5f;
        QVector<int> w;
        w << 1000
          << int((430.f + ilvl * 9.f) * mf)
          << int((120.f + ilvl * 7.5f) * mf)
          << int((14.f + ilvl * 1.9f) * mf)
          << int((3.f + ilvl * 0.62f) * mf);
        int idx = rng().weighted(w);
        r = Rarity(idx);
    }
    if (r == Rar_Unique) {
        const QVector<UniqueDef> &U = Data::uniques();
        bool ok = false;
        for (int i = 0; i < U.size(); ++i) if (U[i].reqLevel <= ilvl + 3) { ok = true; break; }
        if (!ok) r = Rar_Legend;
    }
    if (r == Rar_Unique) return rollUnique(ilvl);

    // 2) слот
    Slot slot = forcedSlot;
    if (!slotForced) {
        QVector<int> w;
        w << 22 << 11 << 11 << 13 << 10 << 8 << 10 << 6 << 8 << 8;
        int idx = rng().weighted(w);
        slot = Slot(qBound(0, idx, int(Slot_EquipCount) - 1));
    }
    if (slot == Slot_Ring2) slot = Slot_Ring1;

    // 3) база
    QVector<int> pool = Data::basesForSlot(slot, ilvl);
    if (pool.isEmpty()) return Item();
    QVector<int> pw;
    for (int i = 0; i < pool.size(); ++i) {
        const ItemBaseDef &b = *Data::base(pool.at(i));
        int diff = ilvl - b.reqLevel;
        int wgt = 100 - qAbs(diff) * 9;
        pw << qMax(4, wgt);
    }
    int baseId = pool.at(rng().weighted(pw));
    const ItemBaseDef *bb = Data::base(baseId);

    Item it = makeItem(baseId, r, ilvl);

    // 4) аффиксы
    int nAff = 0;
    switch (r) {
    case Rar_Magic: nAff = rng().range(1, 2); break;
    case Rar_Rare:  nAff = rng().range(3, 4); break;
    case Rar_Legend:nAff = rng().range(5, 6); break;
    default: nAff = 0; break;
    }
    QVector<int> used;
    int nPre = 0, nSuf = 0;
    for (int k = 0; k < nAff; ++k) {
        bool wantPrefix = (nPre <= nSuf) ? rng().chance(0.62f) : rng().chance(0.38f);
        if (nPre >= 3) wantPrefix = false;
        if (nSuf >= 3) wantPrefix = true;
        QVector<int> pool2 = Data::affixesFor(slot, ilvl, wantPrefix);
        QVector<int> cand, cw;
        for (int i = 0; i < pool2.size(); ++i) {
            if (used.contains(pool2.at(i))) continue;
            const AffixDef &a = *Data::affix(pool2.at(i));
            int w = a.weight;
            // чем выше ilvl - тем ценнее поздние аффиксы
            w += (ilvl - a.minIlvl) * 2;
            cand << pool2.at(i);
            cw << qMax(4, w);
        }
        if (cand.isEmpty()) continue;
        int pick = cand.at(rng().weighted(cw));
        used << pick;
        const AffixDef &a = *Data::affix(pick);
        float roll = rng().frange(a.rollMin, a.rollMax) * (1.f + 0.035f * float(qMax(0, ilvl - a.minIlvl)));
        it.affixes.append(pick);
        it.rolls.append(roll);
        if (a.prefix) ++nPre; else ++nSuf;
    }

    return it;
}

// -------------------------------------------------------------
//  ПАУТИНА НАВЫКОВ
// -------------------------------------------------------------
int Game::nodeRank(int id) const { return m_nodeRank.value(id, 0); }
bool Game::nodeUnlocked(int id) const { return nodeRank(id) > 0; }

int Game::spentPoints() const
{
    int t = 0;
    const QVector<SkillNode> &N = Data::nodes();
    QHash<int, int>::const_iterator it = m_nodeRank.constBegin();
    for (; it != m_nodeRank.constEnd(); ++it) {
        if (it.key() >= 0 && it.key() < N.size()) t += N[it.key()].cost * it.value();
    }
    return t;
}

bool Game::hasKeystone(const QString &id) const
{
    const QVector<SkillNode> &N = Data::nodes();
    for (int i = 0; i < N.size(); ++i)
        if (N[i].kind == Nk_Keystone && N[i].keystone == id && nodeRank(i) > 0) return true;
    return false;
}

bool Game::canTakeNode(int id, QString *reason) const
{
    const SkillNode *n = Data::node(id);
    if (!n) { if (reason) *reason = QStringLiteral("Узел не найден"); return false; }
    int rank = nodeRank(id);
    if (rank >= n->maxRank) { if (reason) *reason = QStringLiteral("Узел уже изучен полностью"); return false; }
    if (n->kind == Nk_Origin) { if (reason) *reason = QStringLiteral("Исток уже активен"); return false; }
    if (m_points < n->cost) { if (reason) *reason = QStringLiteral("Не хватает очков навыков"); return false; }
    if (m_level < n->reqLevel) { if (reason) *reason = QStringLiteral("Требуется %1 уровень").arg(n->reqLevel); return false; }
    if (m_level < Data::nodeReqLevel(n->ring)) {
        if (reason) *reason = QStringLiteral("Круг %1 открывается с %2 уровня").arg(n->ring).arg(Data::nodeReqLevel(n->ring));
        return false;
    }
    // нужен взятый сосед
    bool ok = false;
    for (int i = 0; i < n->links.size(); ++i)
        if (nodeUnlocked(n->links.at(i))) { ok = true; break; }
    if (!ok) { if (reason) *reason = QStringLiteral("Нужно сначала изучить соседний узел"); return false; }
    return true;
}

bool Game::takeNode(int id)
{
    QString why;
    if (!canTakeNode(id, &why)) { if (!why.isEmpty()) log(why, C::bad); return false; }
    const SkillNode *n = Data::node(id);
    m_points -= n->cost;
    m_nodeRank.insert(id, nodeRank(id) + 1);
    AchTracker::inst().setCounterMax(Ach::C_Skills, learnedSkills().size());
    {
        int ks = 0;
        const QVector<SkillNode> &N = Data::nodes();
        for (int i = 0; i < N.size(); ++i)
            if (N[i].kind == Nk_Keystone && nodeRank(i) > 0) ++ks;
        AchTracker::inst().setCounterMax(Ach::C_Keystones, ks);
    }
    recalc();
    emit webChanged();
    emit statsChanged();
    if (n->kind == Nk_Active) {
        log(QStringLiteral("Изучено умение: %1").arg(n->name), C::amber);
        // авто-привязка на свободный слот панели
        for (int i = 0; i < 6; ++i) {
            if (m_hotbar[i] == n->activeSkill) break;
            if (m_hotbar[i] < 0) { m_hotbar[i] = n->activeSkill; break; }
        }
    } else if (n->kind == Nk_Keystone) {
        log(QStringLiteral("Ключевой камень активирован: %1!").arg(n->name), C::arcane);
    } else {
        log(QStringLiteral("%1 (%2/%3)").arg(n->name).arg(nodeRank(id)).arg(n->maxRank), C::boneDim);
    }
    return true;
}

bool Game::canRefundNode(int id, QString *reason) const
{
    const SkillNode *n = Data::node(id);
    if (!n || nodeRank(id) <= 0) { if (reason) *reason = QStringLiteral("Узел не изучен"); return false; }
    if (n->kind == Nk_Origin) { if (reason) *reason = QStringLiteral("Исток нельзя забыть"); return false; }
    // нельзя снимать, если от него зависят взятые узлы, у которых не останется связи
    const QVector<SkillNode> &N = Data::nodes();
    for (int i = 0; i < N.size(); ++i) {
        if (i == id || nodeRank(i) <= 0) continue;
        bool depends = N[i].links.contains(id);
        if (!depends) continue;
        bool other = false;
        for (int k = 0; k < N[i].links.size(); ++k) {
            int l = N[i].links.at(k);
            if (l != id && nodeUnlocked(l)) { other = true; break; }
        }
        if (!other) { if (reason) *reason = QStringLiteral("Сначала забудьте узлы, висящие на этой нити"); return false; }
    }
    return true;
}

bool Game::refundNode(int id)
{
    QString why;
    if (!canRefundNode(id, &why)) { if (!why.isEmpty()) log(why, C::bad); return false; }
    const SkillNode *n = Data::node(id);
    int r = nodeRank(id);
    if (r <= 1) m_nodeRank.remove(id);
    else m_nodeRank.insert(id, r - 1);
    m_points += n->cost;
    if (n->kind == Nk_Active) {
        for (int i = 0; i < 6; ++i) if (m_hotbar[i] == n->activeSkill) m_hotbar[i] = -1;
    }
    recalc();
    emit webChanged();
    emit statsChanged();
    return true;
}

int Game::respecCost() const { return 60 + m_level * 22 + spentPoints() * 4; }

bool Game::respecAll()
{
    AchTracker::inst().bump(Ach::C_Respecs);
    int refund = spentPoints();
    m_nodeRank.clear();
    m_nodeRank.insert(0, 1);
    m_points += refund - 0; // «Исток» не стоил очков
    for (int i = 0; i < 6; ++i) m_hotbar[i] = -1;
    recalc();
    emit webChanged();
    emit statsChanged();
    return true;
}

QList<int> Game::learnedSkills() const
{
    QList<int> out;
    const QVector<SkillNode> &N = Data::nodes();
    for (int i = 0; i < N.size(); ++i) {
        if (N[i].kind == Nk_Active && nodeRank(i) > 0 && N[i].activeSkill >= 0)
            out.append(N[i].activeSkill);
    }
    return out;
}

int Game::skillRank(int skillId) const
{
    const QVector<SkillNode> &N = Data::nodes();
    for (int i = 0; i < N.size(); ++i)
        if (N[i].activeSkill == skillId && nodeRank(i) > 0) return qMax(1, nodeRank(i));
    return 0;
}

int Game::hotbar(int i) const { return (i >= 0 && i < 6) ? m_hotbar[i] : -1; }

bool Game::setHotbar(int i, int skillId)
{
    if (i < 0 || i >= 6) return false;
    if (skillId >= 0) {
        bool known = false;
        QList<int> L = learnedSkills();
        for (int k = 0; k < L.size(); ++k) if (L.at(k) == skillId) { known = true; break; }
        if (!known) return false;
        for (int k = 0; k < 6; ++k) if (m_hotbar[k] == skillId) m_hotbar[k] = -1;
    }
    m_hotbar[i] = skillId;
    emit statsChanged();
    return true;
}

float Game::skillCost(int skillId) const
{
    const ActiveSkill *s = Data::skill(skillId);
    if (!s) return 0;
    int r = qMax(1, skillRank(skillId));
    float c = s->cost * (1.f + 0.10f * float(r - 1));
    return qMax(0.f, c);
}

float Game::skillCooldown(int skillId) const
{
    const ActiveSkill *s = Data::skill(skillId);
    if (!s) return 0;
    int r = qMax(1, skillRank(skillId));
    float cd = s->cd * (1.f - 0.06f * float(r - 1));
    return qMax(0.15f, cd * (1.f - m_d.cdReduction));
}

float Game::skillDamagePct(int skillId) const
{
    const ActiveSkill *s = Data::skill(skillId);
    if (!s) return 0;
    int r = qMax(1, skillRank(skillId));
    return s->dmgPct * (1.f + 0.22f * float(r - 1));
}

// -------------------------------------------------------------
//  ОПЫТ И УРОВНИ
// -------------------------------------------------------------
float Game::xpForLevel(int lvl)
{
    return float(58.0 * qPow(double(lvl), 1.34) + 16.0 * lvl);
}

float Game::xpProgress() const
{
    float need = xpForLevel(m_level);
    return clampf(m_xp / need, 0.f, 1.f);
}

void Game::addXp(float v)
{
    v *= (1.f + m_d.xpGain) * xpMul();
    m_xp += v;
    bool leveled = false;
    while (m_level < 40 && m_xp >= xpForLevel(m_level)) {
        m_xp -= xpForLevel(m_level);
        ++m_level;
        int pts = 2 + ((m_level % 5 == 0) ? 3 : 0);
        m_points += pts;
        leveled = true;
    }
    if (m_level >= 40) m_xp = qMin(m_xp, xpForLevel(40));
    if (leveled) {
        AchTracker::inst().setCounterMax(Ach::C_MaxLevel, m_level);
        recalc();
        m_hp = m_d.maxHp;
        m_mana = m_d.maxMana;
        emit leveledUp(m_level);
        log(QStringLiteral("Новый уровень: %1! Получено очков навыков: %2").arg(m_level).arg(m_points), C::amber);
    }
    emit statsChanged();
    emit resourcesChanged();
}

void Game::addHp(float v)
{
    m_hp = clampf(m_hp + v, 0.f, m_d.maxHp);
    emit resourcesChanged();
}
void Game::addMana(float v)
{
    m_mana = clampf(m_mana + v, 0.f, m_d.maxMana);
    emit resourcesChanged();
}
void Game::addShield(float v)
{
    m_shield = clampf(m_shield + v, 0.f, qMax(m_d.shieldMax, 1.f));
    emit resourcesChanged();
}
void Game::drainShield(float &dmg)
{
    if (m_shield <= 0.f || dmg <= 0.f) return;
    float a = qMin(m_shield, dmg);
    m_shield -= a;
    dmg -= a;
    emit resourcesChanged();
}
void Game::addGold(int v)
{
    if (v > 0) v = int(v * goldMul() + 0.5f);
    m_gold = qMax(0, m_gold + v);
    if (v > 0) AchTracker::inst().bump(Ach::C_GoldEarned, v);
    emit goldChanged(m_gold);
}

// -------------------------------------------------------------
//  СЛОЖНОСТЬ: «Тлеющий уголёк / Пепельный путь / Янтарный жар / Горнило Пустоты»
// -------------------------------------------------------------
QString Game::diffName(int d)
{
    switch (qBound(0, d, 3)) {
    case D_Easy:   return QStringLiteral("Тлеющий уголёк");
    case D_Hard:   return QStringLiteral("Янтарный жар");
    case D_Doom:   return QStringLiteral("Горнило Пустоты");
    default:       return QStringLiteral("Пепельный путь");
    }
}

QString Game::diffDesc(int d)
{
    switch (qBound(0, d, 3)) {
    case D_Easy:   return QStringLiteral("Враги слабее, урона меньше, опыт и золото щедрее. Для тех, кто только ступил в пепел.");
    case D_Hard:   return QStringLiteral("Враги злее и больнее бьют, но добыча и опыт щедрые. Испытание для бывалых.");
    case D_Doom:   return QStringLiteral("Максимальная жестокость и одна жизнь: гибель навсегда стирает сохранение.");
    default:       return QStringLiteral("Честный баланс задумки. Так автор и планировал.");
    }
}

float Game::dmgTakenMul() const
{
    switch (m_diff) {
    case D_Easy: return 0.72f;
    case D_Hard: return 1.40f;
    case D_Doom: return 1.65f;
    default:     return 1.00f;
    }
}
float Game::dmgDealtMul() const
{
    switch (m_diff) {
    case D_Easy: return 1.15f;
    case D_Hard: return 0.92f;
    case D_Doom: return 0.88f;
    default:     return 1.00f;
    }
}
float Game::xpMul() const
{
    switch (m_diff) {
    case D_Easy: return 1.25f;
    case D_Hard: return 1.15f;
    case D_Doom: return 1.15f;
    default:     return 1.00f;
    }
}
float Game::goldMul() const
{
    switch (m_diff) {
    case D_Easy: return 1.20f;
    case D_Hard: return 1.10f;
    case D_Doom: return 1.10f;
    default:     return 1.00f;
    }
}
void Game::registerKill() { ++m_kills; }

void Game::tick(float dt)
{
    m_playTime += dt;
    m_playSec += dt;
    if (m_playSec >= 1.f) {
        AchTracker::inst().bump(Ach::C_Playtime, qint64(m_playSec));
        m_playSec = 0.f;
    }
    if (m_shrineTime > 0.f) {
        m_shrineTime -= dt;
        if (m_shrineTime <= 0.f) {
            m_shrineTime = 0.f;
            m_shrine = Shrine_None;
            log(QStringLiteral("Благословение святилища угасло."), C::boneDim);
            recalc();
            emit statsChanged();
        }
    }
    bool changed = false;
    if (m_hp <= 0.f) return;   // мёртвым реген не положен
    if (m_hp < m_d.maxHp && m_d.hpRegen > 0.f) { m_hp = qMin(m_d.maxHp, m_hp + m_d.hpRegen * dt); changed = true; }
    if (m_mana < m_d.maxMana && m_d.manaRegen > 0.f) { m_mana = qMin(m_d.maxMana, m_mana + m_d.manaRegen * dt); changed = true; }
    if (changed) emit resourcesChanged();
}

void Game::applyShrine(ShrineBuff b, float seconds)
{
    AchTracker::inst().bump(Ach::C_Shrines);
    m_shrine = b;
    m_shrineTime = seconds;
    recalc();
    emit statsChanged();
    emit resourcesChanged();
    QString nm;
    switch (b) {
    case Shrine_Might:   nm = QStringLiteral("Благословение Ярости (+30% урона)"); break;
    case Shrine_Ward:    nm = QStringLiteral("Благословение Оберега (+броня и барьер)"); break;
    case Shrine_Fortune: nm = QStringLiteral("Благословение Удачи (+золото и редкости)"); break;
    case Shrine_Swift:   nm = QStringLiteral("Благословение Ветра (+скорость, -откат)"); break;
    case Shrine_Arcane:  nm = QStringLiteral("Благословение Тайн (+мана, +скорость заклинаний)"); break;
    default: nm = QStringLiteral("Благословение"); break;
    }
    log(nm + QStringLiteral(" на %1 сек.").arg(int(seconds)), C::arcane);
}

void Game::onPlayerDeath()
{
    ++m_deaths;
    AchTracker::inst().bump(Ach::C_Deaths);
    int lost = m_gold / 10;
    m_gold -= lost;
    // пепел забирает пару предметов из сумки (надетое остаётся)
    QVector<int> valid;
    for (int i = 0; i < m_bag.size(); ++i) if (m_bag[i].valid) valid.append(i);
    int take = qMin(2, valid.size());
    for (int k = 0; k < take; ++k) {
        if (valid.isEmpty()) break;
        int pick = valid.at(rng().pick(valid.size()));
        valid.removeAll(pick);
        QString nm = Data::itemNameFull(m_bag[pick]);
        log(QStringLiteral("Пепел забрал: %1").arg(nm), C::bad);
        m_bag[pick] = Item();
    }
    if (take > 0) emit inventoryChanged();
    emit goldChanged(m_gold);
    m_shrine = Shrine_None;
    m_shrineTime = 0;
    recalc();
    m_hp = m_d.maxHp * 0.5f;
    m_mana = m_d.maxMana * 0.5f;
    m_shield = 0;
    emit playerDied();
    emit resourcesChanged();
    log(QStringLiteral("Ты пал. Пепел забрал %1 золота. Возрождение в последнем поселении.").arg(lost), C::bad);
}

// -------------------------------------------------------------
//  МИР
// -------------------------------------------------------------
bool Game::isUnlocked(int locId) const { return m_unlocked.contains(locId); }
bool Game::isCleared(int locId) const { return m_cleared.contains(locId); }

int Game::locationTier(int locId) const
{
    const LocationDef *L = Data::loc(locId);
    return L ? L->tier : 1;
}

void Game::unlockNeighbors(int locId)
{
    const LocationDef *L = Data::loc(locId);
    if (!L) return;
    for (int i = 0; i < L->links.size(); ++i) m_unlocked.insert(L->links.at(i));
    m_unlocked.insert(locId);
}

bool Game::travelTo(int locId)
{
    const LocationDef *L = Data::loc(locId);
    if (!L) return false;
    if (!m_unlocked.contains(locId)) {
        log(QStringLiteral("Путь сюда ещё не открыт."), C::bad);
        return false;
    }
    m_curLoc = locId;
    if (L->type == Loc_Town) m_lastTown = locId;
    emit worldChanged();
    return true;
}

void Game::enterLocation(int locId) { m_curLoc = locId; emit worldChanged(); }

void Game::visitLocation(int locId)
{
    const LocationDef *L = Data::loc(locId);
    if (!L) return;
    unlockNeighbors(locId);
    if (L->type == Loc_Town) {
        m_lastTown = locId;
        static const int towns[6] = { 0, 6, 8, 15, 22, 28 };
        for (int i = 0; i < 6; ++i)
            if (towns[i] == locId) AchTracker::inst().setBit(Ach::M_Towns, i);
    }
    if (!m_cleared.contains(locId)) {
        m_cleared.insert(locId);
        m_clears[locId] = m_clears.value(locId, 0) + 1;
    }
    emit worldChanged();
}

void Game::completeLocation(int locId, bool bossKilled)
{
    const LocationDef *L = Data::loc(locId);
    if (!L) return;
    bool first = !m_cleared.contains(locId);
    m_cleared.insert(locId);
    m_clears[locId] = m_clears.value(locId, 0) + 1;
    unlockNeighbors(locId);
    if (L->type == Loc_Town) m_lastTown = locId;
    emit worldChanged();
    emit locationFinished(locId);
    if (first) {
        int xpBonus = L->tier * 60 + ((L->bossId >= 0) ? L->tier * 150 : 0);
        addXp(float(xpBonus));
        log(QStringLiteral("Локация пройдена: %1 (+%2 опыта)").arg(L->name).arg(xpBonus), C::good);
        if (bossKilled) log(QStringLiteral("Босс повержен! Открыты новые пути."), C::amber);
    }
}

// -------------------------------------------------------------
//  ЛОГ
// -------------------------------------------------------------
void Game::log(const QString &msg, const QColor &c)
{
    emit logMessage(msg, c.isValid() ? c : C::bone);
}

// -------------------------------------------------------------
//  СОХРАНЕНИЕ
// -------------------------------------------------------------
QJsonObject Game::itemToJson(const Item &it)
{
    QJsonObject o;
    if (!it.valid) return o;
    o[QStringLiteral("valid")] = true;
    o[QStringLiteral("b")] = it.baseId;
    o[QStringLiteral("u")] = it.uniqueId;
    o[QStringLiteral("r")] = int(it.rarity);
    o[QStringLiteral("il")] = it.ilvl;
    o[QStringLiteral("c")] = it.count;
    o[QStringLiteral("sd")] = it.seed;
    QJsonArray a, ro, g;
    for (int i = 0; i < it.affixes.size(); ++i) a.append(it.affixes.at(i));
    for (int i = 0; i < it.rolls.size(); ++i) ro.append(double(it.rolls.at(i)));
    for (int i = 0; i < it.gems.size(); ++i) g.append(it.gems.at(i));
    o[QStringLiteral("af")] = a;
    o[QStringLiteral("rl")] = ro;
    o[QStringLiteral("gm")] = g;
    return o;
}

Item Game::itemFromJson(const QJsonObject &o)
{
    Item it;
    if (!o.value(QStringLiteral("valid")).toBool(false)) return it;
    it.valid = true;
    it.baseId = o.value(QStringLiteral("b")).toInt(-1);
    // самоцветы убраны из игры: старые сейвы чистятся при загрузке
    Data::ensureLoaded();
    const ItemBaseDef *bb = Data::base(it.baseId);
    if (bb && bb->bag == Bag_Gem) return Item();
    it.uniqueId = o.value(QStringLiteral("u")).toInt(-1);
    it.rarity = Rarity(o.value(QStringLiteral("r")).toInt(0));
    it.ilvl = o.value(QStringLiteral("il")).toInt(1);
    it.count = qMax(1, o.value(QStringLiteral("c")).toInt(1));
    it.seed = o.value(QStringLiteral("sd")).toInt(1);
    QJsonArray a = o.value(QStringLiteral("af")).toArray();
    for (int i = 0; i < a.size(); ++i) it.affixes.append(a.at(i).toInt());
    QJsonArray r = o.value(QStringLiteral("rl")).toArray();
    for (int i = 0; i < r.size(); ++i) it.rolls.append(float(r.at(i).toDouble()));
    // поле "gm" (гнёзда) больше не читается - камни из вещей выброшены
    return it;
}

QJsonObject itemToJson(const Item &it) { return Game::itemToJson(it); }
Item itemFromJson(const QJsonObject &o) { return Game::itemFromJson(o); }

QJsonObject Game::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("ver")] = 3;
    o[QStringLiteral("hero")] = m_hero;
    o[QStringLiteral("cls")] = m_class;
    o[QStringLiteral("diff")] = m_diff;
    o[QStringLiteral("lvl")] = m_level;
    o[QStringLiteral("xp")] = double(m_xp);
    o[QStringLiteral("hp")] = double(m_hp);
    o[QStringLiteral("mp")] = double(m_mana);
    o[QStringLiteral("sh")] = double(m_shield);
    o[QStringLiteral("gold")] = m_gold;
    o[QStringLiteral("deaths")] = m_deaths;
    o[QStringLiteral("kills")] = m_kills;
    o[QStringLiteral("time")] = double(m_playTime);
    o[QStringLiteral("points")] = m_points;
    o[QStringLiteral("rng")] = QString::number(m_rngState);
    o[QStringLiteral("cur")] = m_curLoc;
    o[QStringLiteral("town")] = m_lastTown;
    o[QStringLiteral("shrine")] = int(m_shrine);
    o[QStringLiteral("shrineT")] = double(m_shrineTime);

    QJsonArray bag;
    for (int i = 0; i < m_bag.size(); ++i) bag.append(itemToJson(m_bag[i]));
    o[QStringLiteral("bag")] = bag;

    QJsonArray eq;
    for (int i = 0; i < m_equip.size(); ++i) eq.append(itemToJson(m_equip[i]));
    o[QStringLiteral("equip")] = eq;

    QJsonArray nodes;
    QHash<int, int>::const_iterator it = m_nodeRank.constBegin();
    for (; it != m_nodeRank.constEnd(); ++it) {
        QJsonObject n;
        n[QStringLiteral("id")] = it.key();
        n[QStringLiteral("rank")] = it.value();
        nodes.append(n);
    }
    o[QStringLiteral("web")] = nodes;

    QJsonArray hb;
    for (int i = 0; i < 6; ++i) hb.append(m_hotbar[i]);
    o[QStringLiteral("hotbar")] = hb;

    QJsonArray un, cl;
    for (QSet<int>::const_iterator i = m_unlocked.constBegin(); i != m_unlocked.constEnd(); ++i) un.append(*i);
    for (QSet<int>::const_iterator i = m_cleared.constBegin(); i != m_cleared.constEnd(); ++i) cl.append(*i);
    o[QStringLiteral("unlocked")] = un;
    o[QStringLiteral("cleared")] = cl;
    QJsonObject clears;
    for (QHash<int, int>::const_iterator i = m_clears.constBegin(); i != m_clears.constEnd(); ++i)
        clears[QString::number(i.key())] = i.value();
    o[QStringLiteral("clears")] = clears;
    return o;
}

bool Game::fromJson(const QJsonObject &o)
{
    if (!o.contains(QStringLiteral("hero"))) return false;
    m_hero = o.value(QStringLiteral("hero")).toString();
    m_class = o.value(QStringLiteral("cls")).toInt(0);
    m_diff = qBound(0, o.value(QStringLiteral("diff")).toInt(1), 3);
    m_level = qMax(1, o.value(QStringLiteral("lvl")).toInt(1));
    m_xp = float(o.value(QStringLiteral("xp")).toDouble());
    m_gold = o.value(QStringLiteral("gold")).toInt();
    m_deaths = o.value(QStringLiteral("deaths")).toInt();
    m_kills = o.value(QStringLiteral("kills")).toInt();
    m_playTime = float(o.value(QStringLiteral("time")).toDouble());
    m_points = o.value(QStringLiteral("points")).toInt();
    m_curLoc = o.value(QStringLiteral("cur")).toInt(0);
    m_lastTown = o.value(QStringLiteral("town")).toInt(0);
    m_shrine = ShrineBuff(o.value(QStringLiteral("shrine")).toInt(0));
    m_shrineTime = float(o.value(QStringLiteral("shrineT")).toDouble());
    bool ok = false;
    quint64 rs = o.value(QStringLiteral("rng")).toString().toULongLong(&ok);
    if (ok && rs) { m_rngState = rs; rng().setState(rs); }

    m_bag = QVector<Item>(BAG_SIZE);
    QJsonArray bag = o.value(QStringLiteral("bag")).toArray();
    for (int i = 0; i < bag.size() && i < m_bag.size(); ++i)
        m_bag[i] = itemFromJson(bag.at(i).toObject());

    m_equip = QVector<Item>(Slot_EquipCount);
    QJsonArray eq = o.value(QStringLiteral("equip")).toArray();
    for (int i = 0; i < eq.size() && i < m_equip.size(); ++i)
        m_equip[i] = itemFromJson(eq.at(i).toObject());

    m_nodeRank.clear();
    QJsonArray nodes = o.value(QStringLiteral("web")).toArray();
    for (int i = 0; i < nodes.size(); ++i) {
        QJsonObject n = nodes.at(i).toObject();
        int id = n.value(QStringLiteral("id")).toInt(-1);
        int rk = n.value(QStringLiteral("rank")).toInt(0);
        if (id >= 0 && rk > 0) m_nodeRank.insert(id, rk);
    }
    QJsonArray hb = o.value(QStringLiteral("hotbar")).toArray();
    for (int i = 0; i < 6; ++i) m_hotbar[i] = i < hb.size() ? hb.at(i).toInt(-1) : -1;

    m_unlocked.clear();
    QJsonArray un = o.value(QStringLiteral("unlocked")).toArray();
    for (int i = 0; i < un.size(); ++i) m_unlocked.insert(un.at(i).toInt());
    m_unlocked.insert(m_curLoc);
    m_unlocked.insert(m_lastTown);
    m_unlocked.insert(Data::firstLocation());
    m_cleared.clear();
    QJsonArray cl = o.value(QStringLiteral("cleared")).toArray();
    for (int i = 0; i < cl.size(); ++i) m_cleared.insert(cl.at(i).toInt());
    m_clears.clear();
    QJsonObject cs = o.value(QStringLiteral("clears")).toObject();
    for (QJsonObject::const_iterator i = cs.constBegin(); i != cs.constEnd(); ++i)
        m_clears.insert(i.key().toInt(), i.value().toInt());

    m_version = o.value(QStringLiteral("ver")).toInt(1);
    recalc();
    m_hp = qBound(1.f, float(o.value(QStringLiteral("hp")).toDouble(m_d.maxHp)), m_d.maxHp);
    m_mana = qBound(0.f, float(o.value(QStringLiteral("mp")).toDouble(m_d.maxMana)), m_d.maxMana);
    m_shield = qBound(0.f, float(o.value(QStringLiteral("sh")).toDouble(0)), qMax(1.f, m_d.shieldMax));

    emit statsChanged();
    emit inventoryChanged();
    emit webChanged();
    emit worldChanged();
    emit goldChanged(m_gold);
    emit resourcesChanged();
    return true;
}

bool Game::saveGame(const QString &path)
{
    QString p = path.isEmpty() ? defaultSavePath() : path;
    QDir().mkpath(QFileInfo(p).absolutePath());
    m_rngState = rng().state();
    QJsonDocument doc(toJson());
    QSaveFile f(p);
    if (!f.open(QIODevice::WriteOnly)) { log(QStringLiteral("Не удалось сохранить игру."), C::bad); return false; }
    f.write(doc.toJson(QJsonDocument::Indented));
    if (!f.commit()) { log(QStringLiteral("Ошибка записи сейва."), C::bad); return false; }
    log(QStringLiteral("Игра сохранена."), C::good);
    return true;
}

bool Game::loadGame(const QString &path)
{
    QString p = path.isEmpty() ? defaultSavePath() : path;
    QFile f(p);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return false;
    if (!fromJson(doc.object())) return false;
    log(QStringLiteral("Игра загружена. С возвращением, %1.").arg(m_hero), C::good);
    return true;
}

} // namespace aa
