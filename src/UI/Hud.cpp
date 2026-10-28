#include "UI/Hud.h"
#include "UI/Tooltip.h"
#include "Core/Style.h"
#include "Core/Data.h"
#include "Core/Game.h"
#include "Game/GameView.h"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QFontMetrics>

namespace aa {

Hud::Hud(Game *game, GameView *view, QWidget *parent)
    : QWidget(parent), m_game(game), m_view(view)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_timer = new QTimer(this);
    m_timer->setInterval(33);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(update()));
    m_timer->start();
}

void Hud::pushLog(const QString &msg, const QColor &c)
{
    LogLine l;
    l.text = msg;
    l.color = c;
    l.life = 9.f;
    m_log.append(l);
    while (m_log.size() > 7) m_log.removeFirst();
}

void Hud::notifyLevelUp(int level)
{
    m_levelUpFx = 3.2f;
    m_levelUpValue = level;
}

void Hud::notifyAch(const QString &name, int glyph)
{
    AchNote n;
    n.text = name;
    n.life = 5.f;
    n.glyph = glyph;
    m_ach.append(n);
    while (m_ach.size() > 3) m_ach.removeFirst();
}

void Hud::notifyLoot(const QString &name, const QColor &c)
{
    Notify n;
    n.text = name;
    n.color = c;
    n.life = 2.6f;
    n.glyph = Gl_Star;
    m_notes.append(n);
    while (m_notes.size() > 5) m_notes.removeFirst();
}

QRectF Hud::hotbarSlotRect(int i) const
{
    float s = 56.f;
    float gap = 6.f;
    float totalW = 6.f * s + 5.f * gap;
    float x0 = (width() - totalW) * 0.5f;
    return QRectF(x0 + float(i) * (s + gap), height() - 22.f - s, s, s);
}

int Hud::hotbarSlotAt(const QPointF &p) const
{
    for (int i = 0; i < 6; ++i) if (hotbarSlotRect(i).contains(p)) return i;
    return -1;
}

bool Hud::orbHpContains(const QPointF &p) const
{
    return QRectF(20, height() - 20 - 116, 116, 116).contains(p);
}
bool Hud::orbMpContains(const QPointF &p) const
{
    return QRectF(width() - 20 - 116, height() - 20 - 116, 116, 116).contains(p);
}

void Hud::resizeEvent(QResizeEvent *e) { QWidget::resizeEvent(e); update(); }

void Hud::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    m_time += 0.033f;
    if (m_levelUpFx > 0.f) m_levelUpFx = qMax(0.f, m_levelUpFx - 0.033f);

    drawOrbs(&p);
    drawHotbar(&p);
    drawTopLeft(&p);
    drawTopRight(&p);
    drawBossBar(&p);
    drawBuffs(&p);
    drawLog(&p);
    drawPrompt(&p);
    drawNotifications(&p);
    drawAchBanner(&p);
    drawStreak(&p);

    // эффект повышения уровня
    if (m_levelUpFx > 0.f) {
        float k = m_levelUpFx / 3.2f;
        p.save();
        p.setOpacity(qMin(1.f, k * 2.f));
        QColor g = C::amber;
        g.setAlphaF(0.18f * k);
        p.fillRect(rect(), g);
        QFont f = fontTitle(46, true);
        QRectF r(0, height() * 0.30f, width(), 70);
        Style::textShadow(&p, r, QStringLiteral("УРОВЕНЬ %1").arg(m_levelUpValue), C::amber, f, Qt::AlignCenter);
        p.restore();
    }
}

