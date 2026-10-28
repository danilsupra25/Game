#include "Game/GameView.h"
#include "Game/Sprites.h"
#include "Core/Data.h"
#include "Core/Style.h"
#include "Core/Rng.h"
#include "Core/Audio.h"
#include "Core/Binds.h"
#include "Game/GlFx.h"
#include "Game/SpriteBank.h"
#include "Core/Ach.h"

#include <QtMath>
#include <QPainter>
#include <QPainterPath>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QApplication>
#include <QRadialGradient>
#include <QFontMetrics>
#include <algorithm>

namespace aa {

// ============================================================
//  МЕЛКИЕ ХЕЛПЕРЫ
// ============================================================
static inline float FX(const QPointF &p) { return float(p.x()); }
static inline float FY(const QPointF &p) { return float(p.y()); }
static inline float distf(const QPointF &a, const QPointF &b)
{
    float dx = float(a.x() - b.x()), dy = float(a.y() - b.y());
    return qSqrt(dx * dx + dy * dy);
}

// ---- наборы реплик NPC (выдаются случайно, без повтора подряд) ----
static const QStringList &npcLines(int kind)
{
    static const QStringList kMerchant = {
        QStringLiteral("Есть товар - найдётся и золото."),
        QStringLiteral("Ты пахнешь пеплом. Это мой лучший аромат - значит, клиент живой."),
        QStringLiteral("Не спрашивай, откуда хлам. Спрашивай, почём."),
        QStringLiteral("После набега цены кусаются... но я всегда иду навстречу."),
        QStringLiteral("Монеты в кармане жгут? Отдай мне - станет легче."),
        QStringLiteral("Лучшая сделка та, о которой не знает кузнец."),
        QStringLiteral("Смотрю, ты снова не умер. Респект. Кошелёк тоже цел?"),
        QStringLiteral("Бери сейчас: завтра это утащат крысы. Большие и злые."),
    };
    static const QStringList kSmith = {
        QStringLiteral("Могу выжечь из тебя старые ошибки."),
        QStringLiteral("Молот не спорит. Молот объясняет."),
        QStringLiteral("Держи крепче. Если отскочит - не моя вина, а твоя удача."),
        QStringLiteral("Сталь помнит каждый удар. Пусть помнит правильно."),
        QStringLiteral("Принесёшь рваный клинок - уйдёт целым. Почти."),
        QStringLiteral("Уголь в горне, искры в глазах - вот и весь секрет ремесла."),
        QStringLiteral("Хочешь острое? Будет острое. Хочешь злое? Принеси железа побольше."),
        QStringLiteral("Я не чинлю характер. Только оружие. Характер - за отдельную плату."),
    };
    static const QStringList kHealer = {
        QStringLiteral("Дай руку. Дыши ровно."),
        QStringLiteral("Не дёргайся. Я и сама не знаю, что будет дальше."),
        QStringLiteral("Раны заживают. Привычка лезть под когти - нет."),
        QStringLiteral("Глоток отвара - и ты почти как новый. Ну, почти."),
        QStringLiteral("Скажи «аа»... Ладно, не надо. Просто стой смирно."),
        QStringLiteral("Я лечу всех. Даже тех, кто этого не заслуживает. Особенно их."),
        QStringLiteral("Кровь - глупая жидкость. Стоит ей кончиться, и ты становишься умнее."),
        QStringLiteral("В следующий раз принеси щит, а не оправдания."),
    };
    static const QStringList kElder = {
        QStringLiteral("Пепел помнит всех, кто прошёл. Присядь у огня."),
        QStringLiteral("Когда-то этот город горел ярко. Теперь он тлеет - как и его жители."),
        QStringLiteral("Янтарь - это застывший свет. Свет, который отказался умирать."),
        QStringLiteral("Не бойся мертвых. Бойся живых, которым нечего терять."),
        QStringLiteral("Жрецы Морпри говорят, что смерть - лишь дверь. Они правы. Но забывают про то, что по ту сторону."),
        QStringLiteral("Каждый уголёк в этом костре - чья-то история. Слушай внимательнее."),
        QStringLiteral("Ты пришёл сюда за силой. Сила придёт за тобой."),
        QStringLiteral("Старые боги уснули. Молодые ещё не родились. Мы живём в их паузе."),
        QStringLiteral("Если увидишь глаза в темноте - не моргай первым."),
        QStringLiteral("Пепел не врёт. Пепел просто всё уже видел."),
    };
    switch (kind) {
    case 0: return kMerchant;
    case 1: return kSmith;
    case 2: return kHealer;
    default: return kElder;
    }
}
static inline QPointF normv(const QPointF &v)
{
    float l = float(qSqrt(v.x() * v.x() + v.y() * v.y()));
    if (l < 0.0001f) return QPointF(1, 0);
    return QPointF(v.x() / l, v.y() / l);
}
static float resistForEnemy(const Actor &a, DmgType t)
{
    // враги слегка сопротивляются своей же стихии
    const EnemyDef *e = Data::enemy(a.defId);
    if (!e) return 0.f;
    if (e->dmgType == t && t != Dmg_Physical) return 0.35f;
    if (t == Dmg_Physical) return clampf(a.armor / (a.armor + 12.f * float(a.level) + 190.f), 0.f, 0.72f);
    return 0.05f;
}

// ============================================================
//  КОНСТРУКТОР
// ============================================================
GameView::GameView(Game *game, QWidget *parent)
    : QWidget(parent), m_game(game), m_nearNpc(0)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setMinimumSize(320, 240);
    setCursor(Qt::BlankCursor);
    for (int i = 0; i < 6; ++i) m_skillCd[i] = 0.f;
    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(onFrame()));
    m_timer->start(16);
    m_clock.start();
}

GameView::~GameView() {}

Actor *GameView::player()
{
    for (int i = 0; i < m_actors.size(); ++i)
        if (m_actors[i].isPlayer) return &m_actors[i];
    return 0;
}

Actor *GameView::findById(int id)
{
    if (id < 0) return 0;
    for (int i = 0; i < m_actors.size(); ++i) if (m_actors[i].id == id) return &m_actors[i];
    return 0;
}

// ============================================================
//  ЗАГРУЗКА ЛОКАЦИИ
// ============================================================
void GameView::clearScene()
{
    m_actors.clear();
    m_projs.clear();
    m_parts.clear();
    m_afx.clear();
    m_texts.clear();
    m_zones.clear();
    m_tels.clear();
    m_pickups.clear();
    m_npcs.clear();
    m_pending.clear();
    m_playerId = -1;
    m_bossId = -1;
    m_bossName.clear();
    m_portalActive = false;
    m_killed = 0;
    m_totalEnemies = 0;
    m_playerDead = false;
    m_deathTimer = 0;
    m_souls = 0;
    m_shake = 0;
    m_flash = 0;
}

void GameView::loadLocation(int locId)
{
    const LocationDef *L = Data::loc(locId);
    if (!L) return;
    clearScene();
    m_locId = locId;

    int kind = 0;
    if (L->type == Loc_Town) kind = 1;
    else if (L->type == Loc_Shrine) kind = 2;

    int seed = L->seed + m_game->clearCount(locId) * 131;
    m_dun = DungeonGen::generate(locId, L->biome, L->tier, kind, seed);
    m_dun.name = L->name;
    buildMinimap();
    m_decals = QImage(qMax(1, m_dun.w * m_dun.tileSize / 2), qMax(1, m_dun.h * m_dun.tileSize / 2),
                      QImage::Format_ARGB32_Premultiplied);
    m_decals.fill(Qt::transparent);
    m_decalBounds = QRectF();
    m_live.clear();
    m_liveDirty = false;
    m_groundZDirty = true;
    buildCanopy();
    m_envT = rng().frange(4.f, 9.f);

    // --- игрок ---
    Actor pl;
    pl.id = newActorId();
    pl.isPlayer = true;
    pl.pos = m_dun.playerSpawn;
    pl.radius = 14.f;
    pl.level = m_game->level();
    m_playerId = pl.id;
    m_actors.append(pl);
    syncPlayerStats();
    Actor *p = player();
    if (p) { p->hp = m_game->hp(); p->maxHp = m_game->maxHp(); }

    // --- NPC в городах ---
    if (kind == 1) {
        for (int i = 0; i < m_dun.npcPoints.size() && i < 4; ++i) {
            Npc n;
            n.id = i;
            n.pos = m_dun.npcPoints.at(i);
            n.kind = i;
            n.name = i == 0 ? QStringLiteral("Торговец Грим")
                            : (i == 1 ? QStringLiteral("Кузнец Уголь")
                               : (i == 2 ? QStringLiteral("Целительница Ива")
                                  : QStringLiteral("Старейшина Пепел")));
            n.lines = npcLines(i);
            n.line = n.lines.at(0);   // первая фраза - «каноническая», при [F] выдаётся случайная
            n.tint = i;
            m_npcs.append(n);
        }
        m_portalActive = true;
        m_totalEnemies = 0;
    } else if (kind == 2) {
        // у святилища его охраняют
        int guardians = 4 + L->tier;
        m_totalEnemies = guardians;
        for (int i = 0; i < guardians; ++i) {
            QPointF sp = m_dun.randomFloorFarFrom(m_dun.playerSpawn, 260.f);
            int def = L->enemyPool.isEmpty() ? 0 : L->enemyPool.at(i % L->enemyPool.size());
            spawnEnemy(def, sp, i == 0, 0);
        }
    } else {
        // --- боевая локация ---
        int count = L->enemyCount;
        if (m_game->isCleared(locId)) count = qMax(4, int(count * 0.75f));
        QVector<int> pool = L->enemyPool;
        if (pool.isEmpty()) pool << 0;
        int ld = qBound(0, (m_game->level() - L->reqLevel) / 3, 8);

        QVector<QPointF> pts = m_dun.spawnPoints;
        for (int i = 0; i < count; ++i) {
            QPointF sp = pts.isEmpty() ? m_dun.randomFloor() : pts.at(i % pts.size());
            // небольшой разброс
            sp += QPointF(rng().frange(-34.f, 34.f), rng().frange(-34.f, 34.f));
            int def = pool.at(rng().pick(pool.size()));
            spawnEnemy(def, sp, false, ld);
        }
        for (int i = 0; i < L->eliteCount; ++i) {
            QPointF sp = m_dun.randomFloorFarFrom(m_dun.playerSpawn, 420.f);
            int def = pool.at(rng().pick(pool.size()));
            spawnEnemy(def, sp, true, ld);
        }
        if (L->bossId >= 0) {
            QPointF sp = m_dun.randomFloorFarFrom(m_dun.playerSpawn, 520.f);
            spawnEnemy(L->bossId, sp, false, 0);
        }
        m_totalEnemies = enemiesAlive();
    }

    if (kind == 1) m_game->visitLocation(locId);   // поселение сразу открывает соседние пути
    m_portalActive = (kind == 1);

    // дрейфующий туман биома
    m_fog.clear();
    {
        int nf = (kind == 1) ? 5 : 10;
        Rng fr(quint64(seed) ^ 0xF00Dull);
        for (int i = 0; i < nf; ++i) {
            FogPatch fp;
            fp.pos = m_dun.randomFloor();
            fp.vel = QPointF(rng().frange(-9, 9), rng().frange(-5, 5));
            fp.r = rng().frange(140.f, 320.f);
            fp.alpha = rng().frange(0.05f, 0.13f) * (L && (L->biome == Bio_Swamp || L->biome == Bio_Void || L->biome == Bio_Frost) ? 1.5f : 1.f);
            fp.seed = int(rng().next());
            m_fog.append(fp);
        }
    }
    // передний план (параллакс)
    m_fg.clear();
    for (int i = 0; i < 7; ++i) {
        FgItem fi;
        fi.ax = (i % 2) ? rng().frange(0.0f, 0.16f) : rng().frange(0.84f, 1.0f);
        fi.ay = (i % 4 < 2) ? rng().frange(0.0f, 0.08f) : rng().frange(0.90f, 0.99f);
        fi.kind = i % 3;
        fi.scale = rng().frange(0.8f, 1.5f);
        fi.phase = rng().unit() * 6.28f;
        m_fg.append(fi);
    }
    m_ghosts.clear();
    m_zoom = 1.f;
    centerCamera();
    for (int i = 0; i < 6; ++i) m_skillCd[i] = 0.f;
    m_rollCd = 0;
    m_attackCd = 0;
    m_prompt.clear();
    m_huntT = 0.f;
    m_fadeIn = 1.f;
    m_bannerT = 3.0f;
    m_bannerText = L ? L->name : QString();
    m_bannerSub = L ? (Data::regionName(L->region) + QStringLiteral(" · ") + locTypeName(L->type)
                       + QStringLiteral(" · порог ") + romanNum(L->tier)) : QString();
    m_bossIntroT = 0.f;
    m_bossIntroDone = false;
    m_locTime = 0.f;
    m_locDmgTaken = 0.f;
    m_killTimes.clear();
    m_streak = 0;
    m_streakT = 0.f;
    m_shrineUsed = false;

    emit objectiveChanged();
    emit promptChanged(QString());
    if (L->type == Loc_Boss || L->type == Loc_Final)
        emit logMsg(QStringLiteral("Ты входишь: %1. Воздух здесь тяжёлый.").arg(L->name), C::bad);
    else
        emit logMsg(QStringLiteral("Ты входишь: %1.").arg(L->name), C::boneDim);
}

void GameView::syncPlayerStats()
{
    Actor *p = player();
    if (!p) return;
    const Derived &d = m_game->d();
    p->maxHp = d.maxHp;
    p->hp = qMin(m_game->hp(), d.maxHp);
    p->dmgMin = d.dmgMin;
    p->dmgMax = d.dmgMax;
    p->dmgType = d.weaponType;
    p->armor = d.armor;
    p->speed = d.moveSpeed;
    p->level = m_game->level();
    p->st.shield = m_game->shield() > 0.f ? 999.f : 0.f;
}

void GameView::respawn()
{
    m_playerDead = false;
    m_deathTimer = 0;
    int town = m_game->lastTown();
    if (town < 0) town = Data::firstLocation();
    m_game->travelTo(town);
    loadLocation(town);
    Actor *p = player();
    if (p) { p->hp = m_game->hp(); }
    setPaused(false);
}

// ============================================================
//  СПАВН
// ============================================================
void GameView::spawnEnemy(int defId, const QPointF &pos, bool elite, int levelDelta)
{
    const EnemyDef *e = Data::enemy(defId);
    if (!e) return;
    Actor a;
    a.id = newActorId();
    a.defId = defId;
    a.pos = pos;
    a.level = e->level + levelDelta;
    a.ai = int(e->ai);
    a.tint = e->tint;
    a.glyph = e->glyph;
    // коллизия и визуал раздельны: босс большой на вид, но с вменяемым хитбоксом
    a.baseRadius = e->radius;
    a.drawScale = e->boss ? e->scale : 1.f;
    a.radius = e->boss ? qMax(e->radius * 1.15f, e->radius * e->scale * 0.55f) : e->radius;
    a.atkRange = e->atkRange;
    a.atkCd = e->atkCd;
    a.atkTimer = rng().frange(0.2f, 1.2f);
    a.detect = e->detectRange;
    a.dmgType = e->dmgType;
    a.flying = e->flying;
    a.explodes = e->explodes;
    a.explodeRadius = e->explodeRadius;
    a.summoner = e->summoner;
    a.summonId = e->summonId;
    a.summonCount = e->summonCount;
    a.projSpeed = e->rangedProjSpeed;
    a.boss = e->boss;
    a.scale = e->scale;
    a.patternSet = e->patternSet;
    a.bossTitle = e->title;
    a.aiPoint = pos;
    a.spawnT = 0.f;

    float hs = 1.f + 0.28f * float(levelDelta);
    float ds = 1.f + 0.20f * float(levelDelta);
    a.maxHp = e->hp * hs;
    a.hp = a.maxHp;
    a.dmgMin = e->dmgMin * ds;
    a.dmgMax = e->dmgMax * ds;
    a.armor = e->armor * (1.f + 0.15f * levelDelta);
    a.speed = e->speed;
    a.xp = int(e->xp * 1.25f * (1.f + 0.2f * levelDelta));
    a.goldMin = int(e->goldMin * (1.f + 0.25f * levelDelta));
    a.goldMax = int(e->goldMax * (1.f + 0.25f * levelDelta));
    a.dropChance = e->dropChance;

    if (elite && !e->boss) {
        a.elite = true;
        a.nameOverride = QStringLiteral("Элитный ") + e->name;
        int nAff = rng().range(1, 2);
        const QVector<EliteAffix> &EA = Data::eliteAffixes();
        for (int i = 0; i < nAff && !EA.isEmpty(); ++i) {
            int af = rng().pick(EA.size());
            if (a.eliteAffixes.contains(af)) continue;
            a.eliteAffixes.append(af);
            switch (af) {
            case 0: a.dmgType = Dmg_Fire; break;
            case 1: a.dmgType = Dmg_Frost; break;
            case 2: a.dmgType = Dmg_Lightning; break;
            case 5: a.speed *= 1.35f; a.atkCd *= 0.7f; a.maxHp *= 0.8f; a.hp = a.maxHp; break;
            default: break;
            }
        }
        a.maxHp *= 3.4f; a.hp = a.maxHp;
        a.dmgMin *= 1.5f; a.dmgMax *= 1.5f;
        a.radius *= 1.32f;
        a.xp = int(a.xp * 3.2f);
        a.goldMin *= 3; a.goldMax *= 3;
        a.dropChance = qMin(0.95f, a.dropChance + 0.55f);
        a.eliteAuraT = rng().unit() * 6.f;
    }
    if (a.boss) {
        a.maxHp *= 1.f;
        a.hp = a.maxHp;
        m_bossId = a.id;
        m_bossName = e->title.isEmpty() ? e->name : e->title;
        m_bossHp = a.hp;
        m_bossMaxHp = a.maxHp;
        m_bossPhase = 1;
    }
    m_actors.append(a);
    if (a.boss) emit bossEngaged(defId, m_bossName);
}

void GameView::spawnSummon(int defId, const QPointF &pos, int owner, float lifeTime, float dmgMul)
{
    const EnemyDef *e = Data::enemy(defId);
    if (!e) return;
    Actor a;
    a.id = newActorId();
    a.defId = defId;
    a.pos = pos;
    a.level = m_game->level();
    a.ai = int(e->ai == Ai_Turret ? Ai_Chase : e->ai);
    a.tint = e->tint;
    a.radius = e->radius * 0.95f;
    a.atkRange = e->atkRange;
    a.atkCd = e->atkCd;
    a.detect = 900.f;
    a.dmgType = e->dmgType;
    a.maxHp = e->hp * 0.9f * dmgMul;
    a.hp = a.maxHp;
    a.dmgMin = e->dmgMin * dmgMul;
    a.dmgMax = e->dmgMax * dmgMul;
    a.armor = e->armor;
    a.speed = e->speed * 1.05f;
    a.xp = 0;
    a.goldMin = a.goldMax = 0;
    a.dropChance = 0.f;
    a.owner = owner;
    a.lifeTime = lifeTime;
    a.isSummon = true;
    a.projSpeed = e->rangedProjSpeed;
    a.spawnT = 0.f;
    m_actors.append(a);
}

// ============================================================
//  КАДР
// ============================================================
void GameView::onFrame()
{
    qint64 ns = m_clock.nsecsElapsed();
    m_clock.restart();
    float dt = float(ns) / 1e9f;
    if (dt > 0.1f) dt = 0.1f;
    m_fpsAcc += dt;
    ++m_fpsCount;
    if (m_fpsAcc > 0.5f) { m_fps = float(m_fpsCount) / m_fpsAcc; m_fpsAcc = 0; m_fpsCount = 0; }

    // авто-облегчение эффектов: при стойкой просадке кадров отключаем крону/следы/лучи
    // и грубее считаем свет; когда запас кадров появился - возвращаем обратно
    if (!m_autoQuality) { m_quality = 1; m_lowFpsT = m_hiFpsT = 0.f; }
    if (m_autoQuality && m_fps > 1.f) {
        if (m_quality == 1) {
            if (m_fps < 45.f) m_lowFpsT += dt; else m_lowFpsT = 0.f;
            if (m_lowFpsT > 2.5f) {
                m_quality = 0;
                m_lowFpsT = m_hiFpsT = 0.f;
                emit logMsg(QStringLiteral("Тяжело: эффекты облегчены (крона, следы и лучи выключены)."), C::boneDim);
            }
        } else {
            if (m_fps > 57.f) m_hiFpsT += dt; else m_hiFpsT = 0.f;
            if (m_hiFpsT > 6.f) {
                m_quality = 1;
                m_hiFpsT = m_lowFpsT = 0.f;
                emit logMsg(QStringLiteral("Запас кадров появился: эффекты снова полные."), C::boneDim);
            }
        }
    }

    if (!m_paused && m_interactive && m_dun.valid()) {
        float sdt = dt * m_timeScale;
        updateSim(sdt);
        m_game->tick(sdt);
        syncPlayerStats();
        Actor *p = player();
        if (p && !m_playerDead) m_game->addHp(0.f);
    }
    if (m_timeScale < 1.f) m_timeScale = qMin(1.f, m_timeScale + dt * 0.6f);
    update();
}

void GameView::updateSim(float dt)
{
    m_time += dt;
    m_huntT += dt;
    m_locTime += dt;
    updateLiveDecals(dt);   // живые декали: TTL ~3 сек, затухание, перекладка слоя
    if (m_streakT > 0.f) { m_streakT -= dt; if (m_streakT <= 0.f) m_streak = 0; }
    if (m_fadeIn > 0.f) m_fadeIn = qMax(0.f, m_fadeIn - dt * 1.15f);
    if (m_bannerT > 0.f) m_bannerT -= dt;
    if (m_bossIntroT > 0.f) m_bossIntroT -= dt;

    // обработка отложенных спавнов
    if (!m_pending.isEmpty()) {
        QVector<PendingSpawn> pend = m_pending;
        m_pending.clear();
        for (int i = 0; i < pend.size(); ++i) {
            const PendingSpawn &ps = pend.at(i);
            if (ps.summon) {
                if (ps.raiserId >= 0) {
                    // поднят жрецом из трупа: сражается на его стороне, недолго живёт
                    int raiserOwner = -1;
                    bool ok = false;
                    for (int k = 0; k < m_actors.size(); ++k)
                        if (m_actors[k].id == ps.raiserId && !m_actors[k].dead) { raiserOwner = m_actors[k].owner; ok = true; break; }
                    spawnSummon(ps.defId, ps.pos, ok ? raiserOwner : -1, 12.f, ok ? 0.8f : 0.7f);
                    if (!m_actors.isEmpty()) {
                        // помечаем слугу как «восставшего» из трупов — фиолетовая аура при отрисовке
                        Actor &s = m_actors[m_actors.size() - 1];
                        s.raisedByPriest = true;
                    }
                } else {
                    spawnSummon(ps.defId, ps.pos, ps.owner, ps.life, ps.dmgMul);
                }
            }
            else spawnEnemy(ps.defId, ps.pos, ps.elite, ps.levelDelta);
        }
    }

    spawnAmbient(dt);
    // дым и искры от источников света
    if (m_quality == 1) {
        QRectF vis(m_cam.x() - 100, m_cam.y() - 100, float(width()) + 200, float(height()) + 200);
        for (int i = 0; i < m_dun.props.size(); ++i) {
            const Prop &pr = m_dun.props.at(i);
            if (pr.lightRadius <= 0.f || (pr.kind != Pk_Brazier && pr.kind != Pk_Vent)) continue;
            if (!vis.contains(pr.pos)) continue;
            if (rng().chance(dt * 5.f)) {
                Part sm;
                sm.pos = pr.pos + QPointF(rng().frange(-3, 3), -pr.height * 0.9f);
                sm.vel = QPointF(rng().frange(-8, 8), -rng().frange(22, 40));
                sm.life = sm.maxLife = rng().frange(0.9f, 1.8f);
                sm.size = rng().frange(2.5f, 5.f);
                sm.c = QColor(60, 52, 48, 70);
                sm.kind = 2;
                sm.additive = false;
                m_parts.append(sm);
            }
            if (rng().chance(dt * 2.2f)) {
                Part sp;
                sp.pos = pr.pos + QPointF(0, -pr.height * 0.85f);
                sp.vel = QPointF(rng().frange(-40, 40), -rng().frange(60, 130));
                sp.life = sp.maxLife = rng().frange(0.4f, 0.9f);
                sp.size = rng().frange(1.2f, 2.2f);
                sp.c = QColor(255, 190, 80, 220);
                sp.kind = 1;
                sp.grav = 120.f;
                m_parts.append(sp);
            }
        }
    }
    m_envT -= dt;
    if (m_envT <= 0.f) {
        m_envT = rng().frange(7.f, 15.f);
        if (m_quality == 1) ambientEvent();
    }
    // дрейф тумана
    for (int i = 0; i < m_fog.size(); ++i) {
        FogPatch &fp = m_fog[i];
        fp.pos += fp.vel * dt;
        if (fp.pos.x() < -fp.r) fp.pos.setX(m_dun.widthPx() + fp.r);
        if (fp.pos.x() > m_dun.widthPx() + fp.r) fp.pos.setX(-fp.r);
        if (fp.pos.y() < -fp.r) fp.pos.setY(m_dun.heightPx() + fp.r);
        if (fp.pos.y() > m_dun.heightPx() + fp.r) fp.pos.setY(-fp.r);
    }
    updatePlayer(dt);
    updateEnemies(dt);
    updateSummons(dt);
    updateProjectiles(dt);
    updateZones(dt);
    updateTelegraphs(dt);
    updateParticles(dt);
    updateAttackFx(dt);
    updatePickups(dt);
    updateNpcs(dt);
    separateActors(dt);
    // никто не может остаться внутри стены/пропа (иначе «неуязвимость в текстурах»)
    for (int i = 0; i < m_actors.size(); ++i) {
        Actor &a = m_actors[i];
        if (!a.dead) resolveGeometry(a);
    }
    updateCamera(dt);

    // очистка мёртвых
    for (int i = m_actors.size() - 1; i >= 0; --i) {
        Actor &a = m_actors[i];
        if (a.dead) {
            a.deathT += dt * (a.boss ? 0.5f : 1.6f);
            if (a.boss) {
                a.eliteAuraT += dt;
                float tt = a.eliteAuraT;
                Spr::Pal bpal = Spr::palFor(a.tint);
                if (tt < 1.5f && rng().chance(dt * 8.f)) {
                    QPointF off(rng().frange(-a.radius, a.radius), rng().frange(-a.radius * 1.5f, a.radius * 0.3f));
                    burst(a.pos + off, 18, bpal.accent, 1, 220, 4.6f, 0.7f);
                    ring(a.pos + off, 12, bpal.light, 70.f);
                    shake(3.f);
                    Audio::inst().play(Sfx_Explosion);
                }
                if (tt >= 1.5f && a.summonTimer <= 0.f) {
                    a.summonTimer = 1.f;   // маркер: финальный взрыв сделан
                    flashScreen(QColor(255, 242, 214), 0.65f);
                    shake(22.f);
                    ring(a.pos, 56, bpal.accent, a.radius * 7.f);
                    ring(a.pos, 40, bpal.light, a.radius * 4.5f);
                    burst(a.pos, 90, bpal.light, 1, 420, 6.f, 1.5f);
                    burst(a.pos, 40, bpal.dark, 2, 160, 8.f, 2.0f);
                    Audio::inst().play(Sfx_Explosion);
                    m_timeScale = 0.3f;
                }
            }
            if (a.deathT > 1.15f) {
                if (a.id == m_bossId) { m_bossId = -1; emit bossDefeated(); }
                m_actors.removeAt(i);
            }
        }
    }
    checkObjective();
}

