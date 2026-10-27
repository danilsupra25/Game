#ifndef AA_GAMEVIEW_H
#define AA_GAMEVIEW_H

#include <QWidget>
#include <QVector>
#include <QSet>
#include <QElapsedTimer>
#include <QTimer>
#include <QImage>
#include <QPointF>
#include <QPair>

#include "Core/Types.h"
#include "Core/Game.h"
#include "Game/Actors.h"
#include "World/Dungeon.h"

#ifdef HAVE_GL
namespace aa { class GlFxView; }
#endif

namespace aa {

class GameView : public QWidget
{
    Q_OBJECT
public:
    explicit GameView(Game *game, QWidget *parent = 0);
    ~GameView();

    void loadLocation(int locId);
    void clearScene();

    void setPaused(bool p);
    bool isPaused() const { return m_paused; }
    void setInteractive(bool on) { m_interactive = on; if (on) setFocus(Qt::OtherFocusReason); }
    bool interactive() const { return m_interactive; }
    void setHideUi(bool h) { m_hideUi = h; }
    void setShakeEnabled(bool on) { m_shakeEnabled = on; }
    void setPixelMode(int scale) { m_pxMode = qBound(1, scale, 4); }   // 1 выкл, 2..4 крупность пикселя
    void setPostFx(bool on);          // GL-постобработка (CRT/bloom/дайтеринг), с фолбэком
    bool postFx() const { return m_fxEnabled; }
    const QImage &sceneImage() const { return m_sceneImg; }
    int pixelMode() const { return m_pxMode; }
    void setScanlines(bool on) { m_scanlines = on; }
    bool scanlines() const { return m_scanlines; }
    bool shakeEnabled() const { return m_shakeEnabled; }
    // качество света: 0 выкл, 1 низкое, 2 высокое (настройки видео)
    void setLightQ(int q) { m_lightQ = qBound(0, q, 2); }
    int  lightQ() const { return m_lightQ; }
    // тени: 0 выкл, 1 мягкие контактные пятна под существами и предметами
    void setShadowQ(int q);
    int  shadowQ() const { return m_shadowQ; }
    // авто-подстройка качества эффектов при просадке FPS (настройка видео)
    void setAutoQuality(bool on) { m_autoQuality = on; }
    bool autoQuality() const { return m_autoQuality; }
    // миникарта в HUD (настройка видео)
    void setMinimapVisible(bool on) { m_minimapOn = on; }
    bool minimapVisible() const { return m_minimapOn; }

    Game *game() const { return m_game; }
    const Dungeon &dungeon() const { return m_dun; }
    void rebakeGround();   // перепечь кэш земли (после вкл/выкл спрайтов-тайлсетов)
    void ensureGroundCache(float z);   // пересобрать кэш земли под зум, если нужно
    int currentLocation() const { return m_locId; }
    float time() const { return m_time; }

    Actor *player();
    bool playerDead() const { return m_playerDead; }
    void respawn();

    int enemiesAlive() const;
    int enemiesTotal() const { return m_totalEnemies; }
    int enemiesKilled() const { return m_killed; }
    QString objectiveText() const;
    float objectiveProgress() const;
    bool portalActive() const { return m_portalActive; }

    bool bossActive() const { return m_bossId >= 0; }
    QString bossName() const { return m_bossName; }
    float bossHp() const { return m_bossHp; }
    float bossMaxHp() const { return m_bossMaxHp; }
    int   bossPhase() const { return m_bossPhase; }

    QVector<QPair<int, float> > activeBuffs() const;   // глиф + оставшееся время

    // служебный метод для автоматических тестов: продвинуть симуляцию на dt секунд
    void debugStep(float dt);

    void renderMinimap(QPainter *p, const QRectF &r);
    void renderTargetInfo(QPainter *p, const QRectF &r);