void Hud::drawOrbs(QPainter *p)
{
    const Derived &d = m_game->d();
    float hpT = d.maxHp > 0 ? clampf(m_game->hp() / d.maxHp, 0.f, 1.f) : 0.f;
    float mpT = d.maxMana > 0 ? clampf(m_game->mana() / d.maxMana, 0.f, 1.f) : 0.f;
    // плавное «перетекание» сфер
    if (m_dHp < 0.f) { m_dHp = hpT; m_dMp = mpT; }
    float kk = qMin(1.f, 0.033f * 9.f);
    m_dHp += (hpT - m_dHp) * kk;
    m_dMp += (mpT - m_dMp) * kk;
    if (hpT > m_dHp) m_dHp = qMin(hpT, m_dHp + 0.033f * 0.5f);  // лечение не «опережает» полосу
    hpT = m_dHp; mpT = m_dMp;

    QRectF hpR(20, height() - 20 - 116, 116, 116);
    QRectF mpR(width() - 20 - 116, height() - 20 - 116, 116, 116);
    Style::orb(p, hpR, hpT, C::hp, C::hpHi, m_time);
    Style::orb(p, mpR, mpT, C::mana, C::manaHi, m_time * 0.8f);

    // числа
    QFont f = fontNum(13, true);
    Style::textShadow(p, QRectF(hpR.left() - 20, hpR.bottom() - 30, hpR.width() + 40, 20),
                      QStringLiteral("%1 / %2").arg(int(m_dHp * d.maxHp)).arg(int(d.maxHp)), C::bone, f, Qt::AlignCenter);
    Style::textShadow(p, QRectF(mpR.left() - 20, mpR.bottom() - 30, mpR.width() + 40, 20),
                      QStringLiteral("%1 / %2").arg(int(m_dMp * d.maxMana)).arg(int(d.maxMana)), C::bone, f, Qt::AlignCenter);

    // барьер поверх сферы здоровья
    if (m_game->shield() > 0.f) {
        float st = clampf(m_game->shield() / qMax(1.f, d.maxHp), 0.f, 1.f);
        p->save();
        p->setPen(QPen(C::manaHi, 3.0));
        p->setBrush(Qt::NoBrush);
        QRectF ar = hpR.adjusted(6, 6, -6, -6);
        p->drawArc(ar, 90 * 16, -int(360 * 16 * st));
        p->restore();
        Style::textShadow(p, QRectF(hpR.left() - 20, hpR.top() - 4, hpR.width() + 40, 18),
                          QStringLiteral("+%1").arg(int(m_game->shield())), C::manaHi, fontSmall(10), Qt::AlignCenter);
    }

    // зелья
    int pot = m_game->countPotions();
    QRectF potR(hpR.right() - 6, hpR.top() - 6, 34, 34);
    p->save();
    Style::panel(p, potR, 1, 0.95f);
    drawGlyph(p, Gl_PotionHp, potR.adjusted(4, 4, -4, -4), pot > 0 ? C::hpHi : C::ash);
    Style::textShadow(p, QRectF(potR.left() - 6, potR.bottom() - 2, potR.width() + 12, 14),
                      QString::number(pot), pot > 0 ? C::bone : C::ash, fontSmall(10), Qt::AlignCenter);
    p->restore();
    Style::textShadow(p, QRectF(potR.left() - 20, potR.top() - 16, potR.width() + 40, 14),
                      QStringLiteral("ПРОБЕЛ"), C::boneDim.darker(110), fontSmall(8), Qt::AlignCenter);

    // золото
    QRectF goldR(mpR.left() - 28, mpR.top() - 6, 34, 34);
    Style::panel(p, goldR, 1, 0.95f);
    drawGlyph(p, Gl_Coins, goldR.adjusted(4, 4, -4, -4), C::gold);
    Style::textShadow(p, QRectF(goldR.left() - 40, goldR.bottom() - 2, goldR.width() + 80, 14),
                      QString::number(m_game->gold()), C::gold, fontSmall(10), Qt::AlignCenter);
}