// ============================================================
//  ИГРОК
// ============================================================
void GameView::updatePlayer(float dt)
{
    Actor *p = player();
    if (!p) return;

    if (m_playerDead) {
        m_deathTimer += dt;
        Actor *dp = player();
        if (dp) dp->deathT += dt;   // лист героя: кадры анимации смерти
        return;
    }

    const Derived &d = m_game->d();
    p->maxHp = d.maxHp;
    p->hp = m_game->hp();
    p->speed = d.moveSpeed;

    // кулдауны
    for (int i = 0; i < 6; ++i) if (m_skillCd[i] > 0) m_skillCd[i] = qMax(0.f, m_skillCd[i] - dt);
    if (m_attackCd > 0) m_attackCd -= dt;
    if (m_rollCd > 0) m_rollCd -= dt;
    if (m_potionCd > 0) m_potionCd -= dt;
    if (m_shadowStepT > 0) m_shadowStepT -= dt;
    if (p->attackAnim > 0) p->attackAnim = qMax(0.f, p->attackAnim - dt * 4.5f);
    if (p->rollT > 0) p->rollT = qMax(0.f, p->rollT - dt * 2.9f);
    if (p->invuln > 0) p->invuln -= dt;
    if (p->hitFlash > 0) p->hitFlash -= dt * 4.f;
    p->st.tick(dt);
    if (p->st.bloodlust > 0.f) { /* активный бафф */ }

    // направление
    QPointF dir(0, 0);
    if (heldAct(Binds::A_Up)) dir.ry() -= 1;
    if (heldAct(Binds::A_Down)) dir.ry() += 1;
    if (heldAct(Binds::A_Left)) dir.rx() -= 1;
    if (heldAct(Binds::A_Right)) dir.rx() += 1;
    QPointF nd = normv(dir);
    bool moving = (dir.x() != 0 || dir.y() != 0);

    // мировая точка под курсором
    m_aim = toWorld(m_mouse);

    float spd = p->speed;
    quint8 tile = m_dun.tileAtPx(p->pos);
    if (tile == T_Water) spd *= 0.66f;
    if (tile == T_Hazard) {
        float dmg = 6.f * float(m_dun.tier) * dt;
        m_game->addHp(-dmg);
        m_locDmgTaken += dmg;
        if (rng().chance(dt * 6.f)) burst(p->pos, 3, C::ember, 1, 60, 2.5f, 0.5f);
        checkPlayerDeath();
    }
    if (p->st.chill > 0.f) spd *= 0.55f;
    if (p->st.bloodlust > 0.f) spd *= 1.10f;

    if (p->rollT > 0.f || m_shadowStepT > 0.f) {
        Ghost gh;
        gh.pos = p->pos;
        gh.life = 0.30f;
        gh.facing = p->facing;
        m_ghosts.append(gh);
        if (m_ghosts.size() > 14) m_ghosts.removeFirst();
    }
    for (int i = m_ghosts.size() - 1; i >= 0; --i) {
        m_ghosts[i].life -= dt;
        if (m_ghosts[i].life <= 0.f) m_ghosts.removeAt(i);
    }
    if (p->rollT > 0.f) {
        QPointF rdir(qCos(p->rollDir), qSin(p->rollDir));
        moveActor(*p, rdir * p->speed * 2.6f * dt);
        p->invuln = qMax(p->invuln, 0.05f);
        if (rng().chance(dt * 30.f)) burst(p->pos, 1, C::arcane, 2, 20, 4.f, 0.4f);
    } else if (moving) {
        moveActor(*p, nd * spd * dt);
        p->facing = float(qAtan2(nd.y(), nd.x()));
    }
    if (moving || p->rollT > 0) p->animT += dt;
    if (moving && p->rollT <= 0.f) {
        float prevStep = m_stepT;
        m_stepT += dt * (spd / 24.f);
        if (int(m_stepT) != int(prevStep)) {
            BiomePal bp = biomePalette(m_dun.biome);
            if (m_dun.biome == Bio_Frost || m_dun.biome == Bio_Ash || m_dun.biome == Bio_Swamp)
                stampDecal(p->pos + QPointF(rng().frange(-4, 4), rng().frange(0, 4)), 3, 7.f, bp.floor2);
            for (int i = 0; i < 2; ++i) {
                Part du;
                du.pos = p->pos + QPointF(rng().frange(-6, 6), rng().frange(-2, 2));
                du.vel = QPointF(rng().frange(-16, 16), -rng().frange(6, 18));
                du.life = du.maxLife = rng().frange(0.3f, 0.55f);
                du.size = rng().frange(2.0f, 3.4f);
                du.c = QColor(bp.floor3.red(), bp.floor3.green(), bp.floor3.blue(), 110);
                du.kind = 2;
                du.additive = false;
                m_parts.append(du);
            }
        }
    }

    // автоповорот к курсору при неподвижности
    if (!moving) {
        QPointF toAim = normv(m_aim - p->pos);
        p->facing = float(qAtan2(toAim.y(), toAim.x()));
    }
    p->rimAng = float(qAtan2(m_aim.y() - p->pos.y(), m_aim.x() - p->pos.x())) + float(M_PI);
    // пылинки в луче света героя
    if (m_quality == 1 && rng().chance(dt * 9.f)) {
        Part mo;
        float a2 = rng().unit() * 6.28f, rr2 = rng().frange(30.f, 190.f);
        mo.pos = p->pos + QPointF(qCos(a2) * rr2, qSin(a2) * rr2 * 0.7f);
        mo.vel = QPointF(rng().frange(-6, 6), -rng().frange(4, 12));
        mo.life = mo.maxLife = rng().frange(1.2f, 2.6f);
        mo.size = rng().frange(0.8f, 1.6f);
        mo.c = QColor(255, 226, 170, 90);
        mo.kind = 0;
        m_parts.append(mo);
    }

    // атака
    if (m_lmb && m_attackCd <= 0.f && p->rollT <= 0.f) basicAttack();

    // синхронизация ресурсов
    if (m_game->shield() <= 0.f) p->st.shield = 0.f;
    checkPlayerDeath();
    // «Жатва пепла»
    if (m_game->hasKeystone(QStringLiteral("ashenharvest"))) {
        if (m_souls > 0) { /* бонус урона учтён в damageActor */ }
    }

    // подсказка взаимодействия
    updatePrompt();
}

void GameView::updatePrompt()
{
    Actor *p = player();
    QString pr;
    m_nearNpc = 0;
    m_nearPortal = false;
    m_nearAltar = false;
    if (!p || m_playerDead) { if (!m_prompt.isEmpty()) { m_prompt.clear(); emit promptChanged(pr); } return; }

    for (int i = 0; i < m_npcs.size(); ++i) {
        if (distf(p->pos, m_npcs[i].pos) < 62.f) {
            m_nearNpc = &m_npcs[i];
            break;
        }
    }
    if (m_nearNpc) {
        QString what = m_nearNpc->kind == 0 ? QStringLiteral("торговать")
                     : (m_nearNpc->kind == 1 ? QStringLiteral("переучить навыки")
                        : (m_nearNpc->kind == 2 ? QStringLiteral("исцелиться")
                           : QStringLiteral("говорить")));
        pr = QStringLiteral("[F] %1 - %2").arg(m_nearNpc->name, what);
    } else if (distf(p->pos, m_dun.exitPos) < 66.f) {
        m_nearPortal = true;
        pr = m_portalActive ? QStringLiteral("[F] Портал - вернуться к карте мира")
                            : QStringLiteral("Портал закрыт, пока локация не зачищена");
    } else if (m_dun.kind == 2) {
        for (int i = 0; i < m_dun.props.size(); ++i) {
            if (m_dun.props[i].kind == Pk_Altar && distf(p->pos, m_dun.props[i].pos) < 70.f) {
                m_nearAltar = true;
                pr = m_shrineUsed ? QStringLiteral("Алтарь уже использован")
                                  : QStringLiteral("[F] Коснуться алтаря");
                break;
            }
        }
    }
    if (pr != m_prompt) { m_prompt = pr; emit promptChanged(pr); }
}

// ============================================================
//  АТАКА И УМЕНИЯ
// ============================================================
void GameView::basicAttack()
{
    Actor *p = player();
    if (!p || m_playerDead) return;
    const Derived &d = m_game->d();
    m_attackCd = 1.f / qMax(0.25f, d.weaponSpeed) * (p->st.bloodlust > 0.f ? 0.78f : 1.f);
    p->attackAnim = 1.f;
    p->attackKind = 0;
    QPointF dir = normv(m_aim - p->pos);
    p->facing = float(qAtan2(dir.y(), dir.x()));

    const Item w = m_game->equipped(Slot_Weapon);
    const ItemBaseDef *wb = w.valid ? Data::base(w.baseId) : 0;
    int wg = wb ? wb->glyph : Gl_Sword;
    bool ranged = wb && (wg == Gl_Bow);
    bool caster = wb && (wg == Gl_Staff || wg == Gl_Wand);

    if (ranged) {
        p->attackKind = 2;      // лист героя: анимация «shoot»
        if (!Audio::inst().hasAsset(Sfx_Hit)) Audio::inst().play(Sfx_Swing);
        int n = 1 + d.extraProj;
        for (int i = 0; i < n; ++i) {
            float spread = (n > 1) ? (float(i) - float(n - 1) * 0.5f) * 0.10f : 0.f;
            float ca = qCos(spread), sa = qSin(spread);
            QPointF rd(dir.x() * ca - dir.y() * sa, dir.x() * sa + dir.y() * ca);
            float dmg = rng().frange(d.dmgMin, d.dmgMax);
            fireProjectile(p->pos + rd * 16.f, rd, dmg, d.weaponType, p->id, false, 0,
                           640.f * (1.f + d.projSpeed), 5.f, 0, 0, C::bone, 1.f, false, 0, 0, 0, Gl_None);
        }
        return;
    }

    if (caster) {
        // маг/жрец с посохом или жезлом: обычная атака летит волшебным снарядом
        p->attackKind = 1;      // лист героя: анимация «cast»
        Audio::inst().play(Audio::inst().hasAsset(Sfx_Hit) ? Sfx_Swing : Sfx_Cast);
        int pk = d.weaponType == Dmg_Fire ? 1 : (d.weaponType == Dmg_Frost ? 2
               : (d.weaponType == Dmg_Lightning ? 3 : (d.weaponType == Dmg_Poison ? 4 : 5)));
        QColor col = dmgTypeColor(d.weaponType);
        float dmg = rng().frange(d.dmgMin, d.dmgMax);
        // маленькая вспышка заклинания у навершия
        spawnAttackFx(p->pos + dir * 20.f, p->facing, 5, col, 30.f, 0.34f);
        int n = 1 + d.extraProj;
        for (int i = 0; i < n; ++i) {
            float spread = (n > 1) ? (float(i) - float(n - 1) * 0.5f) * 0.10f : 0.f;
            float ca = qCos(spread), sa = qSin(spread);
            QPointF rd(dir.x() * ca - dir.y() * sa, dir.x() * sa + dir.y() * ca);
            fireProjectile(p->pos + rd * 18.f, rd, dmg, d.weaponType, p->id, false, pk,
                           560.f * (1.f + d.projSpeed), 6.f, 0, 0, col, 1.05f, false, 0, 0, 0, Gl_None);
        }
        return;
    }

    ++p->attackSeq;             // лист героя: чередование «attack»/«attack2»
    if (!Audio::inst().hasAsset(Sfx_Hit)) Audio::inst().play(Sfx_Swing);
    float range = (92.f + d.areaPct * 46.f);
    float halfAng = 1.15f + d.areaPct * 0.5f;
    // визуальный взмах: росчерк-полумесяц вместо горсти точек
    {
        QColor scol = dmgTypeColor(d.weaponType);
        int kind = 0;                       // 0 клинок, 1 секира/дробящее, 4 выпад
        if (wg == Gl_Axe || wg == Gl_GreatAxe || wg == Gl_Mace || wg == Gl_Hammer) kind = 1;
        else if (wg == Gl_GreatSword) kind = 1;
        else if (wg == Gl_Spear) kind = 4;
        float reach = range * (kind == 1 ? 0.95f : 0.82f);
        // центр дуги чуть впереди героя; чередующиеся удары зеркалятся
        QPointF cpos = p->pos + QPointF(qCos(p->facing), qSin(p->facing)) * (range * 0.28f)
                       + QPointF(0, -p->radius * 0.55f);
        spawnAttackFx(cpos, p->facing, kind, scol, reach, 0.28f, (p->attackSeq & 1) == 0);
    }
    bool hitAny = false;
    for (int i = 0; i < m_actors.size(); ++i) {
        Actor &a = m_actors[i];
        if (a.dead || a.isPlayer || a.isSummon) continue;
        float dd = distf(a.pos, p->pos);
        if (dd > range + a.radius) continue;
        QPointF to = normv(a.pos - p->pos);
        float dot = float(to.x() * dir.x() + to.y() * dir.y());
        if (dot < qCos(halfAng)) continue;
        float dmg = rng().frange(d.dmgMin, d.dmgMax);
        damageActor(a, dmg, d.weaponType, p->id, true, 130.f, p->pos, true);
        if (d.elemBonus > 0.f) damageActor(a, dmg * d.elemBonus, Dmg_Fire, p->id, false, 0.f, p->pos, true);
        stampDecal(a.pos, 2, 30.f, dmgTypeColor(d.weaponType));
        hitAny = true;
    }
    breakPropsArc(p->pos, dir, range, halfAng);
    if (hitAny) shake(2.4f);
}

bool GameView::heldAct(int act) const
{
    for (QSet<int>::const_iterator it = m_keys.constBegin(); it != m_keys.constEnd(); ++it)
        if (Binds::matches(*it, act)) return true;
    return false;
}

void GameView::dodgeRoll()
{
    Actor *p = player();
    if (!p || m_playerDead || m_rollCd > 0.f || p->rollT > 0.f) return;
    const Derived &d = m_game->d();
    QPointF dir(0, 0);
    if (heldAct(Binds::A_Up)) dir.ry() -= 1;
    if (heldAct(Binds::A_Down)) dir.ry() += 1;
    if (heldAct(Binds::A_Left)) dir.rx() -= 1;
    if (heldAct(Binds::A_Right)) dir.rx() += 1;
    if (dir.isNull()) dir = QPointF(qCos(p->facing), qSin(p->facing));
    QPointF nd = normv(dir);
    p->rollDir = float(qAtan2(nd.y(), nd.x()));
    p->rollT = 1.f;
    p->invuln = 0.34f;
    AchTracker::inst().bump(Ach::C_Rolls);
    m_rollCd = qMax(0.42f, 1.15f - d.agi * 0.004f);
    Audio::inst().play(Sfx_Roll);
    burst(p->pos, 10, C::arcane, 2, 60, 4.f, 0.5f);
    if (m_game->hasKeystone(QStringLiteral("shadowdancer"))) {
        p->st.haste = qMax(p->st.haste, 4.f);
        m_shadowStepT = 4.f;
    }
}

void GameView::castSkill(int slotIdx)
{
    Actor *p = player();
    if (!p || m_playerDead) return;
    if (slotIdx < 0 || slotIdx >= 6) return;
    int sk = m_game->hotbar(slotIdx);
    if (sk < 0) return;
    const ActiveSkill *s = Data::skill(sk);
    if (!s) return;
    int rank = qMax(1, m_game->skillRank(sk));
    if (m_skillCd[slotIdx] > 0.f) return;
    float cost = m_game->skillCost(sk);
    if (m_game->mana() < cost) {
        ftext(p->pos + QPointF(0, -40), QStringLiteral("нет маны"), C::manaHi, false, 3, 13.f);
        Audio::inst().play(Sfx_UiBad);
        return;
    }
    m_game->addMana(-cost);
    m_skillCd[slotIdx] = m_game->skillCooldown(sk);
    p->attackAnim = 1.f;
    // физические умения (рывки, взмахи) играют «attack2», магические - «cast»
    p->attackKind = (s->dmgType == Dmg_Physical) ? 3 : 1;
    doSkillFx(*s, rank);
}

float GameView::playerSkillDamage(const ActiveSkill &s, int rank) const
{
    Q_UNUSED(rank);
    const Derived &d = m_game->d();
    float base = rng().frange(d.dmgMin, d.dmgMax) * m_game->skillDamagePct(s.id);
    if (m_souls > 0 && m_game->hasKeystone(QStringLiteral("ashenharvest")))
        base *= (1.f + 0.04f * float(m_souls));
    return base;
}

void GameView::doSkillFx(const ActiveSkill &s, int rank)
{
    Actor *p = player();
    if (!p) return;
    const Derived &d = m_game->d();
    QPointF dir = normv(m_aim - p->pos);
    p->facing = float(qAtan2(dir.y(), dir.x()));
    float area = 1.f + d.areaPct;
    float dur = s.duration * (1.f + d.durationPct) * (1.f + 0.08f * (rank - 1));
    float dmg = playerSkillDamage(s, rank);
    QColor col = s.color.isValid() ? s.color : C::amber;

    switch (s.fx) {
    case Fx_Cleave: {
        if (!Audio::inst().hasAsset(Sfx_Hit)) Audio::inst().play(Sfx_Swing);
        float range = s.range * area;
        float halfAng = 1.25f;
        // широкий росчерк вместо горсти точек + сноп искр в конце дуги
        ++p->attackSeq;
        spawnAttackFx(p->pos + QPointF(qCos(p->facing), qSin(p->facing)) * (range * 0.3f)
                      + QPointF(0, -p->radius * 0.5f),
                      p->facing, 1, col.lighter(115), range * 0.92f, 0.32f, (p->attackSeq & 1) == 0);
        burst(p->pos + dir * (range * 0.55f), 10, col.lighter(125), 1, 150, 3.4f, 0.3f);
        for (int i = 0; i < m_actors.size(); ++i) {
            Actor &a = m_actors[i];
            if (a.dead || a.isPlayer || a.isSummon) continue;
            if (distf(a.pos, p->pos) > range + a.radius) continue;
            QPointF to = normv(a.pos - p->pos);
            if (float(to.x() * dir.x() + to.y() * dir.y()) < qCos(halfAng)) continue;
            damageActor(a, dmg, s.dmgType, p->id, true, 220.f, p->pos, true);
        }
        breakPropsArc(p->pos, dir, s.range * area, 1.25f);
        shake(3.2f);
        break;
    }
    case Fx_Spin: {
        Audio::inst().play(Sfx_Swing);
        Zone z;
        z.pos = p->pos;
        z.radius = s.radius * area;
        z.life = z.maxLife = dur;
        z.tickRate = 0.32f;
        z.dps = dmg / 0.32f;
        z.type = s.dmgType;
        z.owner = p->id;
        z.hostile = false;
        z.kind = 3;
        z.c = col;
        z.follow = p->id;
        m_zones.append(z);
        breakPropsRadius(p->pos, s.radius * area);
        break;
    }
    case Fx_Dash: {
        if (s.dmgPct <= 0.f) {
            // телепорт
            Audio::inst().play(Sfx_Cast);
            QPointF target = p->pos + dir * s.range;
            // не телепортируем в стену
            for (float k = 1.f; k >= 0.15f; k -= 0.1f) {
                QPointF tp = p->pos + dir * s.range * k;
                if (!blockedAt(tp, p->radius)) { target = tp; break; }
            }
            burst(p->pos, 22, col, 2, 120, 5.f, 0.6f);
            p->pos = target;
            m_dun.collideCircle(p->pos, p->radius);
            burst(p->pos, 22, col, 2, 120, 5.f, 0.6f);
            p->invuln = qMax(p->invuln, 0.25f);
        } else {
            Audio::inst().play(Sfx_Roll);
            float dist = s.range;
            QPointF from = p->pos;
            QPointF step = dir * (dist / 14.f);
            for (int i = 0; i < 14; ++i) {
                QPointF np = p->pos + step;
                if (blockedAt(np, p->radius)) break;
                p->pos = np;
                for (int k = 0; k < m_actors.size(); ++k) {
                    Actor &a = m_actors[k];
                    if (a.dead || a.isPlayer || a.isSummon) continue;
                    if (distf(a.pos, p->pos) < s.radius * area * 0.6f + a.radius) {
                        damageActor(a, dmg * 0.6f, s.dmgType, p->id, true, 160.f, from, true);
                    }
                }
                if (rng().chance(0.6f)) burst(p->pos, 2, col, 2, 50, 4.f, 0.4f);
            }
            m_dun.collideCircle(p->pos, p->radius);
            p->invuln = qMax(p->invuln, 0.3f);
            shake(2.5f);
        }
        break;
    }
    case Fx_Projectile: {
        Audio::inst().play(s.dmgType == Dmg_Fire ? Sfx_Fire : Sfx_Cast);
        spawnAttackFx(p->pos + dir * 18.f, p->facing, 5, col, 34.f, 0.34f);
        int n = s.projectiles + d.extraProj;
        for (int i = 0; i < n; ++i) {
            float spread = (n > 1) ? (float(i) - float(n - 1) * 0.5f) * 0.12f : 0.f;
            float ca = qCos(spread), sa = qSin(spread);
            QPointF rd(dir.x() * ca - dir.y() * sa, dir.x() * sa + dir.y() * ca);
            fireProjectile(p->pos + rd * 18.f, rd, dmg, s.dmgType, p->id, false,
                           s.dmgType == Dmg_Fire ? 1 : 5,
                           s.projSpeed * (1.f + d.projSpeed), s.radius * 0.42f * area, s.pierce, s.chains,
                           col, 1.15f, true, s.radius * area, 0.f, 0.f, Gl_None);
        }
        break;
    }
    case Fx_MultiShot: {
        if (!Audio::inst().hasAsset(Sfx_Hit)) Audio::inst().play(Sfx_Swing);
        int n = s.projectiles + d.extraProj;
        for (int i = 0; i < n; ++i) {
            float spread = (float(i) - float(n - 1) * 0.5f) * 0.16f;
            float ca = qCos(spread), sa = qSin(spread);
            QPointF rd(dir.x() * ca - dir.y() * sa, dir.x() * sa + dir.y() * ca);
            fireProjectile(p->pos + rd * 16.f, rd, dmg, s.dmgType, p->id, false, 0,
                           s.projSpeed * (1.f + d.projSpeed), 5.f, s.pierce, 0, col, 1.f, false, 0, 0.f, 0.f, Gl_None);
        }
        break;
    }
    case Fx_Pierce: {
        Audio::inst().play(Sfx_Ice);
        int n = 1 + d.extraProj;
        for (int i = 0; i < n; ++i) {
            float spread = (n > 1) ? (float(i) - float(n - 1) * 0.5f) * 0.10f : 0.f;
            float ca = qCos(spread), sa = qSin(spread);
            QPointF rd(dir.x() * ca - dir.y() * sa, dir.x() * sa + dir.y() * ca);
            fireProjectile(p->pos + rd * 16.f, rd, dmg, s.dmgType, p->id, false, 2,
                           s.projSpeed * (1.f + d.projSpeed), 8.f * area, s.pierce, 0, col, 1.2f,
                           false, 0, 0.45f, 0.f, Gl_None);
        }
        break;
    }
    case Fx_Meteor: {
        Audio::inst().play(Sfx_Cast);
        Telegraph t;
        t.pos = m_aim;
        t.radius = s.radius * area;
        t.dur = 0.85f;
        t.kind = 0;
        t.c = C::ember;
        t.dmg = dmg;
        t.type = s.dmgType;
        t.owner = p->id;
        m_tels.append(t);
        break;
    }
    case Fx_Nova: {
        Audio::inst().play(Sfx_Explosion);
        float R = s.radius * area;
        for (int i = 0; i < m_actors.size(); ++i) {
            Actor &a = m_actors[i];
            if (a.dead || a.isPlayer || a.isSummon) continue;
            float dd = distf(a.pos, p->pos);
            if (dd < R + a.radius) {
                float fall = 1.f - 0.4f * clampf(dd / R, 0.f, 1.f);
                damageActor(a, dmg * fall, s.dmgType, p->id, true, 260.f, p->pos, true);
            }
        }
        Zone z;
        z.pos = p->pos; z.radius = R; z.life = z.maxLife = 0.42f; z.tickRate = 99.f; z.dps = 0;
        z.type = s.dmgType; z.owner = p->id; z.hostile = false; z.kind = 3; z.c = col;
        m_zones.append(z);
        ring(p->pos, 26, col, R);
        spawnAttackFx(p->pos, 0.f, 3, col, R, 0.5f);   // ударная волна по земле
        shake(6.f);
        breakPropsRadius(p->pos, R);
        break;
    }
    case Fx_Chain: {
        Audio::inst().play(Sfx_Shock);
        int maxTargets = s.chains + d.chainBonus + (rank - 1);
        QVector<int> hitIds;
        QPointF from = p->pos;
        Actor *cur = nearestHostile(p->pos, s.range, true);
        float curDmg = dmg;
        int guard = 0;
        while (cur && guard++ < 32) {
            hitIds.append(cur->id);
            Telegraph b;
            b.kind = 1; b.pos = from; b.pos2 = cur->pos; b.width = 10.f;
            b.dur = 0.20f; b.c = col; b.dmg = 0; b.owner = p->id;
            m_tels.append(b);
            damageActor(*cur, curDmg, s.dmgType, p->id, true, 30.f, from, true);
            burst(cur->pos, 8, col, 1, 140, 3.f, 0.4f);
            from = cur->pos;
            curDmg *= 0.82f;
            if (int(hitIds.size()) >= maxTargets) break;
            Actor *next = 0;
            float bestD = 220.f;
            for (int i = 0; i < m_actors.size(); ++i) {
                Actor &a = m_actors[i];
                if (a.dead || a.isPlayer || a.isSummon || hitIds.contains(a.id)) continue;
                float dd = distf(a.pos, from);
                if (dd < bestD) { bestD = dd; next = &a; }
            }
            cur = next;
        }
        break;
    }
    case Fx_Zone: {
        Audio::inst().play(Sfx_Poison);
        Zone z;
        z.pos = m_aim;
        z.radius = s.radius * area;
        z.life = z.maxLife = dur;
        z.tickRate = 0.42f;
        z.dps = dmg / 0.42f;
        z.type = s.dmgType;
        z.owner = p->id;
        z.hostile = false;
        z.kind = 1;
        z.c = col;
        m_zones.append(z);
        burst(m_aim, 18, col, 2, 90, 5.f, 0.9f);
        break;
    }
    case Fx_Summon: {
        Audio::inst().play(Sfx_Cast);
        int def = 31; // тень-прислужник (умолч. для старых умений)
        if (s.scale >= 20.f && s.scale < 60.f) def = int(s.scale + 0.5f); // scale хранит id существа
        else if (m_game->level() >= 22) def = 32;
        // лимит живых прислужников игрока
        int alive = 0;
        for (int k = 0; k < m_actors.size(); ++k) {
            const Actor &o = m_actors[k];
            if (!o.dead && o.isSummon && o.owner == p->id) ++alive;
        }
        int cap = qMax(2, 2 + int(m_game->d().summonPct * 4.f));
        int toSpawn = qMin(s.projectiles, qMax(0, cap - alive));
        if (toSpawn <= 0) {
            ftext(p->pos + QPointF(0, -46), QStringLiteral("прислужников полно"), C::boneDim, false, 3, 13.f);
            break;
        }
        for (int i = 0; i < toSpawn; ++i) {
            PendingSpawn ps;
            ps.summon = true;
            ps.defId = def;
            float a = rng().frange(0.f, 6.283f);
            ps.pos = p->pos + dir * 46.f + QPointF(std::cos(a), std::sin(a)) * 18.f;
            ps.owner = p->id;
            ps.life = dur;
            ps.dmgMul = 1.2f + d.summonPct;
            m_pending.append(ps);
        }
        ring(p->pos + dir * 46.f, 16, col, 40.f);
        break;
    }
    case Fx_Shield: {
        Audio::inst().play(Sfx_Shield);
        float amount = (d.shieldMax * 0.5f + d.maxHp * 0.18f + d.spirit * 1.2f) * (1.f + 0.25f * (rank - 1));
        m_game->addShield(amount);
        p->st.shield = dur;
        ring(p->pos, 20, C::manaHi, 54.f);
        ftext(p->pos + QPointF(0, -46), QStringLiteral("+%1 барьер").arg(int(amount)), C::manaHi, false, 3, 14.f);
        break;
    }
    case Fx_Heal: {
        Audio::inst().play(Sfx_Heal);
        Zone z;
        z.pos = p->pos;
        z.radius = 40.f;
        z.life = z.maxLife = dur;
        z.tickRate = 0.5f;
        z.dps = -(d.maxHp * 0.055f + d.spirit * 0.5f) * (1.f + 0.2f * (rank - 1)) / 0.5f;
        z.type = Dmg_Pure;
        z.owner = p->id;
        z.hostile = false;
        z.kind = 4;
        z.c = C::good;
        z.follow = p->id;
        m_zones.append(z);
        break;
    }
    case Fx_Buff: {
        Audio::inst().play(Sfx_Cast);
        p->st.bloodlust = dur;
        p->st.haste = dur;
        ring(p->pos, 22, C::bloodHi, 50.f);
        ftext(p->pos + QPointF(0, -46), QStringLiteral("Ярость!"), C::bloodHi, true, 3, 15.f);
        break;
    }
    case Fx_Mark: {
        Audio::inst().play(Sfx_Cast);
        fireProjectile(p->pos + dir * 16.f, dir, dmg, s.dmgType, p->id, false, 7,
                       s.projSpeed, 8.f, 0, 0, C::blood, 1.f, false, 0, 0.f, 0.f, Gl_Mark);
        break;
    }
    case Fx_Quake: {
        Audio::inst().play(Sfx_Explosion);
        for (int i = 0; i < 3; ++i) {
            Zone z;
            z.pos = p->pos;
            z.radius = (90.f + i * 62.f) * area;
            z.life = z.maxLife = 0.30f;
            z.delay = 0.14f * float(i);
            z.tickRate = 0.30f;
            z.dps = dmg * (1.f - i * 0.18f) / 0.30f;
            z.type = s.dmgType;
            z.owner = p->id;
            z.hostile = false;
            z.kind = 3;
            z.c = col;
            z.follow = (i == 0) ? p->id : -1;
            m_zones.append(z);
        }
        shake(7.f);
        burst(p->pos, 26, col, 3, 200, 5.f, 0.8f);
        break;
    }
    default: break;
    }
}

