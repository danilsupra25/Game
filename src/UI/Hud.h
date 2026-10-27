#ifndef AA_HUD_H
#define AA_HUD_H

#include <QWidget>
#include <QTimer>
#include "Core/Types.h"

namespace aa {

class Game;
class GameView;

// Полностью рисовальный оверлей поверх игровой сцены.
class Hud : public QWidget
{
    Q_OBJECT
public:
    Hud(Game *game, GameView *view, QWidget *parent = 0);

    void setLogVisible(bool v) { m_showLog = v; }
    void pushLog(const QString &msg, const QColor &c);
    void notifyLevelUp(int level);
    void notifyLoot(const QString &name, const QColor &c);
    void notifyAch(const QString &name, int glyph);
    QRectF hotbarSlotRect(int i) const;
    int hotbarSlotAt(const QPointF &p) const;
    bool orbHpContains(const QPointF &p) const;
    bool orbMpContains(const QPointF &p) const;

protected:
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);

private:
    struct LogLine { QString text; QColor color; float life; };
    struct Notify { QString text; QColor color; float life; int glyph; };
    struct AchNote { QString text; float life; int glyph; };

    void drawOrbs(QPainter *p);
    void drawHotbar(QPainter *p);
    void drawTopLeft(QPainter *p);
    void drawTopRight(QPainter *p);
    void drawBossBar(QPainter *p);
    void drawBuffs(QPainter *p);
    void drawLog(QPainter *p);
    void drawPrompt(QPainter *p);
    void drawNotifications(QPainter *p);
    void drawStreak(QPainter *p);
    void drawAchBanner(QPainter *p);

    Game *m_game;
    GameView *m_view;
    QTimer *m_timer;
    QVector<LogLine> m_log;
    QVector<Notify> m_notes;
    QVector<AchNote> m_ach;
    bool m_showLog = true;
    float m_time = 0.f;
    float m_dHp = -1.f, m_dMp = -1.f;   // отображаемые значения (сглаженные)
    float m_levelUpFx = 0.f;
    int m_levelUpValue = 0;
};

} // namespace aa

#endif // AA_HUD_H