void Hud::drawHotbar(QPainter *p)
{
    float s = 56.f;
    QRectF base = hotbarSlotRect(0);
    QRectF back(base.left() - 12, base.top() - 12, 6 * s + 5 * 6 + 24, s + 24);
    Style::panel(p, back, 1, 0.92f);

    // полоса опыта над панелью
    QRectF xpR(back.left(), back.top() - 12, back.width(), 7);
    Style::bar(p, xpR, m_game->xpProgress(), C::xp, C::edge);
    Style::textShadow(p, QRectF(back.left(), back.top() - 30, back.width(), 16),
                      QStringLiteral("опыт %1 / %2").arg(int(m_game->xp())).arg(int(Game::xpForLevel(m_game->level()))),
                      C::xp.darker(110), fontSmall(9), Qt::AlignCenter);

    for (int i = 0; i < 6; ++i) {
        QRectF r = hotbarSlotRect(i);
        int sk = m_game->hotbar(i);
        const ActiveSkill *as = sk >= 0 ? Data::skill(sk) : 0;
        bool usable = as && m_game->mana() >= m_game->skillCost(sk);
        Style::slotCell(p, r, false, false, as != 0);
        if (as) {
            QColor col = usable ? as->color : as->color.darker(230);
            QRectF gr = r.adjusted(6, 6, -6, -6);
            Style::glow(p, gr.center(), 30.f, col, usable ? 0.25f : 0.05f);
            drawGlyph(p, as->glyph, gr, col, col.lighter(130), usable ? 0.25f : 0.f);
            // кулдаун
            float cd = m_view ? m_view->cooldownOf(i) : 0.f;
            float cdMax = m_game->skillCooldown(sk);
            if (cd > 0.f && cdMax > 0.f) {
                float k = clampf(cd / cdMax, 0.f, 1.f);
                p->save();
                p->setClipRect(r);
                p->fillRect(QRectF(r.left(), r.bottom() - r.height() * k, r.width(), r.height() * k),
                             QColor(6, 4, 10, 165));
                p->restore();
                Style::textShadow(p, r, QString::number(double(cd), 'f', cd > 2.f ? 0 : 1), C::bone, fontNum(14, true),
                                  Qt::AlignCenter);
            }
            if (!usable && cd <= 0.f) {
                p->save();
                p->setClipRect(r);
                p->fillRect(r, QColor(20, 30, 70, 110));
                p->restore();
            }
            // ранг
            int rank = m_game->skillRank(sk);
            Style::textShadow(p, QRectF(r.right() - 16, r.bottom() - 13, 15, 12), romanNum(rank),
                              C::amber, fontSmall(8), Qt::AlignRight);
            // стоимость
            Style::textShadow(p, QRectF(r.left() + 1, r.bottom() - 13, 24, 12),
                              QString::number(int(m_game->skillCost(sk))), C::manaHi, fontSmall(8), Qt::AlignLeft);
        } else {
            drawGlyph(p, Gl_None, r, C::ash);
            p->save();
            p->setOpacity(0.35f);
            Style::textShadow(p, r, QStringLiteral("пусто"), C::ash, fontSmall(9), Qt::AlignCenter);
            p->restore();
        }
        // номер клавиши
        QRectF kr(r.left() + 2, r.top() + 1, 14, 12);
        p->save();
        p->setBrush(QColor(10, 8, 14, 200));
        p->setPen(QPen(C::edge, 1.0));
        p->drawRect(kr);
        Style::textShadow(p, kr, QString::number(i + 1), C::boneDim, fontSmall(9), Qt::AlignCenter);
        p->restore();
    }
}

void Hud::drawTopLeft(QPainter *p)
{
    QRectF panel(14, 12, 250, 62);
    Style::panel(p, panel, 1, 0.88f);
    // портрет-руна
    QRectF por(panel.left() + 8, panel.top() + 8, 46, 46);
    p->save();
    p->setBrush(QColor(14, 11, 20));
    p->setPen(QPen(C::edgeHi.darker(140), 1.6));
    p->drawRect(por);
    int cls = m_game->classId();
    int gl = cls == 0 ? Gl_Swords : (cls == 1 ? Gl_Flask : (cls == 2 ? Gl_Bow : Gl_Skull));
    QColor cc = cls == 0 ? C::ember : (cls == 1 ? C::manaHi : (cls == 2 ? C::good : C::arcane));
    drawGlyph(p, gl, por.adjusted(6, 6, -6, -6), cc, cc.lighter(130), 0.4f);
    p->restore();

    float tx = por.right() + 10;
    Style::textShadow(p, QRectF(tx, panel.top() + 7, panel.right() - tx - 8, 18),
                      m_game->heroName(), C::bone, fontHead(14), Qt::AlignLeft | Qt::AlignVCenter);
    Style::textShadow(p, QRectF(tx, panel.top() + 25, panel.right() - tx - 8, 14),
                      QStringLiteral("Уровень %1").arg(m_game->level()), C::amber, fontSmall(11),
                      Qt::AlignLeft | Qt::AlignVCenter);
    QRectF xr(tx, panel.top() + 41, panel.right() - tx - 10, 8);
    Style::bar(p, xr, m_game->xpProgress(), C::xp, C::edge);

    // очки навыков
    if (m_game->skillPoints() > 0) {
        float pulse = 0.6f + 0.4f * qSin(m_time * 4.f);
        QRectF pr(panel.right() - 36, panel.top() + 9, 28, 28);
        Style::glow(p, pr.center(), 28.f, C::amber, pulse);
        drawGlyph(p, Gl_LevelUp, pr, C::amber, C::gold, 0.5f);
        Style::textShadow(p, QRectF(pr.left() - 16, pr.top() + 5, 15, 18), QString::number(m_game->skillPoints()),
                          C::amber, fontNum(13, true), Qt::AlignRight);
    }
    // счётчики
    Style::textShadow(p, QRectF(panel.left() + 6, panel.bottom() + 2, 300, 14),
                      QStringLiteral("Убийств: %1 · Смертей: %2 · Зачищено: %3/%4")
                      .arg(m_game->kills()).arg(m_game->deaths())
                      .arg(m_game->clearedSet().size()).arg(Data::locationCount()),
                      C::boneDim.darker(110), fontSmall(9), Qt::AlignLeft);
}