// ============================================================
//  СНАРЯДЫ / УРОН / СТАТУСЫ
// ============================================================
void GameView::fireProjectile(const QPointF &from, const QPointF &dir, float dmg, DmgType type, int owner,
                              bool hostile, int kind, float speed, float radius, int pierce, int chains,
                              const QColor &col, float scale, bool explode, float explodeRadius, float slow,
                              float dotDps, int glyph)
{
    Proj pr;
    pr.id = ++m_nextId;
    pr.pos = from;
    pr.startPos = from;
    pr.vel = QPointF(dir.x() * speed, dir.y() * speed);
    pr.dmg = dmg;
    pr.type = type;
    pr.owner = owner;
    pr.hostile = hostile;
    pr.kind = kind;
    pr.radius = radius;
    pr.pierce = pierce;
    pr.chains = chains;
    pr.color = col;
    pr.scale = scale;
    pr.life = 3.2f;
    pr.explodeOnHit = explode;
    pr.explodeRadius = explodeRadius;
    pr.slow = slow;
    pr.dotDps = dotDps;
    pr.glyph = glyph;
    m_projs.append(pr);
}

void GameView::applyStatus(Actor &a, DmgType type, float power, float dur)
{
    if (a.dead) return;
    switch (type) {
    case Dmg_Fire:
        a.st.burn = qMax(a.st.burn, dur);
        a.st.burnDps = qMax(a.st.burnDps, power * 0.30f);
        break;
    case Dmg_Frost:
        a.st.chill = qMax(a.st.chill, dur);
        break;
    case Dmg_Lightning:
        a.st.shock = qMax(a.st.shock, dur * 1.3f);
        break;
    case Dmg_Poison:
        a.st.poison = qMax(a.st.poison, dur * 1.4f);
        a.st.poisonDps = qMax(a.st.poisonDps, power * 0.22f);
        break;
    default: break;
    }
}

void GameView::damageActor(Actor &tgt, float amount, DmgType type, int sourceId, bool canCrit,
                           float knock, const QPointF &from, bool isPlayerSource, bool silent)
{
    if (tgt.dead || amount <= 0.f) return;
    const Derived &d = m_game->d();
    bool crit = false;

    // уровни сложности: множители входящего/исходящего урона
    if (tgt.isPlayer) {
        amount *= m_game->dmgTakenMul();
    } else if (isPlayerSource) {
        amount *= m_game->dmgDealtMul();
    }

    if (tgt.isPlayer) {
        if (tgt.invuln > 0.f || tgt.rollT > 0.f) {
            ftext(tgt.pos + QPointF(0, -44), QStringLiteral("уклон"), C::frost, false, 3, 12.f);
            return;
        }
        if (rng().unit() < d.dodge) {
            ftext(tgt.pos + QPointF(0, -44), QStringLiteral("УКЛОНЕНИЕ"), C::frost, true, 3, 13.f);
            Audio::inst().play(Sfx_Roll);
            AchTracker::inst().bump(Ach::C_Dodges);
            if (m_game->hasKeystone(QStringLiteral("shadowdancer"))) { tgt.st.haste = qMax(tgt.st.haste, 4.f); m_shadowStepT = 4.f; }
            return;
        }
        float res = d.resistOf(type);
        amount *= (1.f - clampf(res, -0.5f, 0.85f));
        if (m_game->hasKeystone(QStringLiteral("bloodash")) && d.maxHp > 0 && m_game->hp() / d.maxHp < 0.40f)
            amount *= 0.88f;
    } else {
        float mit = resistForEnemy(tgt, type);
        amount *= (1.f - clampf(mit, -0.4f, 0.80f));
        if (tgt.elite && tgt.eliteAffixes.contains(4)) amount *= 0.5f;   // бронированный
        if (tgt.st.mark > 0.f) amount *= 1.35f;
        if (tgt.st.shock > 0.f) amount *= 1.18f;
        if (isPlayerSource) {
            float cc = d.critChance * (tgt.st.mark > 0.f ? 1.15f : 1.f);
            if (m_game->hasKeystone(QStringLiteral("bloodash")) && d.maxHp > 0 && m_game->hp() / d.maxHp < 0.40f)
                cc += 0.40f;
            if (canCrit && rng().unit() < cc) { crit = true; amount *= d.critMult; AchTracker::inst().bump(Ach::C_Crits); }
        }
    }

    amount = qMax(1.f, amount);
    tgt.lastHitType = int(type);

    // визуал попадания
    tgt.hitFlash = 1.f;
    tgt.hitTilt = 1.f;
    tgt.tiltDir = float(qAtan2(tgt.pos.y() - from.y(), tgt.pos.x() - from.x()));
    {
        Part fl;
        fl.pos = tgt.pos + QPointF(0, -tgt.radius * 0.8f) + normv(from - tgt.pos) * tgt.radius * 0.5f;
        fl.vel = QPointF(0, 0);
        fl.life = fl.maxLife = 0.16f;
        fl.size = crit ? 9.f : 6.f;
        fl.c = QColor(255, 246, 220, 230);
        fl.kind = 6;
        fl.rot = rng().unit() * 1.57f;
        m_parts.append(fl);
    }
    QColor dc = dmgTypeColor(type);
    int npart = crit ? 14 : 8;
    burst(tgt.pos + QPointF(0, -tgt.radius * 0.7f), npart, dc, crit ? 1 : 0, crit ? 190.f : 120.f, crit ? 3.6f : 2.6f, 0.45f);

    if (tgt.isPlayer) {
        float dmg = amount;
        m_game->drainShield(dmg);
        if (dmg > 0.f) m_game->addHp(-dmg);
        m_locDmgTaken += amount;
        tgt.hp = m_game->hp();
        checkPlayerDeath();
        ftext(tgt.pos + QPointF(rng().frange(-14, 14), -48), QString::number(int(amount)), C::bloodHi, false, 0, 17.f);
        Audio::inst().play(Sfx_Hurt);
        shake(qMin(9.f, 2.f + amount / qMax(1.f, d.maxHp) * 60.f));
        flashScreen(C::blood, qMin(0.45f, amount / qMax(1.f, d.maxHp) * 2.2f));
        applyStatus(tgt, type, amount, 3.0f);
        // шипы
        if (d.thorns > 0.f && sourceId >= 0) {
            Actor *src = findById(sourceId);
            if (src && !src->isPlayer) damageActor(*src, d.thorns, Dmg_Physical, tgt.id, false, 0.f, tgt.pos, false);
        }
    } else {
        tgt.hp -= amount;
        if (crit && isPlayerSource) m_hitStop = qMax(m_hitStop, 0.055f);
        ftext(tgt.pos + QPointF(rng().frange(-12, 12), -tgt.radius * 1.9f),
              crit ? QString::number(int(amount)) + QStringLiteral("!") : QString::number(int(amount)),
              crit ? C::lightning : C::bone, crit, crit ? 1 : 0, crit ? 20.f : 15.f);
        applyStatus(tgt, type, amount, 3.0f);
        if (knock > 0.f && !tgt.boss) {
            QPointF kd = normv(tgt.pos - from);
            float k = knock / qMax(20.f, tgt.radius * 8.f);
            tgt.knockX += kd.x() * k;
            tgt.knockY += kd.y() * k;
        }
        if (!silent) Audio::inst().play(crit ? Sfx_Crit : Sfx_Hit);

        // вампиризм игрока
        if (isPlayerSource) {
            AchTracker::inst().bump(Ach::C_DmgDealt, qint64(amount));
            AchTracker::inst().setCounterMax(Ach::C_MaxHit, qint64(amount));
            if (d.lifeSteal > 0.f) m_game->addHp(amount * d.lifeSteal);
            if (d.manaSteal > 0.f) m_game->addMana(amount * d.manaSteal);
            if (m_game->hasKeystone(QStringLiteral("overload")) && crit && type != Dmg_Physical) {
                // разряд по площади
                float R = 78.f * (1.f + d.areaPct);
                for (int i = 0; i < m_actors.size(); ++i) {
                    Actor &o = m_actors[i];
                    if (o.dead || o.isPlayer || o.isSummon || o.id == tgt.id) continue;
                    if (distf(o.pos, tgt.pos) < R + o.radius)
                        damageActor(o, amount * 0.45f, Dmg_Lightning, sourceId, false, 40.f, tgt.pos, true);
                }
                ring(tgt.pos, 14, C::lightning, R);
            }
        }
        // отражающий элитник
        if (tgt.elite && tgt.eliteAffixes.contains(7) && isPlayerSource) {
            Actor *pl = player();
            if (pl) damageActor(*pl, amount * 0.15f, Dmg_Pure, tgt.id, false, 0.f, tgt.pos, false);
        }
        // вампирский элитник
        if (tgt.elite && tgt.eliteAffixes.contains(3)) {
            tgt.hp = qMin(tgt.maxHp, tgt.hp + amount * 0.25f);
        }
        if (tgt.hp <= 0.f) killActor(tgt, sourceId);
        else if (tgt.boss) {
            m_bossHp = tgt.hp;
            emit bossHealthChanged(tgt.hp, tgt.maxHp, tgt.phase);
        }
    }
}

void GameView::checkPlayerDeath()
{
    if (m_playerDead) return;
    if (m_game->hp() > 0.f) return;
    Actor *p = player();
    m_playerDead = true;
    m_deathTimer = 0;
    if (p) { p->dead = true; p->deathT = 0; }
    m_lmb = false;
    m_timeScale = 0.4f;
    Audio::inst().play(Sfx_Death);
    shake(18.f);
    flashScreen(C::blood, 0.8f);
    if (p) burst(p->pos, 46, C::blood, 4, 200, 4.5f, 1.3f);
    m_game->onPlayerDeath();
    emit playerDiedSignal();
}

void GameView::resolveGeometry(Actor &a)
{
    if (a.flying) return;
    m_dun.collideCircle(a.pos, a.radius);
    QVector<const Prop *> ps = m_dun.solidPropsNear(a.pos, a.radius);
    for (int i = 0; i < ps.size(); ++i) {
        const Prop *pr = ps.at(i);
        QPointF d = a.pos - pr->pos;
        float dl = float(qSqrt(d.x() * d.x() + d.y() * d.y()));
        float need = a.radius + pr->radius;
        if (dl < need && dl > 0.001f) a.pos = pr->pos + QPointF(d.x() / dl * need, d.y() / dl * need);
        else if (dl <= 0.001f) a.pos += QPointF(need, 0);
    }
    a.pos.setX(qBound(float(a.radius), FX(a.pos), float(m_dun.widthPx() - a.radius)));
    a.pos.setY(qBound(float(a.radius), FY(a.pos), float(m_dun.heightPx() - a.radius)));
}

void GameView::healActor(Actor &a, float amount)
{
    if (a.isPlayer) {
        m_game->addHp(amount);
        a.hp = m_game->hp();
        ftext(a.pos + QPointF(0, -48), QStringLiteral("+%1").arg(int(amount)), C::good, false, 2, 15.f);
    } else {
        a.hp = qMin(a.maxHp, a.hp + amount);
    }
}

void GameView::killActor(Actor &a, int killerId)
{
    if (a.dead) return;
    a.dead = true;
    a.deathT = 0.f;
    a.hp = 0.f;

    bool byPlayer = (killerId == m_playerId);
    if (!byPlayer && killerId >= 0) {
        Actor *src = findById(killerId);
        if (src && src->isSummon && src->owner == m_playerId) byPlayer = true;
        if (src && src->isPlayer) byPlayer = true;
    }

    Spr::Pal pal = Spr::palFor(a.tint);
    burst(a.pos + QPointF(0, -a.radius * 0.6f), a.boss ? 90 : 22, pal.dark, 4, a.boss ? 320.f : 150.f, a.boss ? 6.f : 3.4f, a.boss ? 1.6f : 0.9f);
    stampDecal(a.pos, 1, a.baseRadius * (a.boss ? 2.2f : 1.1f), QColor(120, 20, 18));
    burst(a.pos + QPointF(0, -a.radius * 0.6f), a.boss ? 60 : 12, C::blood, 4, a.boss ? 260.f : 120.f, 3.f, 1.0f);
    Audio::inst().play(a.boss ? Sfx_Explosion : Sfx_Kill);

    if (a.explodes && a.explodeRadius > 0.f) {
        for (int i = 0; i < m_actors.size(); ++i) {
            Actor &o = m_actors[i];
            if (o.dead || o.id == a.id) continue;
            if (distf(o.pos, a.pos) < a.explodeRadius + o.radius)
                damageActor(o, (a.dmgMin + a.dmgMax) * 0.8f, Dmg_Fire, a.id, false, 200.f, a.pos, !o.isPlayer);
        }
        Actor *pl = player();
        if (pl && !pl->dead && distf(pl->pos, a.pos) < a.explodeRadius)
            damageActor(*pl, (a.dmgMin + a.dmgMax) * 0.8f, Dmg_Fire, a.id, false, 200.f, a.pos, false);
        ring(a.pos, 24, C::ember, a.explodeRadius);
        shake(5.f);
    }
    if (a.elite && a.eliteAffixes.contains(6)) {
        for (int i = 0; i < 3; ++i) {
            PendingSpawn ps;
            ps.summon = false;
            ps.defId = a.defId;
            ps.pos = a.pos + QPointF(rng().frange(-40, 40), rng().frange(-40, 40));
            ps.elite = false;
            ps.levelDelta = -2;
            m_pending.append(ps);
        }
    }

    if (a.boss) {
        shake(22.f);
        flashScreen(C::amber, 0.5f);
        m_timeScale = 0.35f;
        emit bossDefeated();
        emit logMsg(QStringLiteral("%1 повержен!").arg(a.bossTitle.isEmpty() ? a.nameOverride : a.bossTitle), C::amber);
    }

    if (byPlayer && !a.isSummon) {
        m_killed++;
        m_game->registerKill();
        m_streak++;
        m_streakT = 3.5f;
        if (m_streak == 5 || m_streak == 10 || m_streak == 20 || m_streak == 50) {
            ftext(a.pos + QPointF(0, -60), QStringLiteral("СЕРИЯ x%1!").arg(m_streak), C::amber, true, 3, 18.f);
            Audio::inst().play(Sfx_Ach);
            emit logMsg(QStringLiteral("Серия убийств: x%1!").arg(m_streak), C::amber);
        }
        AchTracker &AC = AchTracker::inst();
        AC.bump(Ach::C_Kills);
        if (a.lastHitType == Dmg_Fire) AC.bump(Ach::C_KillsFire);
        if (a.elite) AC.bump(Ach::C_EliteKills);
        if (a.boss) {
            AC.bump(Ach::C_BossKills);
            switch (a.defId) {
            case 5:  AC.setBit(Ach::M_RegionBoss, 0); break;
            case 11: AC.setBit(Ach::M_RegionBoss, 1); break;
            case 17: AC.setBit(Ach::M_RegionBoss, 2); break;
            case 23: AC.setBit(Ach::M_RegionBoss, 3); break;
            case 29:
                AC.setBit(Ach::M_RegionBoss, 4);
                AC.bump(Ach::C_Final);
                if (m_game->deaths() == 0) AC.bump(Ach::C_FinalNoDeath);
                break;
            default: break;
            }
        }
        m_killTimes.append(m_time);
        while (!m_killTimes.isEmpty() && m_time - m_killTimes.first() > 2.5f) m_killTimes.removeFirst();
        if (m_killTimes.size() >= 3) AC.bump(Ach::C_MultiKill);
        if (a.xp > 0) {
            m_game->addXp(float(a.xp));
            ftext(a.pos + QPointF(0, -a.radius * 2.4f), QStringLiteral("+%1 оп.").arg(a.xp), C::xp, false, 4, 13.f);
        }
        if (m_game->hasKeystone(QStringLiteral("ashenharvest"))) {
            m_souls = qMin(10, m_souls + 1);
            healActor(*player(), m_game->d().maxHp * 0.03f);
        }
        const LocationDef *L = Data::loc(m_locId);
        int ilvl = L ? L->tier * 6 : m_game->level();
        dropLoot(a.pos, ilvl, a.dropChance, a.goldMin, a.goldMax, a.boss || a.elite);
    }
}

// ============================================================
//  ЛУТ
// ============================================================
void GameView::dropLoot(const QPointF &pos, int ilvl, float dropChance, int goldMin, int goldMax, bool boss)
{
    const Derived &d = m_game->d();
    // золото
    int gold = rng().range(goldMin, qMax(goldMin + 1, goldMax));
    gold = int(float(gold) * (1.f + d.goldFind));
    if (gold > 0) {
        Pickup pk;
        pk.kind = 1;
        pk.gold = gold;
        pk.pos = pos + QPointF(rng().frange(-6, 6), rng().frange(-4, 4));
        pk.vel = QPointF(rng().frange(-130, 130), rng().frange(-130, 90));
        m_pickups.append(pk);
    }
    int rolls = boss ? rng().range(3, 5) : 1;
    float chance = dropChance * (1.f + d.magicFind) * (boss ? 3.0f : 1.f);
    for (int i = 0; i < rolls; ++i) {
        if (!rng().chance(chance)) continue;
        int lvl = ilvl + rng().range(-1, 2);
        Item it;
        float rr = rng().unit();
        if (rr < 0.30f) it = m_game->rollPotion(lvl);
        else it = m_game->rollItem(lvl, d.magicFind);
        if (!it.valid) continue;
        Pickup pk;
        pk.kind = it.stackable() ? 2 : 0;
        pk.item = it;
        pk.pos = m_dun.nearestFree(pos + QPointF(rng().frange(-26, 26), rng().frange(-20, 20)));
        m_pickups.append(pk);
    }
    if (boss) {
        // гарантированный уникальный/легендарный
        Item it = rng().chance(0.45f) ? m_game->rollUnique(ilvl + 4)
                                      : m_game->rollItem(ilvl + 4, 2.0f, Rar_Legend);
        if (it.valid) {
            Pickup pk;
            pk.kind = 0;
            pk.item = it;
            pk.pos = pos + QPointF(rng().frange(-18, 18), rng().frange(-14, 14));
            m_pickups.append(pk);
        }
        Item soul = m_game->makeItem(Data::bases().at(0).id, Rar_Normal, 1);
        // осколок души
        for (int i = 0; i < Data::bases().size(); ++i) {
            if (Data::bases()[i].id_str == QLatin1String("m_soul")) {
                soul = m_game->makeItem(i, Rar_Normal, 1);
                break;
            }
        }
        if (soul.valid) {
            Pickup pk; pk.kind = 2; pk.item = soul; pk.pos = pos + QPointF(0, 26);
            m_pickups.append(pk);
        }
    }
}

void GameView::updatePickups(float dt)
{
    Actor *p = player();
    for (int i = m_pickups.size() - 1; i >= 0; --i) {
        Pickup &pk = m_pickups[i];
        pk.bob += dt;
        pk.age += dt;
        if (pk.age < 0.55f && !pk.vel.isNull()) {
            pk.pos += pk.vel * dt;
            pk.vel *= (1.f - dt * 5.f);
            m_dun.collideCircle(pk.pos, 6.f);
        }
        if (m_dun.solidAtPx(pk.pos)) pk.pos = m_dun.nearestFree(pk.pos);
        if (pk.life > 0.f) { pk.life -= dt; if (pk.life <= 0.f) { m_pickups.removeAt(i); continue; } }
        if (!p || m_playerDead) continue;
        float dd = distf(p->pos, pk.pos);
        // золото притягивается
        if (pk.kind == 1 && dd < 90.f) {
            QPointF dir = normv(p->pos - pk.pos);
            pk.pos += dir * (240.f - dd) * dt * 3.f;
        }
        if (dd > 30.f) continue;
        if (pk.kind == 1) {
            m_game->addGold(pk.gold);
            AchTracker::inst().bump(Ach::C_Pickups);
            ftext(pk.pos, QStringLiteral("+%1").arg(pk.gold), C::gold, false, 3, 13.f);
            Audio::inst().play(Sfx_Gold);
            m_pickups.removeAt(i);
            continue;
        }
        if (!pk.item.valid) { m_pickups.removeAt(i); continue; }
        QString nm = Data::itemNameFull(pk.item);
        QColor rc = rarityColor(pk.item.rarity);
        if (m_game->addItem(pk.item)) {
            AchTracker &AC = AchTracker::inst();
            AC.bump(Ach::C_Pickups);
            if (pk.item.rarity >= Rar_Rare) AC.bump(Ach::C_Rare);
            if (pk.item.rarity >= Rar_Legend) AC.bump(Ach::C_Legend);
            if (pk.item.rarity == Rar_Unique) {
                AC.bump(Ach::C_Uniques);
                if (pk.item.uniqueId >= 0) AC.setBit(Ach::M_Uniques, pk.item.uniqueId);
            }
            ftext(pk.pos, nm, rc, pk.item.rarity >= Rar_Rare, 3, pk.item.rarity >= Rar_Rare ? 15.f : 13.f);
            emit lootPicked(nm, rc);
            if (pk.item.rarity >= Rar_Rare)
                emit logMsg(QStringLiteral("Найдено: %1").arg(nm), rc);
            Audio::inst().play(pk.kind == 1 ? Sfx_Gold : Sfx_Pickup);
            if (pk.item.rarity == Rar_Unique) { ring(pk.pos, 20, rc, 46.f); flashScreen(rc, 0.18f); }
            m_pickups.removeAt(i);
        } else {
            // сумка полна
            if (rng().chance(dt)) emit logMsg(QStringLiteral("Сумка полна!"), C::bad);
        }
    }
}

// ============================================================
//  ДВИЖЕНИЕ / КОЛЛИЗИИ
// ============================================================
bool GameView::blockedAt(const QPointF &p, float radius) const
{
    int tx0 = int((p.x() - radius) / m_dun.tileSize), tx1 = int((p.x() + radius) / m_dun.tileSize);
    int ty0 = int((p.y() - radius) / m_dun.tileSize), ty1 = int((p.y() + radius) / m_dun.tileSize);
    for (int ty = ty0; ty <= ty1; ++ty)
        for (int tx = tx0; tx <= tx1; ++tx)
            if (m_dun.tileAt(tx, ty) == T_Wall) return true;
    return false;
}

void GameView::moveActor(Actor &a, const QPointF &delta)
{
    a.pos += delta;
    resolveGeometry(a);
}

void GameView::separateActors(float dt)
{
    Q_UNUSED(dt);
    for (int i = 0; i < m_actors.size(); ++i) {
        Actor &a = m_actors[i];
        if (a.dead) continue;
        for (int j = i + 1; j < m_actors.size(); ++j) {
            Actor &b = m_actors[j];
            if (b.dead || b.flying != a.flying) continue;
            float dx = FX(b.pos - a.pos), dy = FY(b.pos - a.pos);
            float dd = qSqrt(dx * dx + dy * dy);
            float need = a.radius + b.radius;
            if (dd >= need || dd < 0.0001f) continue;
            float push = (need - dd) * 0.5f;
            QPointF n(dx / dd, dy / dd);
            float wa = a.isPlayer ? 0.25f : 1.f;
            float wb = b.isPlayer ? 0.25f : 1.f;
            b.pos += n * push * wb;
            a.pos -= n * push * wa;
        }
    }
}

