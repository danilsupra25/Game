#include "Core/Ach.h"
#include "Core/Style.h"
#include "Core/Data.h"

#include <QSettings>
#include <QDataStream>
#include <QByteArray>

namespace aa {
namespace Ach {

static const Def g_defs[A_Count] = {
    { A_FirstBlood,   "Первая кровь",            "Убей первого врага.",                              Gl_Swords,   1,        C_Kills },
    { A_Killer100,    "Работа мясника",          "Убей 100 врагов.",                                 Gl_Skull,    100,      C_Kills },
    { A_Killer500,    "Пепельный жнец",          "Убей 500 врагов.",                                 Gl_Skull,    500,      C_Kills },
    { A_Fire50,       "Пиромаг",                 "Убей 50 врагов огнём.",                            Gl_Fireball, 50,       C_KillsFire },
    { A_Elite10,      "Охотник на элиту",        "Убей 10 элитных врагов.",                          Gl_Elite,    10,       C_EliteKills },
    { A_BossFirst,    "Убийца богов",            "Повержни первого босса.",                          Gl_Boss,     1,        C_BossKills },
    { A_BossAll,      "Пять корон",              "Повержни всех пять боссов регионов.",              Gl_Crown2,   5,        -1 },
    { A_Final,        "Конец света отменяется",  "Повержни Пожирателя Звёзд.",                       Gl_Star,     1,        C_Final },
    { A_FinalNoDeath, "Безупречность",           "Пройди игру до финала без единой смерти.",         Gl_Heart,    1,        C_FinalNoDeath },
    { A_Level10,      "Новичок пепла",           "Достигни 10 уровня.",                              Gl_LevelUp,  10,       C_MaxLevel },
    { A_Level25,      "Закалённый",              "Достигни 25 уровня.",                              Gl_LevelUp,  25,       C_MaxLevel },
    { A_Level40,      "Легенда пустошей",        "Достигни 40 уровня.",                              Gl_LevelUp,  40,       C_MaxLevel },
    { A_LocFirst,     "Первые шаги",             "Зачисти первую локацию.",                          Gl_Map,      1,        C_LocsCleared },
    { A_LocAll,       "Картограф",               "Зачисти все локации мира.",                        Gl_Map,      33,       C_LocsCleared },
    { A_TownsAll,     "Почтальон пустошей",      "Побывай во всех шести поселениях.",                Gl_Town,     6,        -1 },
    { A_EliteLoc3,    "Зачистка логов",          "Зачисти три элитных логова.",                      Gl_Elite,    3,        C_EliteLocs },
    { A_Gold10k,      "Скупец",                  "Заработай 10 000 золота за игру.",                 Gl_Coins,    10000,    C_GoldEarned },
    { A_Gold100k,     "Крез пустошей",           "Заработай 100 000 золота за игру.",                Gl_Coins,    100000,   C_GoldEarned },
    { A_Rare10,       "Коллекционер",            "Найди 10 редких и лучше предметов.",               Gl_Bag,      10,       C_Rare },
    { A_Legend5,      "Легендарщик",             "Найди 5 легендарных предметов.",                   Gl_Star,     5,        C_Legend },
    { A_Unique1,      "Уникальная находка",      "Найди уникальный предмет.",                        Gl_Crown2,   1,        C_Uniques },
    { A_UniqueAll,    "Музей пепла",             "Найди все 12 уникальных предметов.",               Gl_Crown2,   12,       -1 },
    { A_Gems5,        "Мародёр",                 "Подними 250 предметов с земли.",                    Gl_Bag,      250,      C_Pickups },
    { A_Potions25,    "Завсегдатай лавки",       "Выпей 25 зелий.",                                  Gl_PotionHp, 25,       C_Potions },
    { A_Rolls50,      "Танцующий с клинками",    "Сделай 50 кувырков.",                              Gl_Dash,     50,       C_Rolls },
    { A_Dodge25,      "Не поймать",              "Уклонись от 25 ударов.",                           Gl_Haste,    25,       C_Dodges },
    { A_Crit100,      "Снайфер пустошей",        "Нанеси 100 критических ударов.",                   Gl_Mark,     100,      C_Crits },
    { A_BigHit,       "Один удар",               "Нанеси 1500 урона одним попаданием.",              Gl_Hammer,   1500,     C_MaxHit },
    { A_MultiKill,    "Тройной выброс",          "Убей троих врагов за 2,5 секунды.",                Gl_Nova,     1,        C_MultiKill },
    { A_LocNoDmg,     "Призрак",                 "Зачисти локацию, не получив урона.",               Gl_Ward,     1,        C_LocNoDmg },
    { A_LocFast,      "Спешка",                  "Зачисти локацию быстрее 60 секунд.",               Gl_Haste,    1,        C_LocFast },
    { A_LocLowHp,     "На волоске",              "Зачисти локацию с здоровьем ниже 10%.",            Gl_Heart,    1,        C_LocLowHp },
    { A_Keystones8,   "Восемь корон паутины",    "Изучи все ключевые камни за одного героя.",        Gl_Web,      8,        C_Keystones },
    { A_Skills10,     "Мастер умений",           "Изучи 10 активных умений за одного героя.",        Gl_Spin,     10,       C_Skills },
    { A_Shrine5,      "Паломник",                "Получи 5 благословений святилищ.",                 Gl_Shrine,   5,        C_Shrines },
    { A_Deaths10,     "Пепел принимает",         "Умри 10 раз. Мы не осуждаем.",                     Gl_Skull,    10,       C_Deaths },
    { A_Time5h,       "Ветеран пути",            "Проведи в пути 5 часов.",                          Gl_Torch,    18000,    C_Playtime },
};

const Def &def(int id) { return g_defs[qBound(0, id, int(A_Count) - 1)]; }
int count() { return A_Count; }

} // namespace Ach

// ============================================================
AchTracker &AchTracker::inst()
{
    static AchTracker t;
    return t;
}

AchTracker::AchTracker()
    : m_unlocked(0), m_loaded(false)
{
    m_c.resize(int(Ach::C_Count));
    m_c.fill(0);
    m_m.resize(int(Ach::M_Count));
    m_m.fill(0);
}

void AchTracker::load()
{
    if (m_loaded) return;
    m_loaded = true;
    QSettings s(QStringLiteral("AshAmber"), QStringLiteral("PepeL_i_Yantar"));
    QByteArray raw = s.value(QStringLiteral("ach")).toByteArray();
    if (raw.isEmpty()) return;
    QDataStream ds(&raw, QIODevice::ReadOnly);
    quint32 ver = 0;
    ds >> ver;
    if (ver != 1) return;
    ds >> m_unlocked;
    qint32 n = 0;
    ds >> n;
    for (qint32 i = 0; i < n && i < m_c.size(); ++i) ds >> m_c[i];
    ds >> n;
    for (qint32 i = 0; i < n && i < m_m.size(); ++i) ds >> m_m[i];
}

void AchTracker::save()
{
    QByteArray raw;
    QDataStream ds(&raw, QIODevice::WriteOnly);
    ds << quint32(1);
    ds << m_unlocked;
    ds << qint32(m_c.size());
    for (int i = 0; i < m_c.size(); ++i) ds << m_c[i];
    ds << qint32(m_m.size());
    for (int i = 0; i < m_m.size(); ++i) ds << m_m[i];
    QSettings s(QStringLiteral("AshAmber"), QStringLiteral("PepeL_i_Yantar"));
    s.setValue(QStringLiteral("ach"), raw);
}

bool AchTracker::unlocked(int id) const
{
    return id >= 0 && id < Ach::A_Count && (m_unlocked & (1ull << id));
}

int AchTracker::unlockedCount() const
{
    int n = 0;
    for (int i = 0; i < Ach::A_Count; ++i) if (unlocked(i)) ++n;
    return n;
}

qint64 AchTracker::counter(int c) const { return (c >= 0 && c < m_c.size()) ? m_c[c] : 0; }
quint64 AchTracker::mask(int m) const { return (m >= 0 && m < m_m.size()) ? m_m[m] : 0; }

void AchTracker::bump(int c, qint64 add)
{
    if (c < 0 || c >= m_c.size()) return;
    m_c[c] += add;
    check();
}

void AchTracker::setCounterMax(int c, qint64 v)
{
    if (c < 0 || c >= m_c.size()) return;
    if (v > m_c[c]) { m_c[c] = v; check(); }
}

void AchTracker::setBit(int maskIdx, int bit)
{
    if (maskIdx < 0 || maskIdx >= m_m.size() || bit < 0 || bit > 63) return;
    quint64 b = 1ull << bit;
    if (m_m[maskIdx] & b) return;
    m_m[maskIdx] |= b;
    check();
}

void AchTracker::locCleared(bool noDamage, float seconds, float hpFraction, bool eliteLoc)
{
    if (noDamage) m_c[Ach::C_LocNoDmg] = 1;
    if (seconds > 0.f && seconds < 60.f) m_c[Ach::C_LocFast] = 1;
    if (hpFraction < 0.10f) m_c[Ach::C_LocLowHp] = 1;
    if (eliteLoc) m_c[Ach::C_EliteLocs] += 1;
    check();
}

int AchTracker::progressCur(int id) const
{
    const Ach::Def &d = Ach::def(id);
    switch (id) {
    case Ach::A_BossAll:  { int n = 0; for (int i = 0; i < 5; ++i) if (m_m[Ach::M_RegionBoss] & (1ull << i)) ++n; return n; }
    case Ach::A_TownsAll: { int n = 0; for (int i = 0; i < 6; ++i) if (m_m[Ach::M_Towns] & (1ull << i)) ++n; return n; }
    case Ach::A_UniqueAll:{ int n = 0; for (int i = 0; i < 12; ++i) if (m_m[Ach::M_Uniques] & (1ull << i)) ++n; return n; }
    default: break;
    }
    if (d.counter < 0) return unlocked(id) ? d.goal : 0;
    qint64 v = counter(d.counter);
    return int(qMin<qint64>(v, d.goal));
}

int AchTracker::progressGoal(int id) const { return Ach::def(id).goal; }

bool AchTracker::met(int id) const
{
    const Ach::Def &d = Ach::def(id);
    switch (id) {
    case Ach::A_BossAll:   return m_m[Ach::M_RegionBoss] == 0x1Full;
    case Ach::A_TownsAll:  return m_m[Ach::M_Towns] == 0x3Full;
    case Ach::A_UniqueAll: return m_m[Ach::M_Uniques] == 0xFFFull;
    default: break;
    }
    if (d.counter < 0) return false;
    return counter(d.counter) >= d.goal;
}

void AchTracker::check()
{
    if (!m_loaded) load();
    bool changed = false;
    for (int i = 0; i < Ach::A_Count; ++i) {
        if (unlocked(i)) continue;
        if (met(i)) {
            m_unlocked |= (1ull << i);
            changed = true;
            emit unlockedSignal(i);
        }
    }
    if (changed) save();
}

} // namespace aa