void Hud::drawTopRight(QPainter *p)
{
    const LocationDef *L = m_view ? Data::loc(m_view->currentLocation()) : 0;
    float mw = 208.f, mh = 156.f;
    QRectF panel(width() - mw - 14, 12, mw, mh + (L ? 62 : 30));
    Style::panel(p, panel, 1, 0.88f);
    QRectF mmr(panel.left() + 10, panel.top() + 10, mw - 20, mh - 20);
    if (m_view) m_view->renderMinimap(p, mmr);

    float y = mmr.bottom() + 4;
    if (L) {
        QFont fL = fontHead(13), fS = fontSmall(9), fO = fontSmall(10);
        QFontMetrics fmL(fL), fmS(fS), fmO(fO);
        float tw = panel.width() - 20;
        Style::textShadow(p, QRectF(panel.left() + 8, y, tw, 17), fmL.elidedText(L->name, Qt::ElideRight, int(tw)),
                          C::amber, fL, Qt::AlignLeft | Qt::AlignVCenter);
        y += 17;
        Style::textShadow(p, QRectF(panel.left() + 8, y, tw, 13),
                          fmS.elidedText(QStringLiteral("%1 · %2 · порог %3").arg(Data::regionName(L->region)).arg(biomeName(L->biome)).arg(L->tier), Qt::ElideRight, int(tw)),
                          C::boneDim, fS, Qt::AlignLeft | Qt::AlignVCenter);
        y += 14;
        if (m_view) {
            Style::textShadow(p, QRectF(panel.left() + 8, y, tw, 14),
                              fmO.elidedText(m_view->objectiveText(), Qt::ElideRight, int(tw)), C::bone, fO, Qt::AlignLeft | Qt::AlignVCenter);
            y += 15;
            Style::bar(p, QRectF(panel.left() + 10, y, panel.width() - 20, 6), m_view->objectiveProgress(),
                       C::good, C::edge);
        }
    }
    // fps
    if (m_view)
        Style::textShadow(p, QRectF(panel.left() + 8, panel.bottom() - 14, panel.width() - 16, 12),
                          QStringLiteral("%1 к/с · сущ. %2").arg(int(m_view->fps())).arg(m_view->particlesCount()),
                          C::boneDim.darker(130), fontSmall(8), Qt::AlignRight);
}

void Hud::drawBossBar(QPainter *p)
{
    if (!m_view || !m_view->bossActive()) return;
    float w = qMin(620.f, float(width()) * 0.55f);
    QRectF panel((width() - w) * 0.5f, 12, w, 62);
    Style::panel(p, panel, 1, 0.95f);
    Style::textShadow(p, QRectF(panel.left() + 12, panel.top() + 5, panel.width() - 24, 20),
                      m_view->bossName(), C::bad, fontHead(16), Qt::AlignCenter);
    float t = clampf(m_view->bossHp() / qMax(1.f, m_view->bossMaxHp()), 0.f, 1.f);
    QRectF br(panel.left() + 14, panel.top() + 27, panel.width() - 28, 16);
    QLinearGradient g(br.topLeft(), br.bottomLeft());
    Style::bar(p, br, t, C::blood, C::edgeHi.darker(150), false, 0.6f + 0.4f * float(qSin(m_time * 3.f)));
    // деления фаз
    p->save();
    p->setPen(QPen(QColor(0, 0, 0, 160), 1.4));
    for (int i = 1; i < 4; ++i)
        p->drawLine(QPointF(br.left() + br.width() * i / 4.f, br.top()), QPointF(br.left() + br.width() * i / 4.f, br.bottom()));
    p->restore();
    Style::textShadow(p, QRectF(panel.left() + 12, panel.top() + 44, panel.width() - 24, 14),
                      QStringLiteral("%1 / %2   ·   фаза %3").arg(int(m_view->bossHp())).arg(int(m_view->bossMaxHp())).arg(m_view->bossPhase()),
                      C::boneDim, fontSmall(10), Qt::AlignCenter);
    Q_UNUSED(g);
}