Actor *GameView::nearestHostile(const QPointF &p, float maxDist, bool forPlayer)
{
    Actor *best = 0;
    float bd = maxDist;
    for (int i = 0; i < m_actors.size(); ++i) {
        Actor &a = m_actors[i];
        if (a.dead) continue;
        if (forPlayer) { if (a.isPlayer || a.isSummon) continue; }
        else { if (!a.isPlayer && !a.isSummon) continue; }
        float dd = distf(a.pos, p);
        if (dd < bd) { bd = dd; best = &a; }
    }
    return best;
}

QVector<Actor *> GameView::enemiesInRadius(const QPointF &p, float r)
{
    QVector<Actor *> out;
    for (int i = 0; i < m_actors.size(); ++i) {
        Actor &a = m_actors[i];
        if (a.dead || a.isPlayer || a.isSummon) continue;
        if (distf(a.pos, p) < r + a.radius) out.append(&a);
    }
    return out;
}

// ============================================================
//  ИИ ВРАГОВ
// ============================================================
void GameView::updateEnemies(float dt)
{
    Actor *pl = player();
    for (int i = 0; i < m_actors.size(); ++i) {
        Actor &a = m_actors[i];
        if (a.dead || a.isPlayer || a.isSummon) continue;

        a.animT += dt;
        if (a.hitFlash > 0) a.hitFlash -= dt * 4.f;
        if (a.attackAnim > 0) a.attackAnim = qMax(0.f, a.attackAnim - dt * 3.2f);
        if (a.spawnT < 1.f) a.spawnT = qMin(1.f, a.spawnT + dt * 2.8f);
        if (a.hitTilt > 0.f) a.hitTilt = qMax(0.f, a.hitTilt - dt * 4.5f);
        if (a.windup > 0.f) a.windup = qMax(0.f, a.windup - dt * 4.f);
        a.st.tick(dt);
        a.eliteAuraT += dt;

        // урон со временем
        if (a.st.burn > 0.f && a.st.burnDps > 0.f) {
            a.hp -= a.st.burnDps * dt;
            if (rng().chance(dt * 8.f)) burst(a.pos + QPointF(0, -a.radius), 1, C::ember, 1, 40, 2.4f, 0.4f);
        }
        if (a.st.poison > 0.f && a.st.poisonDps > 0.f) {
            a.hp -= a.st.poisonDps * dt;
            if (rng().chance(dt * 6.f)) burst(a.pos + QPointF(0, -a.radius), 1, C::poison, 0, 30, 2.4f, 0.5f);
        }
        if (a.hp <= 0.f) { killActor(a, m_playerId); continue; }

        // отбрасывание
        if (qAbs(a.knockX) > 0.5f || qAbs(a.knockY) > 0.5f) {
            moveActor(a, QPointF(a.knockX * dt * 6.f, a.knockY * dt * 6.f));
            a.knockX *= qMax(0.f, 1.f - dt * 8.f);
            a.knockY *= qMax(0.f, 1.f - dt * 8.f);
        }

        if (!pl || m_playerDead) continue;

        float dd = distf(a.pos, pl->pos);
        if (a.ai == Ai_Boss) { updateBoss(a, dt); continue; }
        // если игрок долго не может добить последних врагов - они идут к нему сами
        bool hunting = (m_huntT > 35.f) && !m_portalActive;
        if (!hunting && dd > a.detect * 1.6f) continue;   // спит
        float detectNow = hunting ? 1e9f : a.detect;
        Q_UNUSED(detectNow);

        QPointF toP = normv(pl->pos - a.pos);
        a.facing = float(qAtan2(toP.y(), toP.x()));
        a.rimAng = float(qAtan2(pl->pos.y() - a.pos.y(), pl->pos.x() - a.pos.x())) + float(M_PI);
        float spd = a.speed;
        if (a.st.chill > 0.f) spd *= 0.5f;
        if (a.st.stun > 0.f) spd = 0.f;
        if (a.atkTimer > 0.f) a.atkTimer -= dt;

        quint8 tile = m_dun.tileAtPx(a.pos);
        if (tile == T_Water) spd *= 0.8f;

        switch (a.ai) {
        case Ai_Chase:
        case Ai_Swarm: {
            if (dd > a.atkRange) moveActor(a, toP * spd * dt);
            else if (a.atkTimer <= 0.f) {
                a.atkTimer = a.atkCd;
                a.attackAnim = 1.f;
                a.attackKind = 0;
                ++a.attackSeq;
                damageActor(*pl, rng().frange(a.dmgMin, a.dmgMax), a.dmgType, a.id, false, 90.f, a.pos, false);
                burst(pl->pos, 6, dmgTypeColor(a.dmgType), 1, 90, 2.6f, 0.35f);
                // росчерк удара: когти/взмах/волна по архетипу тела
                int fk = fxKindForActor(a);
                spawnAttackFx(a.pos + toP * (a.radius * 0.9f) + QPointF(0, -a.radius * 0.5f),
                              a.facing, fk, dmgTypeColor(a.dmgType),
                              a.radius * (fk == 3 ? 2.6f : 2.3f), 0.26f, (a.attackSeq & 1) == 0);
            }
            break;
        }
        case Ai_Ranged: {
            float want = a.atkRange * 0.68f;
            QPointF mv(0, 0);
            if (dd > want + 40.f) mv = toP;
            else if (dd < want - 50.f) mv = -toP;
            else {
                a.strafeDir = (int(a.id) % 2) ? 1.f : -1.f;
                mv = QPointF(-toP.y() * a.strafeDir, toP.x() * a.strafeDir);
            }
            if (!mv.isNull()) moveActor(a, mv * spd * dt);
            if (a.atkTimer <= 0.f && (dd < a.atkRange || (hunting && dd < 900.f))) {
                a.atkTimer = a.atkCd;
                a.attackAnim = 1.f;
                a.attackKind = 2;   // лист существа: «shoot»
                ++a.attackSeq;
                int kind = a.dmgType == Dmg_Frost ? 2 : (a.dmgType == Dmg_Poison ? 4 : (a.dmgType == Dmg_Arcane ? 5 : 0));
                QColor col = dmgTypeColor(a.dmgType);
                fireProjectile(a.pos + toP * a.radius, toP, rng().frange(a.dmgMin, a.dmgMax), a.dmgType,
                               a.id, true, kind, a.projSpeed, 6.f, 0, 0, col, 1.f,
                               false, 0, a.dmgType == Dmg_Frost ? 0.3f : 0.f,
                               a.dmgType == Dmg_Poison ? a.dmgMax * 0.3f : 0.f, Gl_None);
            }
            break;
        }
        case Ai_Charger: {
            if (a.aiState == 0) {
                if (dd < 320.f && dd > 60.f && a.atkTimer <= 0.f) {
                    a.aiState = 1;
                    a.aiTimer = 0.55f;
                    a.aiPoint = pl->pos;
                    Telegraph t;
                    t.kind = 1; t.pos = a.pos; t.pos2 = a.pos + toP * 340.f; t.width = a.radius * 2.4f;
                    t.dur = 0.55f; t.c = C::bad; t.dmg = 0; t.owner = -1;
                    m_tels.append(t);
                } else if (dd > a.atkRange) moveActor(a, toP * spd * dt);
                if (dd <= a.atkRange && a.atkTimer <= 0.f) {
                    a.atkTimer = a.atkCd;
                    a.attackAnim = 1.f;
                    a.attackKind = 3;   // лист существа: «attack2» - силовой приём
                    ++a.attackSeq;
                    damageActor(*pl, rng().frange(a.dmgMin, a.dmgMax), a.dmgType, a.id, false, 140.f, a.pos, false);
                    spawnAttackFx(a.pos + toP * (a.radius * 0.9f) + QPointF(0, -a.radius * 0.5f),
                                  a.facing, fxKindForActor(a), dmgTypeColor(a.dmgType),
                                  a.radius * 2.4f, 0.28f, (a.attackSeq & 1) == 0);
                }
            } else if (a.aiState == 1) {
                a.aiTimer -= dt;
                a.windup = clampf(1.f - a.aiTimer / 0.55f, 0.f, 1.f);
                if (a.aiTimer <= 0.f) {
                    a.aiState = 2;
                    a.aiTimer = 0.55f;
                    a.aiPoint = normv(a.aiPoint - a.pos);
                    a.windup = 0.f;
                    Audio::inst().play(Sfx_Swing);
                }
            } else {
                a.aiTimer -= dt;
                moveActor(a, a.aiPoint * spd * 4.2f * dt);
                if (rng().chance(dt * 20.f)) burst(a.pos, 2, dmgTypeColor(a.dmgType), 1, 60, 3.f, 0.4f);
                if (distf(a.pos, pl->pos) < a.radius + pl->radius + 6.f) {
                    damageActor(*pl, rng().frange(a.dmgMin, a.dmgMax) * 1.5f, a.dmgType, a.id, false, 320.f, a.pos, false);
                    shake(4.f);
                    // таран: ударная волна на земле
                    spawnAttackFx(a.pos, a.facing, 3, dmgTypeColor(a.dmgType), a.radius * 2.8f, 0.4f);
                    a.aiState = 0;
                    a.atkTimer = a.atkCd * 1.4f;
                }
                if (a.aiTimer <= 0.f) { a.aiState = 0; a.atkTimer = a.atkCd; }
            }
            break;
        }
        case Ai_Caster: {
            float want = a.atkRange * 0.7f;
            if (dd < want - 60.f) moveActor(a, -toP * spd * dt);
            else if (dd > want + 80.f) moveActor(a, toP * spd * dt);
            else {
                a.strafeDir = (int(a.id) % 2) ? 1.f : -1.f;
                moveActor(a, QPointF(-toP.y(), toP.x()) * a.strafeDir * spd * 0.6f * dt);
            }
            if (a.aiState == 0 && a.atkTimer <= 0.f && (dd < a.detect || hunting)) {
                a.aiState = 1;
                a.aiTimer = 1.0f;
                a.aiPoint = pl->pos;
                a.aiParam = 70.f + a.radius;
                Telegraph t;
                t.kind = 0; t.pos = a.aiPoint; t.radius = a.aiParam; t.dur = 1.0f;
                t.c = dmgTypeColor(a.dmgType); t.dmg = rng().frange(a.dmgMin, a.dmgMax) * 1.2f;
                t.type = a.dmgType; t.owner = a.id;
                m_tels.append(t);
                Audio::inst().play(Sfx_Cast);
                // круг колдовства под ногами - виден всю подготовку
                spawnAttackFx(a.pos, 0.f, 5, dmgTypeColor(a.dmgType), a.radius * 2.4f, 0.95f);
                a.attackAnim = 1.f;
                a.attackKind = 1;
            } else if (a.aiState == 1) {
                a.aiTimer -= dt;
                a.windup = clampf(1.f - a.aiTimer / 1.0f, 0.f, 1.f);
                a.attackKind = 1;
                if (a.aiTimer <= 0.f) {
                    a.aiState = 0; a.atkTimer = a.atkCd; a.attackAnim = 1.f; a.windup = 0.f;
                    spawnAttackFx(a.pos + normv(a.aiPoint - a.pos) * a.radius, a.facing, 5,
                                  dmgTypeColor(a.dmgType), a.radius * 1.9f, 0.4f);
                }
            }
            break;
        }
        case Ai_Summoner: {
            if (dd < 260.f) moveActor(a, -toP * spd * dt);
            else if (dd > 420.f || hunting) moveActor(a, toP * spd * dt);
            a.summonTimer -= dt;
            // жрец-некромант: не забрасывает стрелбами, а поднимает свежих мертвецов;
            // выстрел - лишь редкая "прожарка" с расстояния (урон снижен реворком дефа)
            if (a.atkTimer <= 0.f && dd < 340.f && dd > a.radius * 3.f && rng().chance(0.5f)) {
                a.atkTimer = a.atkCd;
                a.attackAnim = 1.f;
                a.attackKind = 1;   // лист существа: «cast»
                ++a.attackSeq;
                fireProjectile(a.pos + toP * a.radius, toP, rng().frange(a.dmgMin, a.dmgMax), a.dmgType,
                               a.id, true, a.dmgType == Dmg_Arcane ? 5 : 4, a.projSpeed, 7.f, 0, 0,
                               dmgTypeColor(a.dmgType), 1.1f, false, 0, 0.f, a.dmgMax * 0.25f, Gl_None);
            }
            if (a.summonTimer <= 0.f) {
                // лимит "своих" прислужников жреца
                int alive = 0;
                for (int k = 0; k < m_actors.size(); ++k)
                    if (!m_actors[k].dead && !m_actors[k].isPlayer && m_actors[k].owner == a.id) ++alive;
                bool raised = false;
                if (alive < 3) {
                    // ищем ближайший свежий труп обычного моба (не босса, не прислужника)
                    int best = -1; float bd = 300.f;
                    for (int k = 0; k < m_actors.size(); ++k) {
                        const Actor &c = m_actors[k];
                        if (!c.dead || c.isPlayer || c.isSummon || c.elite || c.boss) continue;
                        if (c.deathT > 0.5f) continue;      // труп ещё "тёплый"
                        float d2 = distf(a.pos, c.pos);
                        if (d2 < bd) { bd = d2; best = k; }
                    }
                    if (best >= 0) {
                        const Actor corpse = m_actors.at(best);
                        PendingSpawn ps;
                        ps.summon = true;         // поднят жрецом: будет драться с его хозяевами
                        ps.defId = corpse.defId;  // воскрешаем того же моба
                        ps.pos = corpse.pos;
                        ps.elite = false;
                        ps.levelDelta = 0;
                        ps.raiserId = a.id;
                        m_pending.append(ps);
                        ring(corpse.pos, 16, C::arcane, 48.f);
                        burst(corpse.pos, 10, C::arcane, 2, 90, 3.f, 0.9f);
                        ftext(corpse.pos + QPointF(0, -30), QStringLiteral("восстань!"), C::arcane, false, 3, 12.f);
                        raised = true;
                    }
                }
                if (!raised && a.summonId >= 0 && alive < 4) {
                    // трупов рядом нет - зовёт служителя из бездны
                    for (int s = 0; s < a.summonCount; ++s) {
                        PendingSpawn ps;
                        ps.summon = false;
                        ps.defId = a.summonId;
                        ps.pos = a.pos + QPointF(rng().frange(-50, 50), rng().frange(-50, 50));
                        ps.elite = false;
                        ps.levelDelta = 0;
                        m_pending.append(ps);
                    }
                    ring(a.pos, 18, dmgTypeColor(a.dmgType), 54.f);
                    raised = true;
                }
                if (raised) {
                    Audio::inst().play(Sfx_Cast);
                    a.attackAnim = 1.f;
                    a.attackKind = 4;   // собственная анимация «raise»: посох вниз, рука из земли
                    spawnAttackFx(a.pos, 0.f, 5, C::arcane, a.radius * 2.8f, 0.6f);
                }
                a.summonTimer = raised ? 5.5f : 2.5f;   // неудачная попытка - быстро оглядеться снова
            }
            break;
        }
        case Ai_Tank: {
            if (dd > a.atkRange) moveActor(a, toP * spd * dt);
            else if (a.atkTimer <= 0.f) {
                a.atkTimer = a.atkCd;
                a.attackAnim = 1.f;
                a.attackKind = 3;   // лист существа: «attack2» - тяжёлый удар
                ++a.attackSeq;
                damageActor(*pl, rng().frange(a.dmgMin, a.dmgMax) * 1.25f, a.dmgType, a.id, false, 260.f, a.pos, false);
                shake(3.f);
                burst(pl->pos, 10, dmgTypeColor(a.dmgType), 3, 140, 3.4f, 0.5f);
                spawnAttackFx(a.pos + toP * (a.radius * 0.8f), a.facing, 3,
                              dmgTypeColor(a.dmgType), a.radius * 3.0f, 0.42f);
            }
            // аура
            if (dd < a.radius * 3.2f && rng().chance(dt * 1.4f))
                damageActor(*pl, (a.dmgMin + a.dmgMax) * 0.16f, a.dmgType, a.id, false, 0.f, a.pos, false);
            break;
        }
        case Ai_Turret: {
            if (a.atkTimer <= 0.f && (dd < a.atkRange || hunting)) {
                a.atkTimer = a.atkCd;
                a.attackAnim = 1.f;
                a.attackKind = 2;   // лист существа: «shoot» - залп
                ++a.attackSeq;
                for (int s = 0; s < 3; ++s) {
                    float ang = float(qAtan2(toP.y(), toP.x())) + rng().frange(-0.16f, 0.16f);
                    QPointF rd(qCos(ang), qSin(ang));
                    fireProjectile(a.pos + rd * a.radius, rd, rng().frange(a.dmgMin, a.dmgMax), a.dmgType,
                                   a.id, true, 4, a.projSpeed, 6.f, 0, 0, dmgTypeColor(a.dmgType), 1.f,
                                   false, 0, 0.f, a.dmgMax * 0.2f, Gl_None);
                }
            }
            break;
        }
        default:
            if (dd > a.atkRange) moveActor(a, toP * spd * dt);
            break;
        }

        // элитные ауры
        if (a.elite) {
            for (int k = 0; k < a.eliteAffixes.size(); ++k) {
                int af = a.eliteAffixes.at(k);
                if (af == 0 && rng().chance(dt * 1.1f)) {
                    Zone z;
                    z.pos = a.pos; z.radius = 46.f; z.life = z.maxLife = 2.4f; z.tickRate = 0.5f;
                    z.dps = a.dmgMax * 0.35f / 0.5f; z.type = Dmg_Fire; z.owner = a.id; z.hostile = true;
                    z.kind = 0; z.c = C::ember;
                    m_zones.append(z);
                } else if (af == 1 && dd < 150.f && rng().chance(dt * 0.9f)) {
                    pl->st.chill = qMax(pl->st.chill, 2.0f);
                    ring(a.pos, 14, C::frost, 150.f);
                } else if (af == 2 && a.atkTimer <= 0.f && dd < 320.f && rng().chance(dt * 0.6f)) {
                    fireProjectile(a.pos, toP, a.dmgMax * 0.7f, Dmg_Lightning, a.id, true, 3, 520.f, 7.f,
                                   0, 2, C::lightning, 1.1f, false, 0, 0.f, 0.f, Gl_None);
                }
            }
        }
    }
}

// ============================================================
//  БОССЫ
// ============================================================
enum BossMove { BM_Slam = 0, BM_Radial, BM_Spiral, BM_Fan, BM_Charge, BM_Summon,
                BM_Zones, BM_Teleport, BM_Laser, BM_Meteor, BM_Nova };

static const int g_bossMoves[5][3][6] = {
    { { BM_Charge, BM_Fan, BM_Slam, BM_Summon, -1, -1 },
      { BM_Charge, BM_Radial, BM_Meteor, BM_Slam, BM_Zones, -1 },
      { BM_Spiral, BM_Radial, BM_Charge, BM_Meteor, BM_Nova, BM_Zones } },
    { { BM_Summon, BM_Fan, BM_Zones, BM_Slam, -1, -1 },
      { BM_Summon, BM_Radial, BM_Zones, BM_Charge, BM_Fan, -1 },
      { BM_Spiral, BM_Summon, BM_Zones, BM_Nova, BM_Radial, BM_Charge } },
    { { BM_Fan, BM_Teleport, BM_Zones, BM_Nova, -1, -1 },
      { BM_Radial, BM_Teleport, BM_Laser, BM_Zones, BM_Fan, -1 },
      { BM_Spiral, BM_Laser, BM_Teleport, BM_Nova, BM_Meteor, BM_Radial } },
    { { BM_Slam, BM_Charge, BM_Summon, BM_Zones, -1, -1 },
      { BM_Slam, BM_Nova, BM_Zones, BM_Summon, BM_Charge, -1 },
      { BM_Spiral, BM_Slam, BM_Nova, BM_Zones, BM_Charge, BM_Summon } },
    { { BM_Radial, BM_Meteor, BM_Teleport, BM_Laser, -1, -1 },
      { BM_Spiral, BM_Meteor, BM_Laser, BM_Summon, BM_Radial, -1 },
      { BM_Spiral, BM_Laser, BM_Meteor, BM_Nova, BM_Teleport, BM_Summon } }
};