    // отладка/тесты: ближайший враг и счётчики
    QPointF debugNearestHostile() const;
    QPointF debugBossPos() const;      // позиция босса (-1,-1 если нет)
    QPointF debugNpcCenter() const;   // центр группы NPC (для шотов города)
    int debugActorCount() const { return m_actors.size(); }
    float huntTime() const { return m_huntT; }
    int streak() const { return m_streak; }
    float streakTime() const { return m_streakT; }
    int quality() const { return m_quality; }

    QPointF aimWorld() const { return m_aim; }
    QPointF cameraTopLeft() const { return m_cam; }
    void centerCamera();
    float zoom() const { return m_zoom; }
    void setZoom(float z);
    QPointF toWorld(const QPointF &screen) const { return m_cam + QPointF(screen.x() / m_zoom, screen.y() / m_zoom); }
    QPointF toScreen(const QPointF &world) const { return QPointF((world.x() - m_cam.x()) * m_zoom, (world.y() - m_cam.y()) * m_zoom); }
    QRectF worldViewRect() const;

    float cooldownOf(int slot) const { return (slot >= 0 && slot < 6) ? m_skillCd[slot] : 0.f; }
    QString prompt() const { return m_prompt; }
    int souls() const { return m_souls; }
    float fps() const { return m_fps; }
    int   projectilesCount() const { return m_projs.size(); }
    int   particlesCount() const { return m_parts.size(); }

signals:
    void locationCleared(int locId);
    void playerDiedSignal();
    void bossEngaged(int defId, QString name);
    void bossHealthChanged(float cur, float max, int phase);
    void bossDefeated();
    void logMsg(QString msg, QColor color);
    void lootPicked(QString name, QColor color);
    void requestExitToMap();
    void requestShop();
    void requestHeal();
    void requestRespec();
    void requestShrine(int kind);
    void promptChanged(QString text);
    void uiKeyPressed(int key);
    void statsDirty();
    void postFxFailed();
    void objectiveChanged();

public slots:
    void onFrame();
    void onFxFailed();

protected:
    void paintEvent(QPaintEvent *);
    void keyPressEvent(QKeyEvent *);
    void keyReleaseEvent(QKeyEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void wheelEvent(QWheelEvent *);
    void resizeEvent(QResizeEvent *);
    void focusInEvent(QFocusEvent *);
    void focusOutEvent(QFocusEvent *);

private:
    // --- цикл ---
    void updateSim(float dt);
    void updatePlayer(float dt);
    void updateEnemies(float dt);
    void updateSummons(float dt);
    void updateProjectiles(float dt);
    void updateZones(float dt);
    void updateTelegraphs(float dt);
    void updateParticles(float dt);
    void updatePickups(float dt);
    void updateNpcs(float dt);
    void updateBoss(Actor &a, float dt);
    void updateCamera(float dt);
    void clampCamera();
    void spawnAmbient(float dt);
    void stampDecal(const QPointF &w, int kind, float radius, const QColor &col);
    void drawWaterReflections(QPainter &p);
    void ambientEvent();
    void drawCanopy(QPainter &p);
    void drawGodRays(QPainter &p);
    void buildCanopy();
    void breakPropsArc(const QPointF &from, const QPointF &dir, float range, float halfAng);
    void breakPropsRadius(const QPointF &pos, float radius);
    void openOrBreakProp(int idx);
    void drawFog(QPainter &p);
    void drawForeground(QPainter &p);
    bool heldAct(int act) const;

    // --- бой ---
    void basicAttack();
    void castSkill(int slotIdx);
    void dodgeRoll();
    void fireProjectile(const QPointF &from, const QPointF &dir, float dmg, DmgType type, int owner,
                        bool hostile, int kind, float speed, float radius, int pierce, int chains,
                        const QColor &col, float scale, bool explode, float explodeRadius, float slow,
                        float dotDps, int glyph);
    void damageActor(Actor &tgt, float amount, DmgType type, int sourceId, bool canCrit, float knock,
                     const QPointF &from, bool isPlayerSource, bool silent = false);
    void healActor(Actor &a, float amount);
    void killActor(Actor &a, int killerId);
    void applyStatus(Actor &a, DmgType type, float power, float dur);
    void onActorHit(Actor &tgt, Actor *src);

    // --- умения ---
    void doSkillFx(const ActiveSkill &s, int rank);

    // --- спавн ---
    void spawnEnemy(int defId, const QPointF &pos, bool elite, int levelDelta);
    void spawnSummon(int defId, const QPointF &pos, int owner, float lifeTime, float dmgMul);
    int  newActorId() { return ++m_nextId; }
    Actor *findById(int id);
    Actor *nearestHostile(const QPointF &p, float maxDist, bool forPlayer = true);
    QVector<Actor *> enemiesInRadius(const QPointF &p, float r);

    // --- лут ---
    void dropLoot(const QPointF &pos, int ilvl, float dropChance, int goldMin, int goldMax, bool boss);
    void pickupNear();

    // --- движение ---
    bool blockedAt(const QPointF &p, float radius) const;
    void moveActor(Actor &a, const QPointF &delta);
    void resolveGeometry(Actor &a);
    void checkPlayerDeath();
    void separateActors(float dt);

    // --- эффекты ---
    void burst(const QPointF &pos, int count, const QColor &c, int kind, float speed, float size, float life);
    void ring(const QPointF &pos, int count, const QColor &c, float radius);
    void ftext(const QPointF &pos, const QString &text, const QColor &c, bool crit = false, int kind = 0, float size = 15.f);
    void shake(float amount);
    void flashScreen(const QColor &c, float amount);
    // росчерк удара: kind 0 взмах, 1 секира, 2 когти, 3 волна, 4 выпад, 5 круг колдовства
    void spawnAttackFx(const QPointF &pos, float ang, int kind, const QColor &col,
                       float scale, float dur = -1.f, bool flip = false);
    void updateAttackFx(float dt);
    int fxKindForActor(const Actor &a) const;   // когти/взмах/волна по архетипу существа

    // --- отрисовка ---
    void drawWorld(QPainter *p);
    void drawLighting(QPainter *p);
    void drawOverlayFx(QPainter *p);
    void drawEdgeMarkers(QPainter *p);
    void renderFrame(QPainter &p);
    void paintScene(QPainter &p);
    void drawScanlines(QPainter &p);
    void drawCursor(QPainter &p);
    void buildMinimap();

    void interact();
    void checkObjective();
    void syncPlayerStats();
    void updatePrompt();
    float playerSkillDamage(const ActiveSkill &s, int rank) const;

private:
    Game *m_game;
    Dungeon m_dun;
    int m_locId = -1;

    QVector<Actor> m_actors;
    QVector<Proj> m_projs;
    QVector<Part> m_parts;
    QVector<AttackFx> m_afx;   // росчерки ударов/взмахов (полумесяцы, когти, волны)
    QVector<FText> m_texts;
    QVector<Zone> m_zones;
    QVector<Telegraph> m_tels;
    QVector<Pickup> m_pickups;
    QVector<Npc> m_npcs;
    struct PendingSpawn { int defId; QPointF pos; bool elite; int levelDelta; int owner; float life; float dmgMul; bool summon; int raiserId;
        PendingSpawn() : defId(-1), elite(false), levelDelta(0), owner(-1), life(0.f), dmgMul(1.f), summon(false), raiserId(-1) {} };
    QVector<PendingSpawn> m_pending;
    struct FogPatch { QPointF pos, vel; float r, alpha; int seed; };
    QVector<FogPatch> m_fog;
    struct FgItem { float ax, ay; int kind; float scale; float phase; };
    QVector<FgItem> m_fg;
    struct Ghost { QPointF pos; float life; float facing; };
    QVector<Ghost> m_ghosts;
    QImage m_decals;            // полурез-слой стойких следов (гарь, кровь, шаги, лёд)
    QImage m_canopy;            // тайл верхнего полога (параллакс над существами)
    float m_envT = 6.f;         // таймер погодных событий биома

    int m_nextId = 0;
    int m_playerId = -1;
    bool m_playerDead = false;
    float m_deathTimer = 0.f;

    int m_totalEnemies = 0;
    int m_killed = 0;
    bool m_portalActive = false;
    int m_bossId = -1;
    QString m_bossName;
    float m_bossHp = 0.f, m_bossMaxHp = 1.f;
    int m_bossPhase = 1;

    float m_time = 0.f;
    float m_timeScale = 1.f;
    bool m_paused = true;
    bool m_interactive = false;
    bool m_hideUi = false;
    bool m_shakeEnabled = true;
    int m_pxMode = 1;
    bool m_scanlines = false;
    QImage m_buf;
    bool m_fxEnabled = false;
    QImage m_sceneImg;
#ifdef HAVE_GL
    GlFxView *m_fx = 0;
#endif

    QSet<int> m_keys;
    QPointF m_mouse;          // экранные координаты
    QPointF m_aim;            // мировые
    bool m_lmb = false, m_rmb = false;

    QPointF m_cam;
    float m_zoom = 1.f;
    float m_shake = 0.f;
    QPointF m_shakeOff;
    QColor m_flashCol;
    float m_flash = 0.f;

    float m_huntT = 0.f;      // время в локации: враги начинают искать игрока
    float m_fadeIn = 0.f;     // затемнение при входе в локацию
    float m_bannerT = 0.f;    // баннер названия локации
    QString m_bannerText, m_bannerSub;
    float m_bossIntroT = 0.f; // интро босса
    bool m_bossIntroDone = false;
    float m_stepT = 0.f;      // фаза шагов для пыли
    int m_streak = 0;         // серия убийств
    float m_streakT = 0.f;
    float m_locTime = 0.f;    // время текущей зачистки
    float m_locDmgTaken = 0.f;// полученный урон за локацию
    QVector<float> m_killTimes; // окно для «тройного выброса»

    float m_rollCd = 0.f;
    float m_attackCd = 0.f;
    float m_skillCd[6];
    float m_potionCd = 0.f;
    int   m_souls = 0;        // заряды «Жатвы пепла»
    float m_dashCharges = 1.f;
    float m_shadowStepT = 0.f;

    QImage m_minimap;
    QImage m_light;
    // кэш земли в масштабе текущего зума (земля + декали): рисуется 1:1 без
    // дорогого гладкого масштабирования каждый кадр
    QImage m_groundZ;
    float m_groundZZoom = -1.f;
    QImage m_lightUp;          // растянутая маска света (обновляется через кадр)
    qint64 m_lightFrame = 0;
    bool m_vigBaked = false;   // виньетка запечена в маску света в этом кадре
    bool m_groundZDirty = true;
    QRectF m_decalBounds;      // мир: область, где есть следы (для отсечения)

    QTimer *m_timer;
    QElapsedTimer m_clock;
    float m_acc = 0.f;
    float m_alpha = 1.f;
    QVector<Actor> m_render;      // интерполированные копии актёров для рендера
    QVector<Proj> m_renderProjs;
    float m_fps = 60.f;
    float m_fpsAcc = 0.f;
    int m_fpsCount = 0;
    int m_quality = 1;        // 1 полное, 0 облегчённое (авто)
    bool m_autoQuality = true;   // разрешена ли авто-подстройка качества по FPS
    bool m_minimapOn = true;     // показывать миникарту в HUD
    float m_lowFpsT = 0.f;    // сколько секунд FPS держится низким
    float m_hiFpsT = 0.f;     // сколько секунд FPS с запасом
    float m_hitStop = 0.f;    // микро-пауза на критах
    int m_lightQ = 2;         // 0 выкл, 1 низкое, 2 высокое (настройка)
    int m_shadowQ = 1;        // 0 выкл, 1 мягкие контактные тени (настройка)

    bool m_shrineUsed = false;

    QString m_prompt;
    Npc *m_nearNpc;
    bool m_nearPortal = false;
    bool m_nearAltar = false;
};

} // namespace aa

#endif // AA_GAMEVIEW_H
