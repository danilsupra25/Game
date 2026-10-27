#ifndef AA_ACH_H
#define AA_ACH_H

// Достижения: глобальный прогресс профиля (хранится в QSettings, не в сейве персонажа).

#include <QObject>
#include <QString>
#include <QVector>
#include <QColor>
#include <QtGlobal>

namespace aa {

namespace Ach {

enum Id {
    A_FirstBlood = 0, A_Killer100, A_Killer500, A_Fire50, A_Elite10,
    A_BossFirst, A_BossAll, A_Final, A_FinalNoDeath,
    A_Level10, A_Level25, A_Level40,
    A_LocFirst, A_LocAll, A_TownsAll, A_EliteLoc3,
    A_Gold10k, A_Gold100k,
    A_Rare10, A_Legend5, A_Unique1, A_UniqueAll,
    A_Gems5, A_Potions25, A_Rolls50, A_Dodge25, A_Crit100,
    A_BigHit, A_MultiKill,
    A_LocNoDmg, A_LocFast, A_LocLowHp,
    A_Keystones8, A_Skills10, A_Shrine5, A_Deaths10, A_Time5h,
    A_Count
};

enum Counter {
    C_Kills = 0, C_KillsFire, C_EliteKills, C_BossKills, C_Final, C_FinalNoDeath,
    C_Deaths, C_MaxLevel, C_LocsCleared, C_EliteLocs,
    C_GoldEarned, C_Rare, C_Legend, C_Uniques,
    C_Gems, C_Potions, C_Rolls, C_Dodges, C_Crits,
    C_MaxHit, C_MultiKill, C_LocNoDmg, C_LocFast, C_LocLowHp,
    C_Skills, C_Keystones, C_Shrines, C_Respecs,
    C_Playtime, C_DmgDealt,
    C_Pickups,          // добавлен в конец: старые профили совместимы
    C_Count
};

enum Mask { M_Uniques = 0, M_Towns, M_RegionBoss, M_Count };

struct Def
{
    int id;
    const char *name;
    const char *desc;
    int glyph;
    int goal;        // целевое значение для полосы прогресса
    int counter;     // какой счётчик показывать (-1 для масок/флагов)
};

const Def &def(int id);
int count();

} // namespace Ach

class AchTracker : public QObject
{
    Q_OBJECT
public:
    static AchTracker &inst();

    void load();
    void save();

    bool unlocked(int id) const;
    int unlockedCount() const;
    qint64 counter(int c) const;
    quint64 mask(int m) const;

    void bump(int c, qint64 add = 1);
    void setCounterMax(int c, qint64 v);
    void setBit(int maskIdx, int bit);

    // событие зачистки локации (для разовых достижений)
    void locCleared(bool noDamage, float seconds, float hpFraction, bool eliteLoc);

    void check();          // пересчитать все достижения
    int progressCur(int id) const;
    int progressGoal(int id) const;

signals:
    void unlockedSignal(int id);

private:
    AchTracker();
    bool met(int id) const;

    QVector<qint64> m_c;
    QVector<quint64> m_m;
    quint64 m_unlocked;
    bool m_loaded;
};

} // namespace aa

#endif // AA_ACH_H