void GameView::updateBoss(Actor &a, float dt)
{
    Actor *pl = player();
    if (!pl || m_playerDead) return;

    const EnemyDef *e = Data::enemy(a.defId);
    int phases = e ? e->phases : 1;
    int newPhase = 1 + int((1.f - clampf(a.hp / qMax(1.f, a.maxHp), 0.f, 1.f)) * float(phases - 1));
    if (newPhase > a.phase) {
        a.phase = newPhase;
        emit bossHealthChanged(a.hp, a.maxHp, a.phase);
        Audio::inst().play(Sfx_Boss);
        shake(14.f);
        flashScreen(dmgTypeColor(a.dmgType), 0.3f);
        ring(a.pos, 40, dmgTypeColor(a.dmgType), a.radius * 5.f);
        emit logMsg(QStringLiteral("%1 в ярости! Фаза %2.").arg(a.bossTitle).arg(a.phase), C::bad);
        // сброс атаки при смене фазы
        a.aiState = 0;
        a.patternT = 1.2f;
        a.speed *= 1.12f;
        a.atkCd *= 0.88f;
    }

    if (a.hitFlash > 0) a.hitFlash -= dt * 4.f;
    a.animT += dt;
    a.st.tick(dt);
    if (a.st.burn > 0.f && a.st.burnDps > 0.f) a.hp -= a.st.burnDps * dt;
    if (a.st.poison > 0.f && a.st.poisonDps > 0.f) a.hp -= a.st.poisonDps * dt;
    if (a.hp <= 0.f) { killActor(a, m_playerId); return; }
    m_bossHp = a.hp;
    m_bossMaxHp = a.maxHp;
    m_bossPhase = a.phase;

    float dd = distf(a.pos, pl->pos);
    QPointF toP = normv(pl->pos - a.pos);
    a.facing = float(qAtan2(toP.y(), toP.x()));
    float spd = a.speed * (a.st.chill > 0.f ? 0.6f : 1.f);
    a.patternT -= dt;
    if (a.atkTimer > 0.f) a.atkTimer -= dt;
    if (a.attackAnim > 0.f) a.attackAnim = qMax(0.f, a.attackAnim - dt * 2.4f);

    if (!m_bossIntroDone && dd < 560.f) {
        m_bossIntroDone = true;
        m_bossIntroT = 2.6f;
        m_timeScale = 0.42f;
        shake(7.f);
        Audio::inst().play(Sfx_Boss);
        emit logMsg(QStringLiteral("%1 преграждает путь!").arg(a.bossTitle), C::bad);
    }
    int ps = qBound(0, a.patternSet, 4);
    int ph = qBound(0, a.phase - 1, 2);

    QColor bc = dmgTypeColor(a.dmgType);

    if (a.aiState == 0) {
        // обычное поведение: держится на средней дистанции, бьёт вблизи
        float want = 150.f + a.radius;
        if (dd > want + 40.f) moveActor(a, toP * spd * dt);
        else if (dd < want - 90.f) moveActor(a, -toP * spd * 0.7f * dt);
        else moveActor(a, QPointF(-toP.y(), toP.x()) * spd * 0.6f * dt);

        if (dd < a.atkRange + 10.f && a.atkTimer <= 0.f) {
            a.atkTimer = a.atkCd;
            a.attackAnim = 1.f;
            a.attackKind = 3;   // тяжёлый удар (attack2 листа)
            ++a.attackSeq;
            damageActor(*pl, rng().frange(a.dmgMin, a.dmgMax), a.dmgType, a.id, false, 240.f, a.pos, false);
            burst(pl->pos, 12, bc, 1, 140, 3.6f, 0.5f);
            shake(4.f);
            int fk = fxKindForActor(a);
            spawnAttackFx(a.pos + toP * (a.radius * 0.8f) + QPointF(0, -a.radius * 0.4f),
                          a.facing, fk, bc, a.radius * (fk == 3 ? 3.2f : 2.8f), 0.34f,
                          (a.attackSeq & 1) == 0);
        }
        if (a.patternT <= 0.f) {
            // выбор атаки
            QVector<int> avail;
            for (int i = 0; i < 6; ++i) if (g_bossMoves[ps][ph][i] >= 0) avail.append(g_bossMoves[ps][ph][i]);
            if (avail.isEmpty()) { a.patternT = 2.f; return; }
            int mv = avail.at(rng().pick(avail.size()));
            a.aiParam = float(mv);
            a.aiState = 1;
            a.aiTimer = 0.85f;
            Audio::inst().play(Sfx_Boss);
            // изготовление: лист босса играет нужную анимацию уже во время ветрапа
            a.attackAnim = 1.f;
            a.attackKind = (mv == BM_Slam || mv == BM_Nova || mv == BM_Charge) ? 3
                         : (mv == BM_Fan ? 2 : 1);

            switch (mv) {
            case BM_Slam: {
                Telegraph t;
                t.kind = 0; t.pos = pl->pos; t.radius = 120.f + a.radius; t.dur = 0.85f;
                t.c = bc; t.dmg = (a.dmgMin + a.dmgMax) * 0.9f; t.type = a.dmgType; t.owner = a.id;
                m_tels.append(t);
                a.aiTimer = 0.85f;
                break;
            }
            case BM_Meteor: {
                for (int i = 0; i < 3 + a.phase; ++i) {
                    Telegraph t;
                    t.kind = 0;
                    t.pos = pl->pos + QPointF(rng().frange(-140, 140), rng().frange(-110, 110));
                    t.radius = 84.f;
                    t.dur = 0.85f + 0.42f * float(i);
                    t.c = C::ember; t.dmg = (a.dmgMin + a.dmgMax) * 0.7f; t.type = Dmg_Fire; t.owner = a.id;
                    m_tels.append(t);
                }
                a.aiTimer = 0.85f + 0.42f * float(2 + a.phase);
                break;
            }
            case BM_Nova: {
                Telegraph t;
                t.kind = 0; t.pos = a.pos; t.radius = 300.f; t.dur = 1.1f;
                t.c = bc; t.dmg = (a.dmgMin + a.dmgMax) * 0.8f; t.type = a.dmgType; t.owner = a.id;
                m_tels.append(t);
                a.aiTimer = 1.1f;
                break;
            }
            case BM_Laser: {
                a.aiTimer = 0.5f;
                break;
            }
            default:
                a.aiTimer = (mv == BM_Charge) ? 0.6f : 0.5f;
                break;
            }
        }
        return;
    }

    if (a.aiState == 1) {
        // подготовка
        a.aiTimer -= dt;
        if (rng().chance(dt * 20.f)) burst(a.pos + QPointF(0, -a.radius), 2, bc, 1, 90, 3.f, 0.5f);
        if (a.aiTimer > 0.f) return;
        int mv = int(a.aiParam);
        // общий «выброс» в момент исполнения приёма
        a.attackAnim = 1.f;
        switch (mv) {
        case BM_Radial: {
            a.attackKind = 1;
            spawnAttackFx(a.pos, 0.f, 5, bc, a.radius * 3.4f, 0.6f);
            int n = 14 + a.phase * 4;
            float base = rng().frange(0.f, 6.28f);
            for (int i = 0; i < n; ++i) {
                float ang = base + float(i) * 6.2831853f / float(n);
                QPointF rd(qCos(ang), qSin(ang));
                fireProjectile(a.pos + rd * a.radius, rd, (a.dmgMin + a.dmgMax) * 0.42f, a.dmgType,
                               a.id, true, a.dmgType == Dmg_Frost ? 2 : (a.dmgType == Dmg_Fire ? 1 : 5),
                               260.f + a.phase * 22.f, 9.f, 0, 0, bc, 1.2f, false, 0,
                               a.dmgType == Dmg_Frost ? 0.35f : 0.f, 0.f, Gl_None);
            }
            Audio::inst().play(Sfx_Cast);
            a.aiState = 0; a.patternT = 3.2f - 0.4f * a.phase;
            break;
        }
        case BM_Fan: {
            a.attackKind = 1;
            spawnAttackFx(a.pos + QPointF(qCos(a.facing), qSin(a.facing)) * a.radius, a.facing, 5, bc, a.radius * 2.6f, 0.45f);
            int n = 5 + a.phase;
            for (int i = 0; i < n; ++i) {
                float ang = a.facing + (float(i) - float(n - 1) * 0.5f) * 0.17f;
                QPointF rd(qCos(ang), qSin(ang));
                fireProjectile(a.pos + rd * a.radius, rd, (a.dmgMin + a.dmgMax) * 0.5f, a.dmgType,
                               a.id, true, a.dmgType == Dmg_Poison ? 4 : (a.dmgType == Dmg_Frost ? 2 : 1),
                               380.f, 8.f, 0, 0, bc, 1.1f, false, 0, 0.f,
                               a.dmgType == Dmg_Poison ? a.dmgMax * 0.25f : 0.f, Gl_None);
            }
            Audio::inst().play(Sfx_Swing);
            a.aiState = 0; a.patternT = 2.4f - 0.3f * a.phase;
            break;
        }
        case BM_Summon: {
            a.attackKind = 1;
            spawnAttackFx(a.pos, 0.f, 5, bc, a.radius * 3.6f, 0.8f);
            int sid = a.summonId >= 0 ? a.summonId : (a.defId - 1);
            int aliveMinions = 0;
            for (int k = 0; k < m_actors.size(); ++k)
                if (!m_actors[k].dead && !m_actors[k].isPlayer && m_actors[k].owner == a.id) ++aliveMinions;
            int n = qMax(0, a.summonCount + (a.phase > 1 ? 1 : 0) - aliveMinions);
            n = qMin(n, 3);
            for (int i = 0; i < n; ++i) {
                PendingSpawn psp;
                psp.summon = false;
                psp.defId = qMax(0, sid);
                psp.pos = a.pos + QPointF(rng().frange(-90, 90), rng().frange(-90, 90));
                psp.elite = false;
                psp.levelDelta = 0;
                m_pending.append(psp);
            }
            ring(a.pos, 26, bc, 90.f);
            Audio::inst().play(Sfx_Cast);
            a.aiState = 0; a.patternT = 8.0f;
            break;
        }
        case BM_Zones: {
            int n = 3 + a.phase * 2;
            for (int i = 0; i < n; ++i) {
                Zone z;
                z.pos = pl->pos + QPointF(rng().frange(-190, 190), rng().frange(-150, 150));
                if (blockedAt(z.pos, 30.f)) z.pos = a.pos + QPointF(rng().frange(-120, 120), rng().frange(-120, 120));
                z.radius = 66.f;
                z.life = z.maxLife = 5.0f;
                z.tickRate = 0.5f;
                z.dps = (a.dmgMin + a.dmgMax) * 0.16f / 0.5f;
                z.type = a.dmgType;
                z.owner = a.id;
                z.hostile = true;
                z.kind = a.dmgType == Dmg_Poison ? 1 : (a.dmgType == Dmg_Frost ? 2 : 0);
                z.c = bc;
                m_zones.append(z);
            }
            Audio::inst().play(Sfx_Poison);
            a.aiState = 0; a.patternT = 3.6f;
            break;
        }
        case BM_Charge: {
            a.aiState = 2;
            a.aiTimer = 0.75f;
            a.aiPoint = normv(pl->pos - a.pos);
            a.attackKind = 3;
            Audio::inst().play(Sfx_Roll);
            spawnAttackFx(a.pos, a.facing, 3, bc, a.radius * 2.6f, 0.4f);
            break;
        }
        case BM_Spiral: {
            a.aiState = 2;
            a.aiTimer = 2.6f;
            a.aiPoint = QPointF(qCos(a.facing), qSin(a.facing));
            a.strafeDir = rng().chance(0.5f) ? 1.f : -1.f;
            a.attackKind = 1;
            spawnAttackFx(a.pos, 0.f, 5, bc, a.radius * 3.2f, 0.7f);
            break;
        }
        case BM_Laser: {
            a.aiState = 2;
            a.aiTimer = 2.4f;
            a.aiPoint = QPointF(0, 0); // счётчик выстрелов
            a.attackKind = 1;
            spawnAttackFx(a.pos, 0.f, 5, bc, a.radius * 3.0f, 0.7f);
            break;
        }
        case BM_Teleport: {
            burst(a.pos, 30, bc, 2, 160, 5.f, 0.7f);
            a.attackKind = 1;
            spawnAttackFx(a.pos, 0.f, 5, bc, a.radius * 3.0f, 0.5f);
            QPointF np = pl->pos + QPointF(rng().frange(-160, 160), rng().frange(-160, 160));
            for (float k = 1.f; k >= 0.2f; k -= 0.1f) {
                QPointF tp = pl->pos + normv(np - pl->pos) * (distf(np, pl->pos) * k);
                if (!blockedAt(tp, a.radius)) { np = tp; break; }
            }
            a.pos = np;
            m_dun.collideCircle(a.pos, a.radius);
            burst(a.pos, 30, bc, 2, 160, 5.f, 0.7f);
            spawnAttackFx(a.pos, 0.f, 5, bc, a.radius * 3.0f, 0.5f);
            Audio::inst().play(Sfx_Portal);
            // взрыв при появлении
            for (int i = 0; i < m_actors.size(); ++i) {
                Actor &o = m_actors[i];
                if (o.dead || !o.isPlayer) continue;
                if (distf(o.pos, a.pos) < 140.f)
                    damageActor(o, (a.dmgMin + a.dmgMax) * 0.5f, a.dmgType, a.id, false, 200.f, a.pos, false);
            }
            ring(a.pos, 24, bc, 140.f);
            a.aiState = 0; a.patternT = 2.2f;
            break;
        }
        case BM_Slam: {
            a.aiState = 0; a.patternT = 2.8f - 0.3f * a.phase;
            a.attackKind = 3;
            a.attackAnim = 1.f;
            shake(12.f);
            Audio::inst().play(Sfx_Explosion);
            spawnAttackFx(a.pos, 0.f, 3, bc, a.radius * 4.5f, 0.5f);
            for (int i = 0; i < 3; ++i) {
                Zone z;
                z.pos = a.pos;
                z.radius = 150.f + i * 90.f;
                z.life = z.maxLife = 0.35f;
                z.delay = 0.10f * float(i);
                z.tickRate = 0.35f;
                z.dps = (a.dmgMin + a.dmgMax) * 0.4f / 0.35f;
                z.type = a.dmgType;
                z.owner = a.id;
                z.hostile = true;
                z.kind = 3;
                z.c = bc;
                m_zones.append(z);
            }
            burst(a.pos, 40, bc, 3, 260, 6.f, 1.0f);
            break;
        }
        case BM_Nova: {
            a.aiState = 0; a.patternT = 3.4f;
            a.attackKind = 3;
            a.attackAnim = 1.f;
            shake(10.f);
            Audio::inst().play(Sfx_Explosion);
            spawnAttackFx(a.pos, 0.f, 3, bc, a.radius * 5.5f, 0.55f);
            ring(a.pos, 44, bc, 300.f);
            burst(a.pos, 50, bc, 1, 300, 5.f, 1.1f);
            break;
        }
        default:
            a.aiState = 0; a.patternT = 2.5f;
            break;
        }
        return;
    }

    // aiState == 2: длительное исполнение
    if (a.aiState == 2) {
        int mv = int(a.aiParam);
        a.aiTimer -= dt;
        if (mv == BM_Charge) {
            moveActor(a, a.aiPoint * spd * 5.2f * dt);
            if (rng().chance(dt * 30.f)) burst(a.pos, 3, bc, 1, 90, 4.f, 0.5f);
            if (a.dmgType == Dmg_Fire && rng().chance(dt * 6.f)) {
                Zone z;
                z.pos = a.pos; z.radius = 44.f; z.life = z.maxLife = 2.2f; z.tickRate = 0.5f;
                z.dps = a.dmgMax * 0.2f / 0.5f; z.type = Dmg_Fire; z.owner = a.id; z.hostile = true;
                z.kind = 0; z.c = C::ember;
                m_zones.append(z);
            }
            if (distf(a.pos, pl->pos) < a.radius + pl->radius + 8.f) {
                damageActor(*pl, (a.dmgMin + a.dmgMax) * 0.85f, a.dmgType, a.id, false, 380.f, a.pos, false);
                shake(8.f);
                spawnAttackFx(a.pos, a.facing, 3, bc, a.radius * 3.4f, 0.45f);
            }
        } else if (mv == BM_Spiral) {
            float ang = float(qAtan2(a.aiPoint.y(), a.aiPoint.x())) + a.strafeDir * (2.6f - a.aiTimer) * 3.4f;
            int arms = 2 + a.phase;
            for (int i = 0; i < arms; ++i) {
                float aa = ang + float(i) * 6.2831853f / float(arms);
                QPointF rd(qCos(aa), qSin(aa));
                fireProjectile(a.pos + rd * a.radius, rd, (a.dmgMin + a.dmgMax) * 0.30f, a.dmgType,
                               a.id, true, a.dmgType == Dmg_Arcane ? 5 : 1, 230.f, 8.f, 0, 0, bc, 1.1f,
                               false, 0, 0.f, 0.f, Gl_None);
            }
            if (rng().chance(dt * 8.f)) Audio::inst().play(Sfx_Cast);
        } else if (mv == BM_Laser) {
            a.aiPoint.setX(a.aiPoint.x() + dt);   // накопитель времени
            float beamEvery = 0.62f;
            if (a.aiPoint.x() >= beamEvery) {
                a.aiPoint.setX(0.f);
                QPointF rd = normv(pl->pos - a.pos);
                Telegraph t;
                t.kind = 1; t.pos = a.pos; t.pos2 = a.pos + rd * 1100.f; t.width = 52.f;
                t.dur = 0.42f; t.c = bc; t.dmg = (a.dmgMin + a.dmgMax) * 0.65f; t.type = a.dmgType; t.owner = a.id;
                m_tels.append(t);
                Audio::inst().play(Sfx_Shock);
            }
        }
        if (a.aiTimer <= 0.f) { a.aiState = 0; a.patternT = 2.6f - 0.25f * a.phase; }
    }
}

// ============================================================
//  ПРИСЛУЖНИКИ ИГРОКА
// ============================================================
void GameView::updateSummons(float dt)
{
    Actor *pl = player();
    for (int i = 0; i < m_actors.size(); ++i) {
        Actor &a = m_actors[i];
        if (a.dead || !a.isSummon) continue;
        a.animT += dt;
        if (a.hitFlash > 0) a.hitFlash -= dt * 4.f;
        if (a.attackAnim > 0) a.attackAnim = qMax(0.f, a.attackAnim - dt * 3.f);
        if (a.spawnT < 1.f) a.spawnT = qMin(1.f, a.spawnT + dt * 3.2f);
        if (a.hitTilt > 0.f) a.hitTilt = qMax(0.f, a.hitTilt - dt * 4.5f);
        a.st.tick(dt);
        if (a.lifeTime > 0.f) {
            a.lifeTime -= dt;
            if (a.lifeTime <= 0.f) {
                a.dead = true; a.deathT = 0.6f;
                burst(a.pos, 12, C::arcane, 2, 90, 3.4f, 0.6f);
                continue;
            }
        }
        bool friendly = (a.owner == m_playerId);
        if (!pl) continue;
        Actor *tgt = nearestHostile(a.pos, a.detect, friendly);
        if (!tgt) {
            // следуем за хозяином
            float dd = distf(a.pos, pl->pos);
            if (dd > 90.f) {
                QPointF to = normv(pl->pos - a.pos);
                moveActor(a, to * a.speed * dt);
                a.facing = float(qAtan2(to.y(), to.x()));
            }
            continue;
        }
        float dd = distf(a.pos, tgt->pos);
        QPointF to = normv(tgt->pos - a.pos);
        a.facing = float(qAtan2(to.y(), to.x()));
        a.rimAng = float(qAtan2(pl->pos.y() - a.pos.y(), pl->pos.x() - a.pos.x())) + float(M_PI);
        if (a.atkTimer > 0.f) a.atkTimer -= dt;
        if (dd > a.atkRange) moveActor(a, to * a.speed * dt);
        else if (a.atkTimer <= 0.f) {
            a.atkTimer = a.atkCd;
            a.attackAnim = 1.f;
            a.attackKind = 0;
            ++a.attackSeq;
            if (friendly) damageActor(*tgt, rng().frange(a.dmgMin, a.dmgMax), a.dmgType, a.id, true, 60.f, a.pos, true);
            else damageActor(*pl, rng().frange(a.dmgMin, a.dmgMax), a.dmgType, a.id, false, 60.f, a.pos, false);
            burst(tgt->pos, 4, dmgTypeColor(a.dmgType), 1, 70, 2.4f, 0.3f);
            spawnAttackFx(a.pos + to * (a.radius * 0.9f) + QPointF(0, -a.radius * 0.5f),
                          a.facing, fxKindForActor(a), dmgTypeColor(a.dmgType),
                          a.radius * 2.2f, 0.24f, (a.attackSeq & 1) == 0);
        }
    }
}

// ============================================================
//  СНАРЯДЫ
// ============================================================
void GameView::updateProjectiles(float dt)
{
    Actor *pl = player();
    for (int i = m_projs.size() - 1; i >= 0; --i) {
        Proj &pr = m_projs[i];
        pr.life -= dt;
        if (pr.life <= 0.f) { m_projs.removeAt(i); continue; }
        QPointF step = pr.vel * dt;
        QPointF np = pr.pos + step;
        pr.spin += dt * 12.f;

        // след
        if (pr.kind == 1 && rng().chance(dt * 40.f)) burst(pr.pos, 1, pr.color, 2, 20, 3.4f, 0.4f);
        if (pr.kind == 4 && rng().chance(dt * 20.f)) burst(pr.pos, 1, pr.color, 0, 15, 2.4f, 0.5f);

        // попадание в стену
        if (m_dun.solidAtPx(np)) {
            if (pr.explodeOnHit) {
                Zone z;
                z.pos = pr.pos; z.radius = pr.explodeRadius; z.life = z.maxLife = 0.28f;
                z.tickRate = 0.28f; z.dps = pr.dmg * 0.8f / 0.28f; z.type = pr.type;
                z.owner = pr.owner; z.hostile = pr.hostile; z.kind = pr.type == Dmg_Fire ? 0 : 3; z.c = pr.color;
                m_zones.append(z);
                ring(pr.pos, 16, pr.color, pr.explodeRadius);
                shake(3.f);
                Audio::inst().play(Sfx_Explosion);
                stampDecal(pr.pos, 0, pr.explodeRadius * 0.8f, pr.color);
            } else {
                burst(pr.pos, 6, pr.color, 3, 90, 2.6f, 0.4f);
            }
            m_projs.removeAt(i);
            continue;
        }
        pr.pos = np;

        // столкновения
        bool removed = false;
        for (int k = 0; k < m_actors.size() && !removed; ++k) {
            Actor &a = m_actors[k];
            if (a.dead) continue;
            if (pr.hostile) { if (!a.isPlayer) continue; }
            else { if (a.isPlayer || a.isSummon) continue; }
            if (a.id == pr.owner) continue;
            if (pr.hit.contains(a.id)) continue;
            if (distf(a.pos + QPointF(0, -a.radius * 0.6f), pr.pos) > a.radius + pr.radius) continue;

            if (pr.kind == 7) { // метка
                a.st.mark = qMax(a.st.mark, 7.f);
                ftext(a.pos + QPointF(0, -a.radius * 2.f), QStringLiteral("МЕТКА"), C::blood, true, 3, 14.f);
                ring(a.pos, 14, C::blood, 40.f);
                Audio::inst().play(Sfx_Cast);
            } else {
                damageActor(a, pr.dmg, pr.type, pr.owner, !pr.hostile, 90.f, pr.pos, !pr.hostile);
                if (pr.slow > 0.f) a.st.chill = qMax(a.st.chill, 3.f);
                if (pr.dotDps > 0.f) { a.st.poison = qMax(a.st.poison, 4.f); a.st.poisonDps = qMax(a.st.poisonDps, pr.dotDps); }
            }

            if (pr.explodeOnHit) {
                Zone z;
                z.pos = pr.pos; z.radius = pr.explodeRadius; z.life = z.maxLife = 0.30f;
                z.tickRate = 0.30f; z.dps = pr.dmg * 0.75f / 0.30f; z.type = pr.type;
                z.owner = pr.owner; z.hostile = pr.hostile; z.kind = pr.type == Dmg_Fire ? 0 : 3; z.c = pr.color;
                m_zones.append(z);
                ring(pr.pos, 18, pr.color, pr.explodeRadius);
                shake(3.4f);
                Audio::inst().play(Sfx_Explosion);
                stampDecal(pr.pos, 0, pr.explodeRadius * 0.8f, pr.color);
            }
            // цепь
            if (pr.chains > 0) {
                Actor *next = 0;
                float bd = 260.f;
                for (int q = 0; q < m_actors.size(); ++q) {
                    Actor &o = m_actors[q];
                    if (o.dead || o.id == a.id || pr.hit.contains(o.id)) continue;
                    if (pr.hostile && !o.isPlayer) continue;
                    if (!pr.hostile && (o.isPlayer || o.isSummon)) continue;
                    float dd = distf(o.pos, pr.pos);
                    if (dd < bd) { bd = dd; next = &o; }
                }
                if (next) {
                    Telegraph b;
                    b.kind = 1; b.pos = pr.pos; b.pos2 = next->pos; b.width = 8.f; b.dur = 0.16f;
                    b.c = pr.color; b.dmg = 0; b.owner = -1;
                    m_tels.append(b);
                    pr.hit.append(a.id);
                    pr.pos = next->pos;
                    pr.vel = normv(next->pos - a.pos) * float(qSqrt(pr.vel.x() * pr.vel.x() + pr.vel.y() * pr.vel.y()));
                    pr.chains--;
                    pr.dmg *= 0.85f;
                    continue;
                }
            }
            if (pr.pierce > 0) {
                pr.pierce--;
                pr.hit.append(a.id);
                continue;
            }
            m_projs.removeAt(i);
            removed = true;
        }
        if (removed) continue;
        // вылет за пределы
        if (pr.pos.x() < -60 || pr.pos.y() < -60 || pr.pos.x() > m_dun.widthPx() + 60 || pr.pos.y() > m_dun.heightPx() + 60)
            m_projs.removeAt(i);
    }
    Q_UNUSED(pl);
}

// ============================================================
//  ЗОНЫ И ТЕЛЕГРАФЫ
// ============================================================
void GameView::updateZones(float dt)
{
    Actor *pl = player();
    for (int i = m_zones.size() - 1; i >= 0; --i) {
        Zone &z = m_zones[i];
        if (z.delay > 0.f) { z.delay -= dt; continue; }
        z.life -= dt;
        if (z.life <= 0.f) { m_zones.removeAt(i); continue; }
        if (z.follow >= 0) {
            Actor *f = findById(z.follow);
            if (f) z.pos = f->pos;
            else { m_zones.removeAt(i); continue; }
        }
        z.tickT -= dt;
        if (z.tickT > 0.f) continue;
        z.tickT = z.tickRate;

        if (z.hostile) {
            if (pl && !pl->dead && !m_playerDead && distf(pl->pos, z.pos) < z.radius + pl->radius * 0.5f) {
                if (z.dps < 0.f) healActor(*pl, -z.dps * z.tickRate);
                else damageActor(*pl, z.dps * z.tickRate, z.type, z.owner, false, 0.f, z.pos, false, true);
                if (z.slow > 0.f) pl->st.chill = qMax(pl->st.chill, 1.f);
            }
        } else {
            if (z.dps < 0.f) {
                if (pl && !pl->dead) healActor(*pl, -z.dps * z.tickRate);
            } else {
                for (int k = 0; k < m_actors.size(); ++k) {
                    Actor &a = m_actors[k];
                    if (a.dead || a.isPlayer || a.isSummon) continue;
                    if (distf(a.pos, z.pos) < z.radius + a.radius * 0.6f)
                        damageActor(a, z.dps * z.tickRate, z.type, z.owner, false, 0.f, z.pos, true, true);
                }
            }
        }
    }
}

void GameView::updateTelegraphs(float dt)
{
    Actor *pl = player();
    for (int i = m_tels.size() - 1; i >= 0; --i) {
        Telegraph &t = m_tels[i];
        t.t += dt;
        if (t.t < t.dur) continue;
        m_tels.removeAt(i);
        if (t.dmg <= 0.f) continue;   // чисто визуальный

        bool fromPlayer = (t.owner == m_playerId);
        if (t.kind == 0) {
            if (fromPlayer) {
                for (int k = 0; k < m_actors.size(); ++k) {
                    Actor &a = m_actors[k];
                    if (a.dead || a.isPlayer || a.isSummon) continue;
                    if (distf(a.pos, t.pos) < t.radius + a.radius)
                        damageActor(a, t.dmg, t.type, t.owner, true, 160.f, t.pos, true);
                }
            } else if (pl && !pl->dead && !m_playerDead) {
                if (distf(pl->pos, t.pos) < t.radius + pl->radius * 0.6f)
                    damageActor(*pl, t.dmg, t.type, t.owner, false, 180.f, t.pos, false);
            }
            ring(t.pos, 24, t.c, t.radius);
            burst(t.pos, 26, t.c, 1, 220, 4.6f, 0.8f);
            shake(t.radius > 200.f ? 9.f : 5.f);
            Audio::inst().play(Sfx_Explosion);
            stampDecal(t.pos, t.type == Dmg_Frost ? 4 : 0, t.radius * 0.85f, t.c);
        } else if (t.kind == 1) {
            QPointF d = t.pos2 - t.pos;
            float len = distf(t.pos2, t.pos);
            if (len < 1.f) continue;
            QPointF n = normv(d);
            QPointF perp(-n.y(), n.x());
            QVector<Actor *> victims;
            if (fromPlayer) {
                for (int k = 0; k < m_actors.size(); ++k) {
                    Actor &a = m_actors[k];
                    if (a.dead || a.isPlayer || a.isSummon) continue;
                    QPointF rel = a.pos - t.pos;
                    float along = float(rel.x() * n.x() + rel.y() * n.y());
                    float side = qAbs(float(rel.x() * perp.x() + rel.y() * perp.y()));
                    if (along > -20.f && along < len + 20.f && side < t.width * 0.5f + a.radius) victims.append(&a);
                }
            } else if (pl && !pl->dead && !m_playerDead) {
                QPointF rel = pl->pos - t.pos;
                float along = float(rel.x() * n.x() + rel.y() * n.y());
                float side = qAbs(float(rel.x() * perp.x() + rel.y() * perp.y()));
                if (along > -20.f && along < len + 20.f && side < t.width * 0.5f + pl->radius) victims.append(pl);
            }
            for (int k = 0; k < victims.size(); ++k)
                damageActor(*victims[k], t.dmg, t.type, t.owner, fromPlayer, 120.f, t.pos, fromPlayer);
            for (int k = 0; k < 18; ++k) {
                Part pt;
                float f = rng().unit();
                pt.pos = t.pos + n * (len * f) + perp * rng().frange(-t.width * 0.4f, t.width * 0.4f);
                pt.vel = perp * rng().frange(-60, 60) + n * rng().frange(-30, 30);
                pt.life = pt.maxLife = 0.5f;
                pt.size = 3.6f;
                pt.c = t.c;
                pt.kind = 1;
                m_parts.append(pt);
            }
            shake(6.f);
            Audio::inst().play(Sfx_Shock);
        }
    }
}

// ============================================================
//  ЧАСТИЦЫ / ТЕКСТ
// ============================================================
void GameView::updateParticles(float dt)
{
    for (int i = m_parts.size() - 1; i >= 0; --i) {
        Part &p = m_parts[i];
        p.life -= dt;
        if (p.life <= 0.f) { m_parts.removeAt(i); continue; }
        p.pos += p.vel * dt;
        p.vel += QPointF(0, p.grav * dt);
        p.vel *= (1.f - dt * 2.2f);
        p.rot += p.spin * dt;
    }
    if (m_parts.size() > 1400) m_parts.remove(0, m_parts.size() - 1400);

    for (int i = m_texts.size() - 1; i >= 0; --i) {
        FText &f = m_texts[i];
        f.life -= dt;
        if (f.life <= 0.f) { m_texts.removeAt(i); continue; }
        f.pos += QPointF(f.vx * dt, f.vy * dt);
        f.vy += 46.f * dt;
        f.vx *= (1.f - dt * 2.f);
    }
    if (m_texts.size() > 120) m_texts.remove(0, m_texts.size() - 120);
}

void GameView::spawnAmbient(float dt)
{
    if (m_quality == 0) return;
    if (m_playerDead) return;
    if (!rng().chance(dt * 8.f)) return;
    if (m_parts.size() > 420) return;
    Part pt;
    pt.pos = m_cam + QPointF(rng().frange(-20.f, float(width()) + 20.f), rng().frange(-20.f, float(height()) + 20.f));
    pt.grav = 0.f;
    pt.additive = true;
    switch (m_dun.biome) {
    case Bio_Ash:
    case Bio_Lava:
        pt.vel = QPointF(rng().frange(-7, 7), -rng().frange(9, 24));
        pt.c = QColor(255, 150, 60, 120);
        pt.size = rng().frange(1.0f, 2.0f);
        pt.life = pt.maxLife = rng().frange(2.5f, 4.5f);
        pt.kind = 0;
        break;
    case Bio_Ember:
        pt.vel = QPointF(rng().frange(-16, 16), rng().frange(12, 26));
        pt.c = QColor(150, 176, 84, 110);
        pt.size = rng().frange(1.6f, 2.6f);
        pt.life = pt.maxLife = rng().frange(3.f, 5.f);
        pt.kind = 3;
        pt.spin = rng().frange(-3, 3);
        pt.additive = false;
        break;
    case Bio_Frost:
        pt.vel = QPointF(rng().frange(-9, 9), rng().frange(16, 34));
        pt.c = QColor(226, 240, 255, 130);
        pt.size = rng().frange(1.0f, 2.2f);
        pt.life = pt.maxLife = rng().frange(3.f, 5.f);
        pt.kind = 0;
        break;
    case Bio_Crystal:
        pt.vel = QPointF(rng().frange(-4, 4), rng().frange(-8, -2));
        pt.c = QColor(196, 156, 255, 110);
        pt.size = rng().frange(1.0f, 1.8f);
        pt.life = pt.maxLife = rng().frange(2.f, 4.f);
        pt.kind = 0;
        break;
    case Bio_Swamp:
        pt.vel = QPointF(rng().frange(-6, 6), -rng().frange(6, 14));
        pt.c = QColor(150, 220, 110, 100);
        pt.size = rng().frange(1.2f, 2.2f);
        pt.life = pt.maxLife = rng().frange(3.f, 5.f);
        pt.kind = 0;
        break;
    case Bio_Void:
        pt.vel = QPointF(rng().frange(-5, 5), rng().frange(-10, 4));
        pt.c = QColor(200, 150, 255, 120);
        pt.size = rng().frange(1.6f, 2.6f);
        pt.life = pt.maxLife = rng().frange(2.5f, 4.5f);
        pt.kind = 5;
        pt.spin = rng().frange(-1.5f, 1.5f);
        break;
    default:
        pt.vel = QPointF(rng().frange(-6, 6), rng().frange(-4, 8));
        pt.c = QColor(226, 200, 150, 70);
        pt.size = rng().frange(0.8f, 1.6f);
        pt.life = pt.maxLife = rng().frange(3.f, 5.f);
        pt.kind = 0;
        pt.additive = false;
        break;
    }
    m_parts.append(pt);
}