void Hud::drawBuffs(QPainter *p)
{
    if (!m_view) return;
    QVector<QPair<int, float> > buffs = m_view->activeBuffs();
    if (buffs.isEmpty()) return;
    float s = 30.f;
    float x = 22.f;
    float y = height() - 160.f;
    for (int i = 0; i < buffs.size() && i < 10; ++i) {
        QRectF r(x, y - float(i) * (s + 5.f), s, s);
        Style::panel(p, r, 1, 0.9f);
        QColor c = C::amber;
        switch (buffs.at(i).first) {
        case Gl_Blood: c = C::bloodHi; break;
        case Gl_Haste: c = C::good; break;
        case Gl_ShieldDot: c = C::manaHi; break;
        case Gl_Skull: c = C::arcane; break;
        default: c = C::amber; break;
        }
        drawGlyph(p, buffs.at(i).first, r.adjusted(4, 4, -4, -4), c, c.lighter(130), 0.4f);
        float tt = buffs.at(i).second;
        QString label = (tt > 900.f) ? QStringLiteral("∞")
                        : (buffs.at(i).first == Gl_Skull ? QString::number(int(tt))
                                                         : QString::number(double(tt), 'f', 1));
        Style::textShadow(p, QRectF(r.left() - 6, r.bottom() - 3, r.width() + 12, 12), label, C::bone, fontSmall(9),
                          Qt::AlignCenter);
    }
}

void Hud::drawLog(QPainter *p)
{
    if (!m_showLog || m_log.isEmpty()) return;
    float y = height() - 190.f;
    QFont f = fontSmall(11);
    for (int i = m_log.size() - 1; i >= 0; --i) {
        LogLine &l = m_log[i];
        l.life -= 0.033f;
        if (l.life <= 0.f) { m_log.removeAt(i); continue; }
        float a = qMin(1.f, l.life / 1.6f);
        QColor c = l.color;
        c.setAlphaF(a);
        QRectF r(18, y, qMin(float(width()) * 0.42f, 520.f), 16);
        p->save();
        p->setOpacity(a * 0.85f);
        QFontMetrics fm(f);
        QString txt = fm.elidedText(l.text, Qt::ElideRight, int(r.width()));
        p->fillRect(QRectF(r.left() - 4, r.top(), fm.width(txt) + 10, 15), QColor(6, 5, 10, 120));
        p->setFont(f);
        p->setPen(QColor(0, 0, 0, 200));
        p->drawText(r.translated(1, 1), Qt::AlignLeft | Qt::AlignVCenter, txt);
        p->setPen(l.color);
        p->drawText(r, Qt::AlignLeft | Qt::AlignVCenter, txt);
        p->restore();
        y -= 17.f;
        if (y < height() * 0.42f) break;
    }
}

void Hud::drawPrompt(QPainter *p)
{
    if (!m_view) return;
    QString pr = m_view->prompt();
    if (pr.isEmpty()) return;
    QFont f = fontHead(13);
    QFontMetrics fm(f);
    float w = fm.width(pr) + 34;
    QRectF r((width() - w) * 0.5f, height() - 168.f, w, 30);
    Style::panel(p, r, 1, 0.92f);
    float pulse = 0.7f + 0.3f * qSin(m_time * 5.f);
    Style::glow(p, r.center(), r.width() * 0.6f, C::amber, 0.10f * pulse);
    Style::textShadow(p, r, pr, C::amber, f, Qt::AlignCenter);
}