void GameView::burst(const QPointF &pos, int count, const QColor &c, int kind, float speed, float size, float life)
{
    int cap = (m_quality == 1) ? 1400 : 480;
    if (m_parts.size() > cap) count = qMin(count, qMax(0, cap - int(m_parts.size())));
    for (int i = 0; i < count; ++i) {
        Part p;
        float ang = rng().unit() * 6.2831853f;
        float sp = speed * (0.35f + rng().unit() * 0.8f);
        p.pos = pos + QPointF(rng().frange(-4, 4), rng().frange(-4, 4));
        p.vel = QPointF(qCos(ang) * sp, qSin(ang) * sp - (kind == 2 ? 20.f : 0.f));
        p.life = p.maxLife = life * (0.6f + rng().unit() * 0.7f);
        p.size = size * (0.6f + rng().unit() * 0.8f);
        p.c = c;
        p.kind = kind;
        p.grav = kind == 4 ? 260.f : (kind == 2 ? -40.f : 60.f);
        p.rot = rng().unit() * 6.28f;
        p.spin = rng().frange(-8.f, 8.f);
        p.additive = (kind != 2 && kind != 4);
        m_parts.append(p);
    }
}

void GameView::ring(const QPointF &pos, int count, const QColor &c, float radius)
{
    for (int i = 0; i < count; ++i) {
        float ang = float(i) / float(count) * 6.2831853f;
        Part p;
        p.pos = pos + QPointF(qCos(ang) * radius * 0.3f, qSin(ang) * radius * 0.3f);
        p.vel = QPointF(qCos(ang), qSin(ang)) * (radius * 1.7f);
        p.life = p.maxLife = 0.45f;
        p.size = 3.4f;
        p.c = c;
        p.kind = 1;
        p.grav = 0;
        m_parts.append(p);
    }
}

void GameView::ftext(const QPointF &pos, const QString &text, const QColor &c, bool crit, int kind, float size)
{
    FText f;
    f.pos = pos;
    f.text = text;
    f.c = c;
    f.crit = crit;
    f.kind = kind;
    f.size = size;
    if (kind == 5) {
        // длинная реплика NPC: дольше живёт, почти не дрейфует
        f.life = f.maxLife = 4.2f;
        f.vx = 0.f;
        f.vy = -6.f;
        // перенос по словам, чтобы текст влезал в окно над головой
        QFont fn = fontSmall(int(size));
        QFontMetrics fm(fn);
        const int maxW = 300;
        QStringList words = text.split(' ');
        QString cur;
        for (const QString &w : words) {
            QString tryS = cur.isEmpty() ? w : cur + ' ' + w;
            if (fm.width(tryS) > maxW && !cur.isEmpty()) {
                f.lines.append(cur);
                cur = w;
            } else {
                cur = tryS;
            }
        }
        if (!cur.isEmpty()) f.lines.append(cur);
    } else {
        f.life = f.maxLife = kind == 3 ? 1.1f : (crit ? 1.0f : 0.75f);
        f.vx = rng().frange(-24.f, 24.f);
        f.vy = -58.f - (crit ? 26.f : 0.f);
    }
    m_texts.append(f);
}

void GameView::shake(float amount) { if (m_shakeEnabled) m_shake = qMin(26.f, m_shake + amount); }
void GameView::flashScreen(const QColor &c, float amount)
{
    m_flashCol = c;
    m_flash = qMax(m_flash, amount);
}

// ============================================================
//  РОСЧЕРКИ УДАРОВ (AttackFx)
// ============================================================
int GameView::fxKindForActor(const Actor &a) const
{
    // по архетипу тела выбираем форму удара: когти / волна / взмах
    switch (Spr::archFor(a.tint)) {
    case Spr::A_Beast: case Spr::A_Spider: case Spr::A_Horror: return 2;   // когти
    case Spr::A_Golem: case Spr::A_Blob: case Spr::A_Plant: return 3;      // волна/удар
    case Spr::A_Robed: case Spr::A_Wisp: case Spr::A_Eye: return 5;        // колдовской круг
    case Spr::A_Elemental: return 3;
    default: return 0;                                                     // взмах клинком
    }
}

void GameView::spawnAttackFx(const QPointF &pos, float ang, int kind, const QColor &col,
                             float scale, float dur, bool flip)
{
    if (m_afx.size() > 120) m_afx.removeFirst();
    AttackFx f;
    f.pos = pos;
    f.ang = ang;
    f.kind = kind;
    f.color = col;
    f.scale = scale;
    f.dur = (dur > 0.f) ? dur : (kind == 3 ? 0.42f : (kind == 5 ? 0.5f : 0.26f));
    f.flip = flip;
    f.seed = int(rng().next() & 0x3FFF);
    m_afx.append(f);
}

void GameView::updateAttackFx(float dt)
{
    for (int i = m_afx.size() - 1; i >= 0; --i) {
        m_afx[i].t += dt;
        if (m_afx[i].t >= m_afx[i].dur) m_afx.removeAt(i);
    }
}

// ============================================================
//  NPC / ВЗАИМОДЕЙСТВИЕ / ЦЕЛЬ
// ============================================================
void GameView::updateNpcs(float dt)
{
    for (int i = 0; i < m_npcs.size(); ++i) m_npcs[i].animT += dt;
}

void GameView::breakPropsArc(const QPointF &from, const QPointF &dir, float range, float halfAng)
{
    for (int i = 0; i < m_dun.props.size(); ++i) {
        Prop &pr = m_dun.props[i];
        if (pr.kind != Pk_Barrel && pr.kind != Pk_Crate && pr.kind != Pk_Chest) continue;
        QPointF to = pr.pos - from;
        float dd = float(qSqrt(to.x() * to.x() + to.y() * to.y()));
        if (dd > range + pr.radius) continue;
        QPointF n = normv(to);
        if (float(n.x() * dir.x() + n.y() * dir.y()) < qCos(halfAng)) continue;
        openOrBreakProp(i);
    }
}

void GameView::breakPropsRadius(const QPointF &pos, float radius)
{
    for (int i = 0; i < m_dun.props.size(); ++i) {
        Prop &pr = m_dun.props[i];
        if (pr.kind != Pk_Barrel && pr.kind != Pk_Crate && pr.kind != Pk_Chest) continue;
        if (distf(pr.pos, pos) > radius + pr.radius) continue;
        openOrBreakProp(i);
    }
}

void GameView::openOrBreakProp(int idx)
{
    Prop &pr = m_dun.props[idx];
    const LocationDef *L = Data::loc(m_locId);
    int ilvl = L ? L->tier * 6 : m_game->level();
    if (pr.kind == Pk_Chest) {
        pr.kind = Pk_ChestOpen;
        pr.solid = false;
        Audio::inst().play(Sfx_Gold);
        Audio::inst().play(Sfx_Pickup);
        ring(pr.pos, 20, C::gold, 54.f);
        burst(pr.pos + QPointF(0, -10), 26, C::gold, 1, 160, 3.4f, 0.9f);
        ftext(pr.pos + QPointF(0, -34), QStringLiteral("Сундук!"), C::gold, true, 3, 15.f);
        emit logMsg(QStringLiteral("Ты открыл сундук."), C::gold);
        Pickup g;
        g.kind = 1;
        g.gold = rng().range(30, 60) * (L ? L->tier : 1);
        g.pos = m_dun.nearestFree(pr.pos + QPointF(0, -6));
        g.vel = QPointF(rng().frange(-90, 90), rng().frange(-110, -30));
        m_pickups.append(g);
        Item it = m_game->rollItem(ilvl + 2, 0.8f, rng().chance(0.25f) ? Rar_Legend : -1);
        if (it.valid) {
            Pickup pk;
            pk.kind = 0;
            pk.item = it;
            pk.pos = m_dun.nearestFree(pr.pos + QPointF(rng().frange(-18, 18), rng().frange(-14, 14)));
            pk.vel = QPointF(rng().frange(-60, 60), rng().frange(-90, -20));
            m_pickups.append(pk);
        }
    } else {
        PropKind was = pr.kind;
        pr.kind = Pk_None;
        pr.solid = false;
        Audio::inst().play(Sfx_Hit);
        burst(pr.pos, 14, was == Pk_Barrel ? QColor(112, 82, 52) : QColor(126, 98, 62), 3, 150, 3.6f, 0.7f);
        burst(pr.pos, 6, QColor(60, 44, 30), 2, 60, 4.f, 0.8f);
        if (rng().chance(0.55f)) {
            Pickup g;
            g.kind = 1;
            g.gold = rng().range(4, 14) * (L ? L->tier : 1);
            g.pos = m_dun.nearestFree(pr.pos);
            g.vel = QPointF(rng().frange(-80, 80), rng().frange(-90, -20));
            m_pickups.append(g);
        }
        if (rng().chance(0.22f)) {
            Pickup pk;
            pk.kind = 2;
            pk.item = m_game->rollPotion(ilvl);
            pk.pos = m_dun.nearestFree(pr.pos + QPointF(rng().frange(-12, 12), rng().frange(-10, 10)));
            pk.vel = QPointF(rng().frange(-50, 50), rng().frange(-70, -10));
            m_pickups.append(pk);
        }
    }
}

void GameView::interact()
{
    Actor *p = player();
    if (!p || m_playerDead) return;
    if (m_nearNpc) {
        Audio::inst().play(Sfx_Ui);
        // любой NPC при разговоре выдаёт случайную реплику (без повтора подряд)
        Npc &npc = *m_nearNpc;
        if (!npc.lines.isEmpty()) {
            int idx = rng().pick(npc.lines.size());
            if (idx == npc.lastLine) idx = (idx + 1) % npc.lines.size();
            npc.lastLine = idx;
            npc.line = npc.lines.at(idx);
        }
        ftext(npc.pos + QPointF(0, -74), npc.line, C::amber, false, 5, 12.5f);
        if (npc.kind == 0) emit requestShop();
        else if (npc.kind == 1) emit requestRespec();
        else if (npc.kind != 2 && npc.kind != 3) { /* нет обработчика */ }
        else if (npc.kind == 3) { /* старейшина: только разговор */ }
        else emit requestHeal();
        return;
    }
    if (m_nearPortal) {
        if (!m_portalActive) {
            ftext(p->pos + QPointF(0, -50), QStringLiteral("Портал закрыт"), C::bad, false, 3, 13.f);
            Audio::inst().play(Sfx_UiBad);
            return;
        }
        Audio::inst().play(Sfx_Portal);
        emit requestExitToMap();
        return;
    }
    if (m_nearAltar) {
        if (m_shrineUsed) { Audio::inst().play(Sfx_UiBad); return; }
        m_shrineUsed = true;
        int kind = 1 + rng().pick(5);
        Audio::inst().play(Sfx_Heal);
        ring(p->pos, 30, C::arcane, 80.f);
        emit requestShrine(kind);
        return;
    }
}

int GameView::enemiesAlive() const
{
    int n = 0;
    for (int i = 0; i < m_actors.size(); ++i) {
        const Actor &a = m_actors[i];
        if (a.dead || a.isPlayer || a.isSummon) continue;
        ++n;
    }
    return n;
}

void GameView::checkObjective()
{
    if (m_portalActive) return;
    if (m_dun.kind == 1) { m_game->visitLocation(m_locId); m_portalActive = true; return; }
    if (enemiesAlive() > 0) return;
    m_portalActive = true;
    Audio::inst().play(Sfx_Portal);
    const LocationDef *L = Data::loc(m_locId);
    bool bossKilled = L && L->bossId >= 0;
    if (L) m_game->completeLocation(m_locId, bossKilled);
    {
        AchTracker &AC = AchTracker::inst();
        AC.setCounterMax(Ach::C_LocsCleared, m_game->clearedSet().size());
        Actor *pl = player();
        float hpFrac = (pl && m_game->maxHp() > 0.f) ? m_game->hp() / m_game->maxHp() : 1.f;
        AC.locCleared(m_locDmgTaken <= 0.5f, m_locTime, hpFrac, L && L->type == Loc_Elite);
    }
    emit logMsg(QStringLiteral("Локация зачищена! Портал открыт."), C::good);
    ftext(player() ? player()->pos : m_dun.playerSpawn, QStringLiteral("Путь открыт"), C::good, true, 3, 17.f);
    emit locationCleared(m_locId);
    // бонус за первое прохождение
    if (L && m_game->clearCount(m_locId) <= 1) {
        int bonus = 20 + L->tier * 22 + (bossKilled ? 120 * L->tier : 0);
        m_game->addGold(bonus);
        emit logMsg(QStringLiteral("Награда за зачистку: %1 золота.").arg(bonus), C::gold);
        Item it = m_game->rollItem(L->tier * 6 + 2, 0.6f, bossKilled ? Rar_Legend : -1);
        if (it.valid) {
            Pickup pk;
            pk.kind = 0;
            pk.item = it;
            pk.pos = m_dun.nearestFree(m_dun.exitPos + QPointF(0, 46));
            m_pickups.append(pk);
        }
    }
}

QString GameView::objectiveText() const
{
    const LocationDef *L = Data::loc(m_locId);
    if (!L) return QString();
    if (L->type == Loc_Town) return QStringLiteral("Поселение - торгуй, лечись и уходи через портал");
    if (m_portalActive) return QStringLiteral("Зачищено - иди к порталу (F)");
    if (L->bossId >= 0) return QStringLiteral("Убей босса и всех врагов (%1)").arg(enemiesAlive());
    if (L->type == Loc_Shrine) return QStringLiteral("Перебей стражей и коснись алтаря");
    return QStringLiteral("Зачисти локацию: осталось %1").arg(enemiesAlive());
}

float GameView::objectiveProgress() const
{
    if (m_totalEnemies <= 0) return m_portalActive ? 1.f : 0.f;
    int alive = enemiesAlive();
    return clampf(1.f - float(alive) / float(m_totalEnemies), 0.f, 1.f);
}

QVector<QPair<int, float> > GameView::activeBuffs() const
{
    QVector<QPair<int, float> > out;
    Actor *p = const_cast<GameView *>(this)->player();
    if (!p) return out;
    if (p->st.bloodlust > 0.f) out.append(qMakePair(int(Gl_Blood), p->st.bloodlust));
    if (p->st.haste > 0.f) out.append(qMakePair(int(Gl_Haste), p->st.haste));
    if (m_game->shield() > 0.f) out.append(qMakePair(int(Gl_ShieldDot), 999.f));
    if (m_game->shrineTime() > 0.f) {
        int g = Gl_Star;
        switch (m_game->shrine()) {
        case Shrine_Might: g = Gl_Swords; break;
        case Shrine_Ward: g = Gl_Ward; break;
        case Shrine_Fortune: g = Gl_Coins; break;
        case Shrine_Swift: g = Gl_Haste; break;
        case Shrine_Arcane: g = Gl_Crystal; break;
        default: g = Gl_Star; break;
        }
        out.append(qMakePair(g, m_game->shrineTime()));
    }
    if (m_souls > 0) out.append(qMakePair(int(Gl_Skull), float(m_souls)));
    return out;
}

// ============================================================
//  КАМЕРА
// ============================================================
void GameView::clampCamera()
{
    float vw = float(width()) / m_zoom, vh = float(height()) / m_zoom;
    float maxX = float(m_dun.widthPx()) - vw;
    float maxY = float(m_dun.heightPx()) - vh;
    if (maxX <= 0.f) m_cam.setX(maxX * 0.5f);
    else m_cam.setX(qBound(0.f, FX(m_cam), maxX));
    if (maxY <= 0.f) m_cam.setY(maxY * 0.5f);
    else m_cam.setY(qBound(0.f, FY(m_cam), maxY));
}

void GameView::updateCamera(float dt)
{
    Actor *p = player();
    if (!p) return;
    QPointF rp = p->prevPos + (p->pos - p->prevPos) * m_alpha;
    QPointF focus = rp + (m_aim - rp) * 0.16f;
    QPointF target = focus - QPointF(width() * 0.5 / m_zoom, height() * 0.5 / m_zoom);
    float k = clampf(dt * 7.f, 0.f, 1.f);
    m_cam += (target - m_cam) * k;
    clampCamera();

    if (m_shake > 0.f) {
        m_shake = qMax(0.f, m_shake - dt * 34.f);
        m_shakeOff = QPointF(rng().frange(-m_shake, m_shake), rng().frange(-m_shake, m_shake));
    } else m_shakeOff = QPointF(0, 0);
    if (m_flash > 0.f) m_flash = qMax(0.f, m_flash - dt * 2.2f);
}

void GameView::centerCamera()
{
    Actor *p = player();
    if (!p) return;
    m_cam = p->pos - QPointF(width() * 0.5 / m_zoom, height() * 0.5 / m_zoom);
    clampCamera();
}

// ============================================================
//  ВВОД
// ============================================================
void GameView::keyPressEvent(QKeyEvent *e)
{
    int k = e->key();
    // «неигровые» клавиши (панели, сохранения) отдаём окну - с учётом привязок
    for (int a = Binds::A_Inv; a <= Binds::A_Ach; ++a) {
        if (Binds::matches(k, a)) { emit uiKeyPressed(a); e->accept(); return; }
    }
    m_keys.insert(k);
    if (!m_interactive || m_paused) { e->accept(); return; }

    for (int i = 0; i < 6; ++i) {
        if (Binds::matches(k, Binds::A_Skill1 + i)) { castSkill(i); e->accept(); return; }
    }
    if (Binds::matches(k, Binds::A_Potion)) {
        if (m_potionCd <= 0.f) {
            if (m_game->usePotion()) { m_potionCd = 0.6f; Audio::inst().play(Sfx_Heal); }
            else Audio::inst().play(Sfx_UiBad);
        }
    } else if (Binds::matches(k, Binds::A_Interact)) {
        interact();
    } else if (Binds::matches(k, Binds::A_Return)) {
        // свиток возвращения
        const LocationDef *L = Data::loc(m_locId);
        if (L && L->type != Loc_Town && m_game->useScrollOfReturn()) {
            Audio::inst().play(Sfx_Portal);
            Actor *p = player();
            if (p) { ring(p->pos, 26, C::arcane, 60.f); burst(p->pos, 30, C::arcane, 2, 140, 5.f, 0.9f); }
            emit logMsg(QStringLiteral("Свиток возвращения рассыпался пеплом."), C::arcane);
            emit requestExitToMap();
        } else if (L && L->type == Loc_Town) {
            emit logMsg(QStringLiteral("Ты и так в поселении."), C::boneDim);
        } else {
            Audio::inst().play(Sfx_UiBad);
            emit logMsg(QStringLiteral("Свитков возвращения больше нет."), C::bad);
        }
    } else if (Binds::matches(k, Binds::A_Shard)) {
        // осколок души - полный переброс
        if (m_game->useSoulShard()) { Audio::inst().play(Sfx_Heal); emit requestRespec(); }
        else Audio::inst().play(Sfx_UiBad);
    }
    e->accept();
}

void GameView::keyReleaseEvent(QKeyEvent *e)
{
    m_keys.remove(e->key());
    e->accept();
}

void GameView::mousePressEvent(QMouseEvent *e)
{
    m_mouse = e->localPos();
    if (!m_interactive || m_paused) { e->accept(); return; }
    if (e->button() == Qt::LeftButton) {
        m_lmb = true;
        Actor *pl0 = player();
        // клик не пробивает кулдаун скорости атаки - иначе можно «закликивать» врагов
        if (m_attackCd <= 0.f && pl0 && pl0->rollT <= 0.f) basicAttack();
    }
    else if (e->button() == Qt::RightButton) { m_rmb = true; dodgeRoll(); }
    else if (e->button() == Qt::MiddleButton) interact();
    setFocus(Qt::MouseFocusReason);
    e->accept();
}

void GameView::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) m_lmb = false;
    if (e->button() == Qt::RightButton) m_rmb = false;
    e->accept();
}

void GameView::mouseMoveEvent(QMouseEvent *e)
{
    m_mouse = e->localPos();
    m_aim = toWorld(m_mouse);
    e->accept();
}

void GameView::wheelEvent(QWheelEvent *e)
{
    if (!m_interactive || m_paused) { e->accept(); return; }
    float d = float(e->angleDelta().y());
    float nz = clampf(m_zoom * (d > 0.f ? 1.12f : 0.89f), 0.65f, 1.9f);
    if (qAbs(nz - m_zoom) > 0.001f) {
        QPointF mw = toWorld(m_mouse);          // точка мира под курсором
        m_zoom = nz;
        m_cam = mw - QPointF(m_mouse.x() / m_zoom, m_mouse.y() / m_zoom);
        clampCamera();
        m_aim = toWorld(m_mouse);
    }
    e->accept();
}

void GameView::setZoom(float z)
{
    QPointF center(width() * 0.5, height() * 0.5);
    QPointF mw = toWorld(center);
    m_zoom = clampf(z, 0.65f, 1.9f);
    m_cam = mw - QPointF(center.x() / m_zoom, center.y() / m_zoom);
    clampCamera();
}

// Перепечь кэш земли: вызывается после переключения спрайт-листов/тайлсетов,
// чтобы текущая локация сразу получила (или потеряла) спрайтовые тайлы.
void GameView::rebakeGround()
{
    if (!m_dun.valid()) return;
    m_dun.buildGround();
    m_groundZDirty = true;
    update();
}

void GameView::ensureGroundCache(float z)
{
    if (!m_dun.valid() || m_dun.ground.isNull()) { m_groundZ = QImage(); m_groundZZoom = -1.f; return; }
    QSize need(qCeil(m_dun.widthPx() * z), qCeil(m_dun.heightPx() * z));
    // защита памяти: очень большой кэш не держим - рисуем по-старому
    if (quint64(need.width()) * quint64(need.height()) * 4ull > 160ull * 1024ull * 1024ull) {
        m_groundZ = QImage();
        m_groundZZoom = -1.f;
        return;
    }
    if (!m_groundZDirty && m_groundZ.size() == need && qFuzzyCompare(m_groundZZoom, z)) return;
    QImage img(need, QImage::Format_ARGB32_Premultiplied);
    {
        QPainter gp(&img);
        gp.setRenderHint(QPainter::SmoothPixmapTransform, true);
        gp.drawImage(QRectF(0, 0, need.width(), need.height()), m_dun.ground);
        if (!m_decals.isNull()) {
            gp.setOpacity(0.88f);
            gp.drawImage(QRectF(0, 0, need.width(), need.height()), m_decals);
        }
        gp.end();
    }
    m_groundZ = img;
    m_groundZZoom = z;
    m_groundZDirty = false;
}

void GameView::setShadowQ(int q)
{
    m_shadowQ = qBound(0, q, 1);
    // уровень 0 убирает и «пятна» под существами (они рисуются в Sprites.cpp)
    Spr::setBlobShadows(m_shadowQ > 0);
    update();
}

QRectF GameView::worldViewRect() const
{
    return QRectF(m_cam.x(), m_cam.y(), float(width()) / m_zoom, float(height()) / m_zoom);
}

void GameView::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
#ifdef HAVE_GL
    if (m_fx) m_fx->setGeometry(rect());
#endif
    QSize s(width() / 2, height() / 2);
    if (s.width() < 2) s.setWidth(2);
    if (s.height() < 2) s.setHeight(2);
    if (m_light.size() != s) m_light = QImage(s, QImage::Format_ARGB32_Premultiplied);
}

void GameView::focusInEvent(QFocusEvent *e) { QWidget::focusInEvent(e); }
void GameView::focusOutEvent(QFocusEvent *e)
{
    m_keys.clear();
    m_lmb = false;
    m_rmb = false;
    QWidget::focusOutEvent(e);
}

void GameView::setPaused(bool p)
{
    m_paused = p;
    if (!p) { m_clock.restart(); m_keys.clear(); }
}

void GameView::debugStep(float dt)
{
    if (!m_paused && m_interactive && m_dun.valid()) {
        updateSim(dt);
        m_game->tick(dt);
        syncPlayerStats();
    }
    m_time += dt;
}

bool GameView::debugTalkToNpc(int npcKind)
{
    for (int i = 0; i < m_npcs.size(); ++i) {
        if (m_npcs[i].kind != npcKind) continue;
        Actor *p = player();
        if (!p) return false;
        p->pos = m_npcs[i].pos + QPointF(20, 0);   // встаём вплотную
        interact();                                // тот же путь, что и клавиша [F]
        return true;
    }
    return false;
}

QString GameView::lastNpcLine() const
{
    for (int i = m_texts.size() - 1; i >= 0; --i)
        if (m_texts[i].kind == 5) return m_texts[i].text;
    return QString();
}

int GameView::lastNpcLines() const
{
    for (int i = m_texts.size() - 1; i >= 0; --i)
        if (m_texts[i].kind == 5) return qMax(1, int(m_texts[i].lines.size()));
    return 0;
}

// ============================================================
//  МИНИКАРТА
// ============================================================
void GameView::buildMinimap()
{
    if (!m_dun.valid()) { m_minimap = QImage(); return; }
    BiomePal pal = biomePalette(m_dun.biome);
    QImage img(m_dun.w, m_dun.h, QImage::Format_RGB32);
    for (int y = 0; y < m_dun.h; ++y) {
        for (int x = 0; x < m_dun.w; ++x) {
            quint8 t = m_dun.tileAt(x, y);
            QColor c;
            switch (t) {
            case T_Wall:   c = pal.wallTop.darker(175); break;
            case T_Hazard: c = pal.hazard.darker(130); break;
            case T_Water:  c = QColor(70, 120, 150); break;
            case T_Road:   c = pal.floor3.darker(115); break;
            default:       c = pal.floor3.lighter(108); break;
            }
            img.setPixel(x, y, c.rgb());
        }
    }
    m_minimap = img;
}

void GameView::renderMinimap(QPainter *p, const QRectF &r)
{
    if (!m_minimapOn) return;   // скрыта настройкой «Миникарта» в видео-опциях
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setRenderHint(QPainter::SmoothPixmapTransform, true);
    p->setClipRect(r);
    p->fillRect(r, C::ink);
    if (m_minimap.isNull() || !m_dun.valid()) { p->restore(); return; }

    float sc = qMin(float(r.width()) / float(m_minimap.width()), float(r.height()) / float(m_minimap.height()));
    float dw = m_minimap.width() * sc, dh = m_minimap.height() * sc;
    float ox = r.left() + (r.width() - dw) * 0.5f, oy = r.top() + (r.height() - dh) * 0.5f;

    // центрируем на игроке, если карта больше области
    Actor *pl = player();
    if (pl) {
        float px = ox + FX(pl->pos) / float(m_dun.tileSize) * sc;
        float py = oy + FY(pl->pos) / float(m_dun.tileSize) * sc;
        float cx = r.center().x(), cy = r.center().y();
        float shx = qBound(float(r.width() * 0.5f - dw), float(cx - px), 0.f);
        float shy = qBound(float(r.height() * 0.5f - dh), float(cy - py), 0.f);
        if (dw > r.width()) { ox += shx; }
        if (dh > r.height()) { oy += shy; }
    }

    p->drawImage(QRectF(ox, oy, dw, dh), m_minimap);

    auto toMap = [&](const QPointF &w) -> QPointF {
        return QPointF(ox + FX(w) / float(m_dun.tileSize) * sc, oy + FY(w) / float(m_dun.tileSize) * sc);
    };

    // враги
    for (int i = 0; i < m_actors.size(); ++i) {
        const Actor &a = m_actors[i];
        if (a.dead || a.isPlayer) continue;
        QPointF mp = toMap(a.pos);
        float rr = a.boss ? 4.2f : (a.elite ? 3.0f : 2.0f);
        QColor c = a.boss ? QColor(255, 90, 70) : (a.isSummon ? C::arcane : (a.elite ? C::amber : C::bloodHi));
        p->setPen(QPen(QColor(0, 0, 0, 170), 1.2));
        p->setBrush(c);
        p->drawEllipse(mp, rr, rr);
    }
    // NPC
    for (int i = 0; i < m_npcs.size(); ++i) {
        QPointF mp = toMap(m_npcs[i].pos);
        p->setPen(QPen(QColor(0, 0, 0, 170), 1.0));
        p->setBrush(C::gold);
        p->drawEllipse(mp, 2.4f, 2.4f);
    }
    // портал
    {
        QPointF mp = toMap(m_dun.exitPos);
        p->setPen(QPen(QColor(0, 0, 0, 170), 1.0));
        p->setBrush(m_portalActive ? C::arcane : C::ash);
        QPolygonF pg;
        pg << mp + QPointF(0, -4) << mp + QPointF(3.4f, 0) << mp + QPointF(0, 4) << mp + QPointF(-3.4f, 0);
        p->drawPolygon(pg);
    }
    // игрок
    if (pl) {
        QPointF mp = toMap(pl->pos);
        p->save();
        p->translate(mp);
        p->rotate(qRadiansToDegrees(pl->facing) + 90.f);
        p->setPen(QPen(QColor(0, 0, 0, 200), 1.2));
        p->setBrush(C::bone);
        QPolygonF pg;
        pg << QPointF(0, -5) << QPointF(3.6f, 4) << QPointF(0, 2) << QPointF(-3.6f, 4);
        p->drawPolygon(pg);
        p->restore();
    }
    Style::frame(p, r, C::edge, 1.4);
    p->restore();
}

void GameView::renderTargetInfo(QPainter *p, const QRectF &r)
{
    Actor *hover = 0;
    QPointF w = m_aim;
    for (int i = 0; i < m_actors.size(); ++i) {
        Actor &a = m_actors[i];
        if (a.dead || a.isPlayer || a.isSummon) continue;
        if (distf(a.pos + QPointF(0, -a.radius), w) < a.radius * 1.7f + 12.f) { hover = &a; break; }
    }
    if (!hover) return;
    p->save();
    QString nm = hover->boss ? hover->bossTitle
                             : (hover->elite ? QStringLiteral("Элитный ") + (Data::enemy(hover->defId) ? Data::enemy(hover->defId)->name : QString())
                                             : (Data::enemy(hover->defId) ? Data::enemy(hover->defId)->name : QString()));
    if (!hover->nameOverride.isEmpty() && hover->elite) nm = hover->nameOverride;
    QFont f = fontSmall(10);
    QFontMetrics fm(f);
    int tw = fm.width(nm) + 20;
    QRectF box(r.left() + (r.width() - tw) * 0.5f, r.top(), tw, 26);
    Style::panel(p, box, 1, 0.92f);
    Style::textShadow(p, QRectF(box.left(), box.top() + 1, box.width(), 13), nm,
                      hover->boss ? C::bad : (hover->elite ? C::amber : C::bone), f, Qt::AlignCenter);
    float t = clampf(hover->hp / qMax(1.f, hover->maxHp), 0.f, 1.f);
    Style::bar(p, QRectF(box.left() + 8, box.top() + 15, box.width() - 16, 7), t,
               hover->boss ? C::blood : C::hp, C::edge);
    p->restore();
}

// ============================================================
//  ОТРИСОВКА
// ============================================================
struct RItem { float y; int type; int idx; };
static bool ritemLess(const RItem &a, const RItem &b) { return a.y < b.y; }

void GameView::paintEvent(QPaintEvent *)
{
    if (!m_dun.valid()) {
        QPainter p0(this);
        p0.fillRect(rect(), C::voidBlack);
        return;
    }
#ifdef HAVE_GL
    if (m_fxEnabled && m_fx && m_fx->ok()) {
        if (m_sceneImg.size() != size()) m_sceneImg = QImage(size(), QImage::Format_RGB32);
        QPainter bp(&m_sceneImg);
        bp.setRenderHint(QPainter::Antialiasing, true);
        bp.setRenderHint(QPainter::SmoothPixmapTransform, true);
        paintScene(bp);
        drawCursor(bp);
        bp.end();
        m_fx->update();
        QPainter p(this);
        p.fillRect(rect(), C::voidBlack);
        return;
    }
#endif
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    paintScene(p);
    if (m_scanlines) drawScanlines(p);
    drawCursor(p);
}

void GameView::setPostFx(bool on)
{
#ifdef HAVE_GL
    if (on) {
        if (!m_fx) {
            m_fx = new GlFxView(this, this);
            m_fx->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            m_fx->setFocusPolicy(Qt::NoFocus);
            connect(m_fx, SIGNAL(failed()), this, SLOT(onFxFailed()), Qt::QueuedConnection);
            m_fx->setGeometry(rect());
            m_fx->show();
            m_fx->raise();
        } else {
            m_fx->setGeometry(rect());
            m_fx->show();
            m_fx->raise();
        }
        m_fxEnabled = true;
    } else {
        m_fxEnabled = false;
        if (m_fx) m_fx->hide();
    }
#else
    if (on) emit logMsg(QStringLiteral("Постобработка GL недоступна в этой сборке (нет модуля opengl)."), C::bad);
    m_fxEnabled = false;
#endif
    update();
}

void GameView::onFxFailed()
{
    m_fxEnabled = false;
#ifdef HAVE_GL
    if (m_fx) m_fx->hide();
#endif
    emit logMsg(QStringLiteral("OpenGL не смог инициализироваться - постобработка отключена, рендер как раньше."), C::bad);
    emit postFxFailed();
    update();
}

void GameView::paintScene(QPainter &p)
{
    if (m_pxMode > 1) {
        QSize small(qMax(2, width() / m_pxMode), qMax(2, height() / m_pxMode));
        if (m_buf.size() != small) m_buf = QImage(small, QImage::Format_RGB32);
        QPainter bp(&m_buf);
        bp.setRenderHint(QPainter::Antialiasing, false);
        bp.setRenderHint(QPainter::SmoothPixmapTransform, false);
        bp.scale(1.0 / m_pxMode, 1.0 / m_pxMode);
        renderFrame(bp);
        bp.end();
        p.setRenderHint(QPainter::Antialiasing, false);
        p.setRenderHint(QPainter::SmoothPixmapTransform, false);
        p.drawImage(QRectF(0, 0, width(), height()), m_buf, QRectF(0, 0, m_buf.width(), m_buf.height()));
        return;
    }
    renderFrame(p);
}

void GameView::drawScanlines(QPainter &p)
{
    p.save();
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 46));
    for (int y = 0; y < height(); y += 3)
        p.drawRect(0, y, width(), 1);
    p.restore();
}

void GameView::drawCursor(QPainter &p)
{
    if (!m_interactive || m_hideUi) return;
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    QPointF c = m_mouse;
    float pulse = 0.7f + 0.3f * qSin(m_time * 5.f);
    p.setPen(QPen(C::amber, 1.6));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(c, 9.f, 9.f);
    p.setPen(QPen(QColor(242, 181, 68, int(150 * pulse)), 1.2));
    p.drawPath(runeRing(c, 13.f, 6, 3.4f, m_time * 1.4f));
    p.setPen(QPen(C::bone, 1.6));
    p.drawLine(c - QPointF(4, 0), c + QPointF(4, 0));
    p.drawLine(c - QPointF(0, 4), c + QPointF(0, 4));
    p.restore();
}

void GameView::renderFrame(QPainter &p)
{
    BiomePal pal = biomePalette(m_dun.biome);
    p.fillRect(QRectF(0, 0, float(width()), float(height())), pal.sky);

    // интерполированные позиции для плавной картинки между шагами симуляции
    m_render = m_actors;
    for (int i = 0; i < m_render.size(); ++i) {
        const Actor &a = m_actors.at(i);
        m_render[i].pos = a.prevPos + (a.pos - a.prevPos) * m_alpha;
    }
    m_renderProjs = m_projs;
    for (int i = 0; i < m_renderProjs.size(); ++i) {
        const Proj &pr = m_projs.at(i);
        m_renderProjs[i].pos = pr.prevPos + (pr.pos - pr.prevPos) * m_alpha;
    }

    const float z = m_zoom;
    // камера вместе с тряской, привязанная к целым экранным пикселям:
    // землю из кэша можно класть строго 1:1 без дорогого гладкого масштаба
    QPointF camS(m_cam.x() - m_shakeOff.x() / z, m_cam.y() - m_shakeOff.y() / z);
    QPointF camSnap(qRound(camS.x() * z) / z, qRound(camS.y() * z) / z);

    // --- земля из кэша (экранные координаты, 1:1) ---
    bool groundCached = false;
    if (m_pxMode == 1) {
        ensureGroundCache(z);
        if (!m_groundZ.isNull()) {
            const int ox = int(qRound(camSnap.x() * z)), oy = int(qRound(camSnap.y() * z));
            QRect isr = QRect(ox, oy, width(), height())
                    .intersected(QRect(0, 0, m_groundZ.width(), m_groundZ.height()));
            if (!isr.isEmpty()) {
                p.drawImage(QPoint(isr.x() - ox, isr.y() - oy), m_groundZ, isr);
                groundCached = true;
            }
        }
    }

    p.save();
    p.scale(z, z);
    p.translate(-camSnap.x(), -camSnap.y());

    QRectF wv = worldViewRect();
    QRectF view = wv.adjusted(-120.0 / z, -160.0 / z, 120.0 / z, 160.0 / z);

    // --- запасной путь: земля напрямую (пиксель-режим или слишком большой кэш) ---
    if (!groundCached) {
        QRect src(int(qMax(0.0, wv.left())), int(qMax(0.0, wv.top())),
                  int(qMin(double(m_dun.ground.width()), wv.right()) - qMax(0.0, wv.left())),
                  int(qMin(double(m_dun.ground.height()), wv.bottom()) - qMax(0.0, wv.top())));
        if (src.width() > 0 && src.height() > 0) {
            if (m_pxMode > 1) p.drawImage(QPoint(src.x(), src.y()), m_dun.ground, src);
            else { p.setRenderHint(QPainter::SmoothPixmapTransform, true); p.drawImage(QPoint(src.x(), src.y()), m_dun.ground, src); }
        }
    }
    // стойкие следы на земле (в кэш они пекутся сразу; тут - только запасной путь)
    if (!groundCached && !m_decals.isNull() && m_decalBounds.intersects(wv)) {
        QRectF visW = wv.intersected(QRectF(0, 0, m_dun.widthPx(), m_dun.heightPx()))
                        .intersected(m_decalBounds);
        if (visW.width() > 2 && visW.height() > 2) {
            p.save();
            p.setOpacity(0.88f);
            p.drawImage(visW, m_decals, QRectF(visW.left() * 0.5, visW.top() * 0.5,
                                               visW.width() * 0.5, visW.height() * 0.5));
            p.restore();
        }
    }
    drawWaterReflections(p);

    // --- пульсация опасных тайлов ---
    int tx0 = qMax(0, int(view.left() / m_dun.tileSize)), tx1 = qMin(m_dun.w - 1, int(view.right() / m_dun.tileSize));
    int ty0 = qMax(0, int(view.top() / m_dun.tileSize)), ty1 = qMin(m_dun.h - 1, int(view.bottom() / m_dun.tileSize));
    for (int ty = ty0; ty <= ty1; ++ty) {
        for (int tx = tx0; tx <= tx1; ++tx) {
            quint8 t = m_dun.tileAt(tx, ty);
            QPointF c((tx + 0.5f) * m_dun.tileSize, (ty + 0.5f) * m_dun.tileSize);
            if (t == T_Hazard) {
                float pulse = 0.5f + 0.5f * qSin(m_time * 2.4f + tx * 0.7f + ty * 0.5f);
                Spr::drawLightHalo(&p, c, m_dun.tileSize * (0.9f + pulse * 0.5f), pal.hazard, 0.18f + pulse * 0.14f);
            } else if (t == T_Water) {
                float pulse = 0.5f + 0.5f * qSin(m_time * 1.4f + tx * 0.9f - ty * 0.6f);
                p.setPen(QPen(QColor(190, 230, 255, int(20 + pulse * 26)), 1.0));
                p.drawLine(c - QPointF(8, 0), c + QPointF(8, 0));
                // бегущие блики - не на каждой плитке (дешевле на больших водоёмах)
                if ((tx * 7 + ty * 13) % 3 == 0) {
                    p.setPen(QPen(QColor(220, 246, 255, int(26 + 22 * qSin(m_time * 2.2f + tx * 1.7f))), 1.2));
                    QPointF o(qSin(m_time * 0.9f + ty) * 6, qCos(m_time * 0.7f + tx) * 3);
                    p.drawArc(QRectF(c - QPointF(10, 6) + o, QSizeF(20, 12)), 30 * 16, 120 * 16);
                }
            } else if (t == T_Hazard && m_dun.biome == Bio_Lava) {
                // пузыри лавы
                float bt = fmodf(m_time * 0.65f + (tx * 3 + ty * 5) * 0.13f, 1.f);
                QPointF bc = c + QPointF(qSin(tx * 7.3f) * 8, qCos(ty * 5.1f) * 6);
                p.setPen(QPen(QColor(255, 220, 140, int(150 * (1.f - bt))), 1.4));
                p.setBrush(QColor(255, 150, 50, int(90 * (1.f - bt))));
                p.drawEllipse(bc, 2 + bt * 7, (2 + bt * 7) * 0.6f);
            }
        }
    }

    for (int i = 0; i < m_zones.size(); ++i) {
        const Zone &z = m_zones[i];
        if (z.delay > 0.f) continue;
        if (!view.contains(z.pos)) continue;
        Spr::drawZone(&p, z, m_time);
    }
    for (int i = 0; i < m_tels.size(); ++i) {
        if (!view.intersects(QRectF(m_tels[i].pos.x() - 400, m_tels[i].pos.y() - 400, 800, 800))) continue;
        Spr::drawTelegraph(&p, m_tels[i]);
    }
    if (view.contains(m_dun.exitPos) || true) Spr::drawPortal(&p, m_dun.exitPos, m_time, m_portalActive);

    QVector<RItem> list;
    list.reserve(m_dun.props.size() + m_actors.size() + m_npcs.size() + m_pickups.size());
    for (int i = 0; i < m_dun.props.size(); ++i) {
        const Prop &pr = m_dun.props.at(i);
        if (!view.intersects(QRectF(pr.pos.x() - 120, pr.pos.y() - 200, 240, 240))) continue;
        RItem it; it.y = FY(pr.pos); it.type = 0; it.idx = i; list.append(it);
    }
    for (int i = 0; i < m_render.size(); ++i) {
        const Actor &a = m_render[i];
        if (!view.intersects(QRectF(a.pos.x() - 140, a.pos.y() - 220, 280, 280))) continue;
        RItem it; it.y = FY(a.pos); it.type = 1; it.idx = i; list.append(it);
    }
    for (int i = 0; i < m_npcs.size(); ++i) {
        RItem it; it.y = FY(m_npcs[i].pos); it.type = 2; it.idx = i; list.append(it);
    }
    for (int i = 0; i < m_pickups.size(); ++i) {
        const Pickup &pk = m_pickups[i];
        if (!view.contains(pk.pos)) continue;
        RItem it; it.y = FY(pk.pos); it.type = 3; it.idx = i; list.append(it);
    }
    std::sort(list.begin(), list.end(), ritemLess);
    for (int i = 0; i < list.size(); ++i) {
        const RItem &it = list.at(i);
        switch (it.type) {
        case 0: Spr::drawProp(&p, m_dun.props.at(it.idx), m_time, m_dun.biome); break;
        case 1: {
            const Actor &a = m_render.at(it.idx);
            if (a.isPlayer) {
                const Item w = m_game->equipped(Slot_Weapon);
                const ItemBaseDef *wb = w.valid ? Data::base(w.baseId) : 0;
                const Item o = m_game->equipped(Slot_Offhand);
                const ItemBaseDef *ob = o.valid ? Data::base(o.baseId) : 0;
                Spr::drawPlayer(&p, a, m_time, m_game->classId(), wb ? wb->glyph : Gl_Sword,
                                ob && ob->glyph == Gl_Shield);
            } else {
                Spr::drawActor(&p, a, m_time);
            }
            break;
        }
        case 2: Spr::drawNpc(&p, m_npcs.at(it.idx), m_time); break;
        case 3: {
            const Pickup &pk = m_pickups.at(it.idx);
            if (pk.kind == 1) Spr::drawGoldPile(&p, pk.pos, m_time, pk.gold);
            else Spr::drawGroundItem(&p, pk.item, pk.pos, m_time, pk.item.rarity, clampf(pk.age * 2.6f, 0.f, 1.f));
            break;
        }
        default: break;
        }
    }

    for (int i = 0; i < m_render.size(); ++i) {
        const Actor &a = m_render[i];
        if (!view.contains(a.pos)) continue;
        if (a.isPlayer) continue;
        Spr::drawHealthPip(&p, a);
    }

    // росчерки ударов (взмахи, когти, волны, круги) - поверх существ
    for (int i = 0; i < m_afx.size(); ++i) {
        const AttackFx &fx = m_afx[i];
        if (!view.contains(fx.pos)) continue;
        Spr::drawAttackFx(&p, fx, m_time);
    }

    for (int i = 0; i < m_renderProjs.size(); ++i) {
        const Proj &pr = m_renderProjs[i];
        if (!view.contains(pr.pos)) continue;
        Spr::drawProjectile(&p, pr, m_time);
    }
    for (int i = 0; i < m_parts.size(); ++i) {
        if (!view.contains(m_parts[i].pos)) continue;
        Spr::drawParticle(&p, m_parts[i]);
    }
    for (int i = 0; i < m_texts.size(); ++i) {
        const FText &f = m_texts[i];
        if (!view.contains(f.pos)) continue;
        float k = clampf(f.life / f.maxLife, 0.f, 1.f);
        p.save();
        p.setOpacity(k);
        float pop = f.crit ? (1.f + 0.42f * qSin((1.f - k) * float(M_PI))) : 1.f;
        QFont fn = f.kind == 3 ? fontSmall(int(f.size) - 1) : fontNum(int(f.size * pop), f.crit);
        p.setFont(fn);
        QFontMetrics fm(fn);
        if (f.kind == 5 && !f.lines.isEmpty()) {
            // реплика NPC: «облачко» по размеру текста, многострочное
            int lineH = fm.height() + 2;
            int wMax = 0;
            for (const QString &s : f.lines) wMax = qMax(wMax, fm.width(s));
            int bw = wMax + 16, bh = int(f.lines.size()) * lineH + 10;
            QRectF bub(f.pos.x() - bw / 2.f, f.pos.y() - bh, bw, bh);
            p.setPen(QColor(0, 0, 0, 170));
            p.setBrush(QColor(12, 9, 18, 205));
            p.drawRoundedRect(bub, 6, 6);
            // хвостик к голове NPC
            QPainterPath tail;
            tail.moveTo(f.pos.x() - 5, bub.bottom());
            tail.lineTo(f.pos.x(), f.pos.y() + 2);
            tail.lineTo(f.pos.x() + 5, bub.bottom());
            tail.closeSubpath();
            p.drawPath(tail);
            p.setPen(f.c);
            qreal ty = bub.top() + 5;
            for (const QString &s : f.lines) {
                p.drawText(QRectF(bub.left() + 8, ty, wMax, lineH),
                           Qt::AlignLeft | Qt::AlignVCenter, s);
                ty += lineH;
            }
        } else {
            // урон/лечение и пр.: окно шире для длинных слов, без элидинга
            int w = fm.width(f.text);
            QRectF tr(f.pos.x() - (w + 20) / 2.f, f.pos.y() - 20, w + 20, 26);
            p.setPen(QColor(0, 0, 0, 200));
            p.drawText(tr, Qt::AlignCenter, f.text);
            p.setPen(f.c);
            p.drawText(tr.translated(-1, -1), Qt::AlignCenter, f.text);
        }
        p.restore();
    }

    // призрачные следы рывка
    for (int i = 0; i < m_ghosts.size(); ++i) {
        const Ghost &gh = m_ghosts[i];
        float k = clampf(gh.life / 0.30f, 0.f, 1.f);
        p.save();
        p.setOpacity(k * 0.38f);
        p.translate(gh.pos);
        p.rotate(qRadiansToDegrees(gh.facing) + 90.f);
        QColor gc = C::arcane;
        p.setBrush(gc);
        p.setPen(Qt::NoPen);
        QPainterPath clo;
        clo.moveTo(QPointF(-10, 6));
        clo.cubicTo(QPointF(-14, -14), QPointF(14, -14), QPointF(10, 6));
        clo.closeSubpath();
        p.drawPath(clo);
        p.restore();
    }

    drawFog(p);
    drawCanopy(p);

    p.restore();
    drawGodRays(p);

    drawLighting(&p);
    drawOverlayFx(&p);
    drawEdgeMarkers(&p);
    drawForeground(p);
}

// общий рисунок пятна: используется и для слоя декалей, и для кэша земли
static void paintDecalBlob(QPainter &d, int kind, float radius, const QColor &col, Rng &rr)
{
    d.setPen(Qt::NoPen);
    switch (kind) {
    case 0: {   // гарь
        QRadialGradient g(QPointF(0, 0), radius);
        QColor c(10, 8, 8, 150);
        g.setColorAt(0, c);
        c.setAlpha(0);
        g.setColorAt(1, c);
        d.setBrush(g);
        d.save(); d.scale(1.0, 0.62); d.drawEllipse(QPointF(0, 0), radius, radius); d.restore();
        d.setPen(QPen(QColor(6, 4, 4, 120), 1.6));
        for (int i = 0; i < 4; ++i) {
            float a = rr.unit() * 6.28f;
            d.drawLine(QPointF(0, 0), QPointF(qCos(a) * radius * rr.frange(0.5f, 1.1f),
                                              qSin(a) * radius * rr.frange(0.3f, 0.7f)));
        }
        break;
    }
    case 1:     // кровь
        for (int i = 0; i < 5; ++i) {
            QColor c = col;
            c.setAlpha(int(rr.frange(90, 170)));
            d.setBrush(c);
            d.drawEllipse(QPointF(rr.frange(-radius, radius) * 0.7f, rr.frange(-radius, radius) * 0.4f),
                          rr.frange(2.f, radius * 0.42f), rr.frange(1.5f, radius * 0.26f));
        }
        break;
    case 2: {   // росчерк клинка
        d.setBrush(Qt::NoBrush);
        QColor c = col;
        c.setAlpha(90);
        d.setPen(QPen(c, qMax(1.4f, radius * 0.16f), Qt::SolidLine, Qt::RoundCap));
        d.drawArc(QRectF(-radius, -radius * 0.6f, radius * 2, radius * 1.2f),
                  int(rr.frange(0, 360) * 16), 110 * 16);
        break;
    }
    case 3:     // след ноги
        d.setBrush(QColor(0, 0, 0, 46));
        d.save(); d.scale(1.0, 0.62);
        d.drawEllipse(QPointF(0, 0), radius * 0.5f, radius * 0.8f);
        d.restore();
        break;
    default: {  // ледяная корка
        QColor c = col;
        c.setAlpha(60);
        d.setBrush(c);
        d.save(); d.scale(1.0, 0.6f); d.drawEllipse(QPointF(0, 0), radius, radius); d.restore();
        d.setPen(QPen(QColor(230, 250, 255, 70), 1.0));
        d.drawLine(QPointF(-radius * 0.6f, 0), QPointF(radius * 0.5f, -radius * 0.15f));
        break;
    }
    }
}

void GameView::stampDecal(const QPointF &w, int kind, float radius, const QColor &col)
{
    if (m_decals.isNull() || m_quality == 0) return;
    // боевые следы (кровь/гарь/магия) живут ~3 секунды и тают: вечные пятна
    // превращали арену в кашу и тормозили перерисовку кэша земли
    LiveDecal ld;
    ld.pos = w; ld.kind = kind; ld.radius = radius; ld.col = col;
    ld.life = 3.0f;
    m_live.append(ld);
    if (m_live.size() > 120) m_live.removeFirst();   // жёсткий потолок на случай мясорубки
    m_liveDirty = true;                              // слой нужно переложить на этом тике
}

void GameView::updateLiveDecals(float dt)
{
    bool any = false;
    for (int i = m_live.size() - 1; i >= 0; --i) {
        m_live[i].life -= dt;
        if (m_live[i].life <= 0.f) { m_live.removeAt(i); continue; }
        any = true;
    }
    if (!any && !m_liveDirty) { m_liveDirty = false; return; }
    m_liveDirty = false;
    if (m_decals.isNull()) return;
    // перекладываем живые декали в слой: старые пятна сами исчезают
    m_decals.fill(Qt::transparent);
    QRectF bounds;
    QPainter d(&m_decals);
    d.setRenderHint(QPainter::Antialiasing, true);
    d.scale(0.5f, 0.5f);
    for (int i = 0; i < m_live.size(); ++i) {
        const LiveDecal &ld = m_live.at(i);
        float k = ld.life / 3.0f;                 // 1 -> 0 за жизнь
        float a = k > 0.6f ? 1.f : (k / 0.6f);    // последние 40% - плавное затухание
        d.save();
        d.setOpacity(a * 0.9f);
        d.translate(ld.pos);
        Rng rr(quint64(int(ld.pos.x() * 31 + ld.pos.y() * 17)) ^ 0xABCD);
        paintDecalBlob(d, ld.kind, ld.radius, ld.col, rr);
        d.restore();
        QRectF b(ld.pos.x() - ld.radius * 1.4f, ld.pos.y() - ld.radius * 1.4f,
                 ld.radius * 2.8f, ld.radius * 2.8f);
        bounds = bounds.isNull() ? b : bounds.united(b);
    }
    d.end();
    m_decalBounds = bounds;
}