void Hud::drawNotifications(QPainter *p)
{
    float y = height() * 0.34f;
    for (int i = m_notes.size() - 1; i >= 0; --i) {
        Notify &n = m_notes[i];
        n.life -= 0.033f;
        if (n.life <= 0.f) { m_notes.removeAt(i); continue; }
        float a = qMin(1.f, n.life / 0.8f);
        float inK = easeOutCubic(clampf((2.6f - n.life) / 0.35f, 0.f, 1.f));
        float slide = (1.f - inK) * 60.f;
        QFont f = fontHead(14);
        QFontMetrics fm(f);
        float w = fm.width(n.text) + 46;
        QRectF r((width() - w) * 0.5f + slide, y, w, 30);
        p->save();
        p->setOpacity(a);
        Style::panel(p, r, 1, 0.95f);
        drawGlyph(p, n.glyph, QRectF(r.left() + 6, r.top() + 5, 20, 20), n.color, n.color.lighter(130), 0.5f);
        Style::textShadow(p, QRectF(r.left() + 30, r.top(), r.width() - 36, r.height()), n.text, n.color, f,
                          Qt::AlignLeft | Qt::AlignVCenter);
        p->restore();
        y -= 36.f;
    }
}

void Hud::drawAchBanner(QPainter *p)
{
    if (m_ach.isEmpty()) return;
    float y = 96.f;
    for (int i = m_ach.size() - 1; i >= 0; --i) {
        AchNote &n = m_ach[i];
        n.life -= 0.033f;
        if (n.life <= 0.f) { m_ach.removeAt(i); continue; }
        float a = qMin(1.f, n.life / 0.7f) * qMin(1.f, (5.f - n.life) / 0.35f + 0.2f);
        QFont f = fontHead(16);
        QFontMetrics fm(f);
        QString head = QStringLiteral("ДОСТИЖЕНИЕ");
        float w = qMax(fm.width(n.text), fm.width(head) + 60) + 92;
        QRectF r((width() - w) * 0.5f, y, w, 62);
        p->save();
        p->setOpacity(a);
        Style::panel(p, r, 1, 0.97f);
        Style::glow(p, r.center(), r.width() * 0.6f, C::amber, 0.22f);
        p->setPen(QPen(C::amber, 1.8));
        p->setBrush(Qt::NoBrush);
        p->drawRect(r.adjusted(3, 3, -3, -3));
        drawGlyph(p, n.glyph, QRectF(r.left() + 12, r.top() + 12, 38, 38), C::amber, C::gold, 0.6f);
        Style::textShadow(p, QRectF(r.left() + 58, r.top() + 8, r.width() - 70, 18), head, C::amber, fontSmall(10),
                          Qt::AlignLeft | Qt::AlignVCenter);
        Style::textShadow(p, QRectF(r.left() + 58, r.top() + 26, r.width() - 70, 26), n.text, C::bone, f,
                          Qt::AlignLeft | Qt::AlignVCenter);
        p->restore();
        y += 70.f;
    }
}

void Hud::drawStreak(QPainter *p)
{
    if (!m_view || m_view->streak() < 3) return;
    float t = m_view->streakTime();
    int n = m_view->streak();
    float pulse = 0.6f + 0.4f * qSin(m_time * 8.f);
    QString txt = QStringLiteral("СЕРИЯ x%1").arg(n);
    QFont f = fontHead(n >= 20 ? 22 : (n >= 10 ? 19 : 16));
    QFontMetrics fm(f);
    float w = fm.width(txt) + 46;
    QRectF r(width() - w - 24, height() * 0.5f - 30, w, 44);
    p->save();
    p->setOpacity(qMin(1.f, t / 0.6f));
    Style::glow(p, r.center(), r.width() * 0.8f, C::amber, 0.25f * pulse);
    Style::panel(p, r, 1, 0.9f);
    p->setPen(QPen(C::amber, 1.6));
    p->setBrush(Qt::NoBrush);
    p->drawRect(r.adjusted(2, 2, -2, -2));
    drawGlyph(p, Gl_Swords, QRectF(r.left() + 8, r.top() + 10, 24, 24), C::amber, C::gold, 0.5f);
    Style::textShadow(p, QRectF(r.left() + 36, r.top(), r.width() - 42, r.height() - 8), txt, C::amber, f,
                      Qt::AlignLeft | Qt::AlignVCenter);
    Style::bar(p, QRectF(r.left() + 6, r.bottom() - 7, r.width() - 12, 4), t / 3.5f, C::ember, C::edge);
    p->restore();
}

} // namespace aa