void GameView::drawWaterReflections(QPainter &p)
{
    p.save();
    for (int i = 0; i < m_render.size(); ++i) {
        const Actor &a = m_render[i];
        if (a.dead) continue;
        if (m_dun.tileAtPx(a.pos) != T_Water) continue;
        Spr::Pal pal = Spr::palFor(a.tint);
        QColor c = a.isPlayer ? QColor(210, 190, 160, 70) : QColor(pal.main.red(), pal.main.green(), pal.main.blue(), 70);
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        float vr = a.baseRadius * a.drawScale;
        p.save();
        p.translate(a.pos.x(), a.pos.y() + vr * 0.35f);
        p.scale(1.0, -0.42);
        p.drawEllipse(QPointF(0, -vr * 0.9f), vr * 0.75f, vr * 1.05f);
        p.restore();
        float rp = fmodf(m_time * 0.8f + a.id * 0.37f, 1.6f);
        p.setPen(QPen(QColor(210, 240, 255, int(70 * (1.f - rp / 1.6f))), 1.0));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(a.pos, vr * (0.7f + rp), vr * 0.30f * (0.7f + rp));
    }
    p.restore();
}

void GameView::ambientEvent()
{
    Biome b = m_dun.biome;
    QRectF vis(m_cam.x(), m_cam.y(), float(width()), float(height()));
    switch (b) {
    case Bio_Frost: {
        for (int i = 0; i < 46; ++i) {
            Part pt;
            pt.pos = QPointF(vis.left() + rng().unit() * vis.width(), vis.top() - 20);
            pt.vel = QPointF(rng().frange(60, 150), rng().frange(90, 170));
            pt.life = pt.maxLife = rng().frange(1.2f, 2.4f);
            pt.size = rng().frange(1.2f, 2.6f);
            pt.c = QColor(235, 246, 255, 170);
            pt.kind = 0;
            m_parts.append(pt);
        }
        break;
    }
    case Bio_Lava:
    case Bio_Ash: {
        for (int i = 0; i < 30; ++i) {
            Part pt;
            pt.pos = QPointF(vis.left() + rng().unit() * vis.width(), vis.top() - 10);
            pt.vel = QPointF(rng().frange(-30, 30), rng().frange(60, 140));
            pt.life = pt.maxLife = rng().frange(1.f, 2.f);
            pt.size = rng().frange(1.f, 2.2f);
            pt.c = b == Bio_Lava ? QColor(255, 150, 50, 200) : QColor(190, 180, 170, 120);
            pt.kind = b == Bio_Lava ? 1 : 2;
            pt.grav = 40.f;
            m_parts.append(pt);
        }
        break;
    }
    case Bio_Void: {
        QPointF c = QPointF(vis.left() + rng().unit() * vis.width(), vis.top() + rng().unit() * vis.height());
        ring(c, 26, C::arcane, 130.f);
        flashScreen(C::arcane, 0.10f);
        Audio::inst().play(Sfx_Cast);
        break;
    }
    case Bio_Swamp: {
        QPointF c = QPointF(vis.left() + rng().unit() * vis.width(), vis.top() + rng().unit() * vis.height());
        burst(c, 22, QColor(150, 220, 110), 2, 40, 4.f, 1.6f);
        break;
    }
    case Bio_Ember: {
        for (int i = 0; i < 14; ++i) {
            Part pt;
            pt.pos = QPointF(vis.left() + rng().unit() * vis.width(), vis.top() - 10);
            pt.vel = QPointF(rng().frange(-20, 20), rng().frange(30, 70));
            pt.life = pt.maxLife = rng().frange(2.f, 3.5f);
            pt.size = rng().frange(1.6f, 2.8f);
            pt.c = QColor(150, 176, 84, 130);
            pt.kind = 3;
            pt.spin = rng().frange(-4, 4);
            m_parts.append(pt);
        }
        break;
    }
    default: break;
    }
}

void GameView::drawFog(QPainter &p)
{
    BiomePal pal = biomePalette(m_dun.biome);
    QRectF wv = worldViewRect();
    p.save();
    for (int i = 0; i < m_fog.size(); ++i) {
        const FogPatch &fp = m_fog[i];
        QRectF fr(fp.pos.x() - fp.r, fp.pos.y() - fp.r * 0.55f, fp.r * 2.f, fp.r * 1.1f);
        if (!wv.intersects(fr)) continue;
        float pulse = 0.75f + 0.25f * qSin(m_time * 0.5f + fp.seed);
        QColor c = pal.fog;
        c.setAlpha(255);
        Style::disc(&p, fp.pos, fp.r, fp.r * 0.55f, c, fp.alpha * pulse);
    }
    p.restore();
}

void GameView::buildCanopy()
{
    m_canopy = QImage();
    Biome b = m_dun.biome;
    if (b == Bio_Town) return;
    const int S = 512;
    QImage img(S, S, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    Rng r(quint64(m_dun.seed) ^ 0xCAFEF00Dull);
    BiomePal pal = biomePalette(b);
    auto wrap = [&](const QPointF &c, float rad, const QColor &col, int kind) {
        for (int ox = -1; ox <= 1; ++ox)
            for (int oy = -1; oy <= 1; ++oy) {
                QPointF q(c.x() + ox * S, c.y() + oy * S);
                p.setPen(Qt::NoPen);
                p.setBrush(col);
                if (kind == 0) p.drawEllipse(q, rad, rad * 0.72f);
                else if (kind == 1) {
                    QPainterPath lf;
                    lf.moveTo(q);
                    lf.cubicTo(q + QPointF(rad, -rad * 0.4f), q + QPointF(rad * 1.6f, rad * 0.5f), q + QPointF(rad * 0.4f, rad * 1.1f));
                    lf.cubicTo(q + QPointF(rad * 0.2f, rad * 0.5f), q + QPointF(-rad * 0.2f, rad * 0.3f), q);
                    p.drawPath(lf);
                } else {
                    QPolygonF sh;
                    sh << q + QPointF(0, -rad) << q + QPointF(rad * 0.4f, 0) << q + QPointF(0, rad * 0.6f) << q + QPointF(-rad * 0.4f, 0);
                    p.drawPolygon(sh);
                }
            }
    };
    int n = 26;
    for (int i = 0; i < n; ++i) {
        QPointF c(r.unit() * S, r.unit() * S);
        float rad = r.frange(14.f, 46.f);
        QColor col(6, 5, 10, int(r.frange(46, 92)));
        switch (b) {
        case Bio_Ember: case Bio_Swamp: wrap(c, rad, col, r.chance(0.6f) ? 1 : 0); break;
        case Bio_Crystal: case Bio_Void: wrap(c, rad * 0.8f, col, 2); break;
        case Bio_Frost: wrap(c, rad, QColor(10, 14, 22, int(r.frange(30, 60))), 0); break;
        default: wrap(c, rad * 0.7f, QColor(8, 6, 10, int(r.frange(26, 56))), r.chance(0.4f) ? 1 : 0); break;
        }
    }
    // свисающие нити/паутина
    p.setPen(QPen(QColor(6, 5, 10, 70), 1.2));
    for (int i = 0; i < 10; ++i) {
        float x = r.unit() * S;
        float y0 = r.unit() * S;
        QPainterPath th;
        th.moveTo(x, y0);
        th.cubicTo(x + r.frange(-14, 14), y0 + 30, x + r.frange(-14, 14), y0 + 60, x + r.frange(-10, 10), y0 + r.frange(50, 110));
        p.drawPath(th);
    }
    p.end();
    // запекаем прозрачность 0.55 прямо в пиксели (premultiplied: домножаем все каналы)
    for (int y = 0; y < img.height(); ++y) {
        QRgb *ln = reinterpret_cast<QRgb *>(img.scanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            int a = qAlpha(ln[x]);
            if (!a) continue;
            ln[x] = qRgba(qRed(ln[x]) * 55 / 100, qGreen(ln[x]) * 55 / 100,
                          qBlue(ln[x]) * 55 / 100, a * 55 / 100);
        }
    }
    m_canopy = img;
    Q_UNUSED(pal);
}

void GameView::drawCanopy(QPainter &p)
{
    if (m_canopy.isNull() || m_quality == 0) return;
    const int S = 512;
    float ox = -fmodf(FX(m_cam) * 1.12f, float(S));
    float oy = -fmodf(FY(m_cam) * 1.12f, float(S));
    // прозрачность запечена в саму плитку (см. buildCanopy) - без setOpacity дешевле
    for (float y = oy - S; y < height(); y += S)
        for (float x = ox - S; x < width(); x += S)
            p.drawImage(QPointF(x, y), m_canopy);
}

void GameView::drawGodRays(QPainter &p)
{
    if (m_quality == 0) return;
    p.save();
    p.setCompositionMode(QPainter::CompositionMode_Plus);
    QRectF vis(m_cam.x() - 100, m_cam.y() - 100, float(width()) + 200, float(height()) + 200);
    for (int i = 0; i < m_dun.props.size(); ++i) {
        const Prop &pr = m_dun.props.at(i);
        if (pr.lightRadius <= 0.f || !vis.contains(pr.pos)) continue;
        QPointF sp = toScreen(pr.pos);
        float base = m_time * 6.f + (pr.seed % 10);
        for (int k = 0; k < 5; ++k) {
            float a = base + k * 72.f + qSin(m_time * 0.7f + k) * 8.f;
            QColor c = pr.lightColor;
            c.setAlpha(14);
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            QPolygonF wedge;
            float len = pr.lightRadius * 1.5f;
            wedge << sp
                  << sp + QPointF(qCos(qDegreesToRadians(a - 7)) * len, qSin(qDegreesToRadians(a - 7)) * len * 0.6f)
                  << sp + QPointF(qCos(qDegreesToRadians(a + 7)) * len, qSin(qDegreesToRadians(a + 7)) * len * 0.6f);
            p.drawPolygon(wedge);
        }
    }
    p.restore();
}

void GameView::drawForeground(QPainter &p)
{
    if (m_hideUi) return;
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    float px = -FX(m_cam) * 0.06f, py = -FY(m_cam) * 0.045f;
    for (int i = 0; i < m_fg.size(); ++i) {
        const FgItem &fi = m_fg[i];
        float sx = fi.ax * (width() + 260) - 130 + fmodf(px, 260.f);
        float sy = fi.ay * height() + fmodf(py, 120.f) * 0.2f;
        float sway = qSin(m_time * 0.9f + fi.phase) * 3.5f;
        QColor dark(6, 5, 9, 215);
        p.save();
        p.translate(sx, sy);
        p.scale(fi.scale, fi.scale);
        p.setBrush(dark);
        p.setPen(QPen(QColor(6, 5, 9, 230), 2.0));
        if (fi.kind == 0) {          // свисающая ветвь сверху
            p.rotate(sway * 0.4f);
            QPainterPath br;
            br.moveTo(0, -40);
            br.cubicTo(20, 10, -10, 40, 26, 90);
            br.cubicTo(6, 60, 30, 30, -6, -40);
            br.closeSubpath();
            p.drawPath(br);
            for (int k = 0; k < 4; ++k) {
                QPainterPath lf;
                lf.moveTo(10 + k * 5, 20 + k * 18);
                lf.cubicTo(34 + k * 5, 26 + k * 18, 30 + k * 5, 44 + k * 18, 12 + k * 5, 40 + k * 18);
                lf.closeSubpath();
                p.drawPath(lf);
            }
        } else if (fi.kind == 1) {  // трава снизу
            p.rotate(sway * 0.25f);
            for (int k = 0; k < 6; ++k) {
                QPainterPath bl;
                float bx = (k - 2.5f) * 14;
                bl.moveTo(bx, 60);
                bl.cubicTo(bx + sway, 10, bx + sway * 2 - 8, -30, bx + sway * 2.4f, -70 - k * 6);
                bl.cubicTo(bx + sway * 2 + 6, -28, bx + sway + 8, 12, bx + 10, 60);
                bl.closeSubpath();
                p.drawPath(bl);
            }
        } else {                    // цепь с фонарём
            p.rotate(sway * 0.5f);
            p.setPen(QPen(dark, 5.0));
            p.drawLine(QPointF(0, -60), QPointF(0, 40));
            for (int k = 0; k < 5; ++k)
                p.drawEllipse(QPointF(0, -50 + k * 20), 5, 7);
            Style::glow(&p, QPointF(0, 52), 46.f, QColor(255, 180, 80), 0.35f + 0.1f * qSin(m_time * 7.f + fi.phase));
            p.setBrush(dark);
            p.setPen(QPen(QColor(6, 5, 9, 230), 2.0));
            p.drawEllipse(QPointF(0, 52), 12, 15);
            p.setBrush(QColor(255, 200, 110, 200));
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(0, 52), 6, 8);
        }
        p.restore();
    }
    p.restore();
}

// маска «дырки» света: чёрный диск с профилем альфы 255 -> 200 -> 70 -> 0
// (тот же, что был у градиента), но готовится один раз
static const QImage &lightHoleTex()
{
    static QImage img;
    if (!img.isNull()) return img;
    const int S = 128;
    QImage t(S, S, QImage::Format_ARGB32_Premultiplied);
    const float h = S * 0.5f;
    for (int y = 0; y < S; ++y) {
        QRgb *ln = reinterpret_cast<QRgb *>(t.scanLine(y));
        for (int x = 0; x < S; ++x) {
            float dx = (x + 0.5f - h) / h, dy = (y + 0.5f - h) / h;
            float d = qSqrt(dx * dx + dy * dy);
            float a = 0.f;
            if (d < 0.45f)      a = 255.f - (255.f - 200.f) * (d / 0.45f);
            else if (d < 0.78f) a = 200.f - (200.f - 70.f) * ((d - 0.45f) / 0.33f);
            else if (d < 1.f)   a = 70.f * (1.f - (d - 0.78f) / 0.22f);
            ln[x] = qPremultiply(qRgba(0, 0, 0, int(a)));
        }
    }
    img = t;
    return img;
}

void GameView::drawLighting(QPainter *p)
{
    if (m_lightQ <= 0) return;   // свет выключен в настройках - никакой темноты
    BiomePal pal = biomePalette(m_dun.biome);
    float dark = pal.darkness;
    if (dark <= 0.03f) return;
    // маска света мягкая и меняется плавно - на высоком качестве пересобираем её
    // через кадр (30 Гц), в промежутках просто кладем готовую: вдвое дешевле
    ++m_lightFrame;
    const bool hq = (m_lightQ == 2 && m_quality != 0);
    const bool refresh = !hq || (m_lightFrame & 1) == 0 || m_lightUp.size() != size();
    if (!refresh && m_lightUp.size() == size()) {
        p->drawImage(QPoint(0, 0), m_lightUp);
        m_vigBaked = true;   // виньетка уже запечена в маску
        return;
    }
    // высокое качество: div 2 (или 3 в авто-облегчении), низкое: div 4 и меньше источников
    int div = (m_lightQ == 2) ? ((m_quality == 1) ? 2 : 3) : 4;
    QSize sz(width() / div, height() / div);
    if (sz.width() < 4 || sz.height() < 4) return;
    if (m_light.size() != sz) m_light = QImage(sz, QImage::Format_ARGB32_Premultiplied);

    int alpha = int(dark * 196.f);
    m_light.fill(QColor(4, 3, 8, alpha));

    QPainter lp(&m_light);
    lp.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    QPointF off = m_cam;
    float zdiv = m_zoom / float(div);

    struct Hole { QPointF w; float r; float pw; };
    QVector<Hole> holes;

    const Actor *pl = 0;
    for (int i = 0; i < m_render.size(); ++i) if (m_render[i].isPlayer) { pl = &m_render[i]; break; }
    if (pl) {
        float rr = 335.f + (m_game->shrine() == Shrine_Arcane ? 60.f : 0.f);
        holes.append(Hole{ pl->pos, rr, 1.0f });
    }
    QRectF lview = worldViewRect().adjusted(-160, -160, 160, 160);
    for (int i = 0; i < m_dun.props.size(); ++i) {
        const Prop &pr = m_dun.props.at(i);
        if (pr.lightRadius <= 0.f) continue;
        if (!lview.contains(pr.pos)) continue;
        float flick = (pr.kind == Pk_Brazier || pr.kind == Pk_Vent)
                ? 0.86f + 0.14f * qSin(m_time * 9.f + pr.seed % 10) : 1.f;
        holes.append(Hole{ pr.pos + QPointF(0, -pr.height * 0.5f), pr.lightRadius * flick, 0.9f * flick });
    }
    if (m_lightQ == 2) {
        // низкое качество света не тратится на дыры от зон, снарядов и тел
        for (int i = 0; i < m_zones.size(); ++i) {
            const Zone &z = m_zones[i];
            if (z.delay > 0.f) continue;
            if (z.kind == 0 || z.kind == 3 || z.kind == 4)
                holes.append(Hole{ z.pos, z.radius * 1.5f, 0.75f });
        }
        for (int i = 0; i < m_projs.size(); ++i) {
            const Proj &pr = m_projs[i];
            if (pr.kind == 1 || pr.kind == 3 || pr.kind == 5)
                holes.append(Hole{ pr.pos, 90.f, 0.6f });
        }
        for (int i = 0; i < m_render.size(); ++i) {
            const Actor &a = m_render[i];
            if (a.dead) continue;
            if (a.boss) holes.append(Hole{ a.pos + QPointF(0, -a.radius * 2), a.radius * 7.f, 0.7f });
            else if (a.tint == 24 || a.tint == 32) holes.append(Hole{ a.pos, 110.f, 0.6f });
        }
    }
    holes.append(Hole{ m_dun.exitPos + QPointF(0, -30), m_portalActive ? 170.f : 70.f, 0.8f });

    lp.setRenderHint(QPainter::SmoothPixmapTransform, true);
    for (int i = 0; i < holes.size(); ++i) {
        const Hole &h = holes.at(i);
        QPointF sp((h.w.x() - off.x()) * zdiv, (h.w.y() - off.y()) * zdiv);
        float rr = h.r * zdiv;
        if (sp.x() + rr < 0 || sp.y() + rr < 0 || sp.x() - rr > sz.width() || sp.y() - rr > sz.height()) continue;
        lp.setOpacity(h.pw);
        lp.drawImage(QRectF(sp.x() - rr, sp.y() - rr, rr * 2, rr * 2), lightHoleTex());
    }
    lp.setOpacity(1.0);
    // цветной «туман» биома запекаем прямо в маску (в низком разрешении это дёшево,
    // а полноэкранный Plus-fill стоил бы несколько миллисекунд каждый кадр)
    if (pal.fog.alpha() > 0) {
        lp.setCompositionMode(QPainter::CompositionMode_Plus);
        QColor fc(pal.fog.red(), pal.fog.green(), pal.fog.blue(),
                  qBound(0, int(pal.fog.alpha() * 0.13f), 255));
        lp.fillRect(QRect(0, 0, sz.width(), sz.height()), fc);
        lp.setCompositionMode(QPainter::CompositionMode_SourceOver);
    }
    lp.end();

    // двухпроходное масштабирование QImage заметно быстрее «гладкого» drawImage;
    // на низком качестве света растягиваем быстро (маска и так грубая)
    const Qt::TransformationMode tm = hq ? Qt::SmoothTransformation : Qt::FastTransformation;
    m_lightUp = m_light.scaled(width(), height(), Qt::IgnoreAspectRatio, tm);
    {
        // виньетка печётся прямо в маску света: один полноэкранный проход вместо двух
        QPainter up(&m_lightUp);
        Style::vignette(&up, QRectF(0, 0, float(width()), float(height())), 0.55f);
        up.end();
    }
    m_vigBaked = true;
    p->drawImage(QPoint(0, 0), m_lightUp);
}

QPointF GameView::debugNearestHostile() const
{
    const Actor *pl = 0;
    for (int i = 0; i < m_actors.size(); ++i) if (m_actors[i].isPlayer) { pl = &m_actors[i]; break; }
    if (!pl) return QPointF(-1, -1);
    const Actor *best = 0;
    float bd = 1e9f;
    for (int i = 0; i < m_actors.size(); ++i) {
        const Actor &a = m_actors[i];
        if (a.dead || a.isPlayer || a.isSummon) continue;
        float dd = distf(a.pos, pl->pos);
        if (dd < bd) { bd = dd; best = &a; }
    }
    return best ? best->pos : QPointF(-1, -1);
}

QPointF GameView::debugBossPos() const
{
    for (int i = 0; i < m_actors.size(); ++i)
        if (m_actors[i].boss && !m_actors[i].dead) return m_actors[i].pos;
    return QPointF(-1, -1);
}

QPointF GameView::debugNpcCenter() const
{
    if (m_npcs.isEmpty()) return QPointF(-1, -1);
    QPointF s(0, 0);
    for (int i = 0; i < m_npcs.size(); ++i) s += m_npcs.at(i).pos;
    return s / float(m_npcs.size());
}

void GameView::drawEdgeMarkers(QPainter *p)
{
    if (m_portalActive || m_hideUi || m_playerDead) return;
    Actor *pl = player();
    if (!pl) return;
    QRectF wv = worldViewRect();
    QRectF inner = wv.adjusted(34 / m_zoom, 34 / m_zoom, -34 / m_zoom, -34 / m_zoom);
    float cx = float(width()) * 0.5f, cy = float(height()) * 0.5f;
    for (int i = 0; i < m_render.size(); ++i) {
        const Actor &a = m_render[i];
        if (a.dead || a.isPlayer || a.isSummon) continue;
        if (inner.contains(a.pos)) continue;
        QPointF sp0 = toScreen(a.pos);
        float dx = FX(sp0) - cx, dy = FY(sp0) - cy;
        float len = qSqrt(dx * dx + dy * dy);
        if (len < 1.f) continue;
        dx /= len; dy /= len;
        // точка на границе экрана
        float sx = float(width()) * 0.5f, sy = float(height()) * 0.5f;
        float kx = (sx - 26.f) / qMax(0.0001f, qAbs(dx));
        float ky = (sy - 26.f) / qMax(0.0001f, qAbs(dy));
        float k = qMin(kx, ky);
        QPointF sp(sx + dx * k, sy + dy * k);
        Q_UNUSED(sp0);
        QColor c = a.boss ? QColor(255, 80, 60) : (a.elite ? C::amber : QColor(226, 110, 90));
        float dd = distf(a.pos, pl->pos);
        float alpha = a.boss ? 1.f : clampf(1.f - dd / 2600.f, 0.25f, 1.f);
        p->save();
        p->setOpacity(alpha);
        p->translate(sp);
        p->rotate(qRadiansToDegrees(qAtan2(dy, dx)) + 90.f);
        p->setBrush(c);
        p->setPen(QPen(QColor(0, 0, 0, 190), 1.4));
        QPolygonF tri;
        float sz = a.boss ? 13.f : 9.f;
        tri << QPointF(0, -sz) << QPointF(sz * 0.8f, sz * 0.7f) << QPointF(0, sz * 0.3f) << QPointF(-sz * 0.8f, sz * 0.7f);
        p->drawPolygon(tri);
        p->restore();
        if (a.boss) Style::glow(p, sp, 30.f, c, 0.6f);
    }
}

void GameView::drawOverlayFx(QPainter *p)
{
    const Derived &d = m_game->d();
    Actor *pl = player();
    p->save();

    // виньетка (если свет выключен - рисуем отдельно; иначе она запечена в маску света)
    if (!m_vigBaked)
        Style::vignette(p, QRectF(0, 0, width(), height()), 0.55f);
    m_vigBaked = false;

    // пульс при низком здоровье
    if (pl && !m_playerDead && d.maxHp > 0) {
        float t = m_game->hp() / d.maxHp;
        if (t < 0.32f) {
            float pulse = 0.5f + 0.5f * qSin(m_time * 5.5f);
            float a = (0.32f - t) / 0.32f;
            QRadialGradient g(QPointF(width() * 0.5, height() * 0.5), qMax(width(), height()) * 0.6f);
            g.setColorAt(0.45, QColor(180, 20, 20, 0));
            g.setColorAt(1.0, QColor(180, 20, 20, int(150 * a * (0.5f + 0.5f * pulse))));
            p->fillRect(rect(), QBrush(g));
        }
    }
    // вспышка урона / событий
    if (m_flash > 0.01f) {
        QColor c = m_flashCol;
        c.setAlphaF(clampf(m_flash, 0.f, 1.f) * 0.5f);
        p->fillRect(rect(), c);
    }
    // затемнение при входе в локацию
    if (m_fadeIn > 0.01f)
        p->fillRect(QRectF(0, 0, float(width()), float(height())), QColor(4, 3, 8, int(255 * clampf(m_fadeIn, 0.f, 1.f))));

    // баннер названия локации
    if (m_bannerT > 0.f && m_bannerT < 2.85f && !m_playerDead) {
        float tIn = easeOutCubic(clampf((2.85f - m_bannerT) / 0.45f, 0.f, 1.f));
        float tOut = clampf(m_bannerT / 0.55f, 0.f, 1.f);
        float a = qMin(tIn, tOut);
        float slide = (1.f - tIn) * 46.f;
        p->save();
        p->setOpacity(a);
        QRectF br(0, height() * 0.30f + slide, width(), 60);
        QColor bg(6, 4, 10, 150);
        p->fillRect(QRectF(0, br.top() - 8, width(), br.height() + 16), bg);
        Style::textShadow(p, br, m_bannerText, C::amber, fontTitle(34, true), Qt::AlignCenter);
        Style::textShadow(p, QRectF(0, br.bottom() + 2, width(), 20), m_bannerSub, C::boneDim, fontHead(13), Qt::AlignCenter);
        Style::divider(p, br.bottom() + 30, width() * 0.32f, width() * 0.68f, QColor(201, 146, 46, int(180 * a)));
        p->restore();
    }

    // интро босса
    if (m_bossIntroT > 0.f) {
        float tIn = easeOutCubic(clampf((2.6f - m_bossIntroT) / 0.4f, 0.f, 1.f));
        float tOut = clampf(m_bossIntroT / 0.5f, 0.f, 1.f);
        float a = qMin(tIn, tOut);
        float slide = (1.f - tIn) * width() * 0.5f;
        p->save();
        p->setOpacity(a);
        QRectF br(-slide, height() * 0.62f, width(), 64);
        p->fillRect(QRectF(0, br.top() - 10, width(), br.height() + 20), QColor(30, 4, 6, 170));
        Style::textShadow(p, br, m_bossName, C::bloodHi, fontTitle(38, true), Qt::AlignCenter);
        Style::textShadow(p, QRectF(0, br.bottom() + 2, width(), 18),
                          QStringLiteral("босс региона"), C::boneDim, fontHead(12), Qt::AlignCenter);
        p->restore();
    }

    // экран смерти
    if (m_playerDead) {
        float k = clampf(m_deathTimer * 0.7f, 0.f, 1.f);
        p->fillRect(rect(), QColor(10, 2, 4, int(190 * k)));
    }
    p->restore();
}

} // namespace aa
