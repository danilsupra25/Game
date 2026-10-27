#include "UI/Tooltip.h"
#include "Core/Style.h"
#include "Core/Data.h"
#include "Core/Game.h"

#include <QPainter>
#include <QFontMetrics>
#include <QApplication>
#include <QDesktopWidget>
#include <QtMath>

namespace aa {

Tooltip::Tooltip(QWidget *parent) : QWidget(parent), m_border(C::edge), m_glyph(0)
{
    // ToolTip-флаг на Windows уходит под фулскрин-окно; используем обычное
    // неактивируемое окно поверх всех
    // дочерний виджет главного окна: гарантированно поверх панелей и в фулскрине
    setWindowFlags(Qt::Widget);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setAutoFillBackground(false);
    setFocusPolicy(Qt::NoFocus);
    setMouseTracking(false);
}

Tooltip *Tooltip::instance(QWidget *parentWidget)
{
    static Tooltip *t = 0;
    if (!t) {
        t = new Tooltip(parentWidget);
    }
    return t;
}

void Tooltip::place(const QPoint &gp)
{
    QWidget *par = parentWidget();
    if (!par) { hide(); return; }
    adjustSize();
    QPoint lp = par->mapFromGlobal(gp);
    int x = lp.x() + 18;
    int y = lp.y() + 14;
    if (x + width() > par->width() - 6) x = lp.x() - width() - 14;
    if (y + height() > par->height() - 6) y = qMax(4, lp.y() - height() - 10);
    if (x < 4) x = 4;
    if (y < 4) y = 4;
    move(x, y);
    raise();
    show();
    update();
}

void Tooltip::hideNow() { hide(); }

void Tooltip::showText(const QString &title, const QString &body, const QPoint &gp, const QColor &tc)
{
    m_lines.clear();
    m_glyph = 0;
    m_glow = 0.f;
    m_border = tc.isValid() ? tc : C::edge;
    if (!title.isEmpty()) {
        Line l; l.text = title; l.color = tc.isValid() ? tc : C::amber; l.font = fontHead(13);
        l.align = Qt::AlignLeft; l.spaceBefore = 0;
        m_lines.append(l);
    }
    QStringList parts = body.split(QLatin1Char('\n'));
    for (int i = 0; i < parts.size(); ++i) {
        Line l; l.text = parts.at(i); l.color = C::bone; l.font = fontBody(11);
        l.align = Qt::AlignLeft; l.spaceBefore = (i == 0 && !title.isEmpty()) ? 4 : 0;
        m_lines.append(l);
    }
    place(gp);
}

void Tooltip::showItem(const Item &it, const Game *game, const QPoint &gp, bool inShop, int price)
{
    m_lines.clear();
    m_glyph = 0;
    if (!it.valid) { hideNow(); return; }
    const ItemBaseDef *b = Data::base(it.baseId);
    if (!b) { hideNow(); return; }
    m_glyph = b->glyph;
    m_glyphColor = rarityColor(it.rarity);
    m_border = rarityColor(it.rarity);
    m_glow = it.rarity >= Rar_Rare ? 0.5f : 0.15f;

    QFont fBig = fontHead(14);
    QFont fNorm = fontBody(11);
    QFont fSmall = fontSmall(10);
    QFont fFlav = fontBody(11);
    fFlav.setItalic(true);

    auto add = [&](const QString &t, const QColor &c, const QFont &f, float sb = 0.f, int al = Qt::AlignLeft) {
        Line l; l.text = t; l.color = c; l.font = f; l.align = al; l.spaceBefore = sb; m_lines.append(l);
    };

    add(Data::itemNameFull(it), rarityColor(it.rarity), fBig);
    QString typeLine = b->bag == Bag_Equip ? slotName(b->slot) : (b->bag == Bag_Potion ? QStringLiteral("Зелье")
                        : (b->bag == Bag_Gem ? QStringLiteral("Самоцвет") : QStringLiteral("Предмет")));
    if (b->bag == Bag_Equip) typeLine += QStringLiteral("  ·  порог %1").arg(romanNum(b->tier));
    add(typeLine, C::boneDim, fSmall, 1.f);

    if (it.rarity == Rar_Unique) {
        const UniqueDef *u = Data::unq(it.uniqueId);
        if (u) add(QStringLiteral("«%1»").arg(u->flavor), C::amber.darker(115), fFlav, 5.f);
    }

    if (b->bag == Bag_Equip) {
        if (b->dmgMax > 0.f) {
            float dmin = game ? game->itemDmgMin(it) : b->dmgMin;
            float dmax = game ? game->itemDmgMax(it) : b->dmgMax;
            add(QStringLiteral("Урон: %1-%2  (%3)").arg(int(dmin)).arg(int(dmax)).arg(dmgTypeName(b->dmgType)),
                C::bone, fNorm, 5.f);
            add(QStringLiteral("Скорость: %1 атак/сек").arg(QString::number(double(b->atkSpeed), 'f', 2)), C::bone, fNorm);
            float dps = (dmin + dmax) * 0.5f * b->atkSpeed;
            add(QStringLiteral("Урон в секунду: %1").arg(int(dps)), C::boneDim, fSmall);
        }
        float arm = b->base.armor;
        if (arm > 0.f) add(QStringLiteral("Броня: %1").arg(int(arm)), C::bone, fNorm, b->dmgMax > 0.f ? 1.f : 5.f);
        if (b->reqLevel > 1) add(QStringLiteral("Требуется уровень: %1").arg(b->reqLevel), C::boneDim, fSmall, 3.f);
    } else if (b->bag == Bag_Potion) {
        if (b->base.hpFlat > 0) add(QStringLiteral("Восстанавливает %1 здоровья").arg(int(b->base.hpFlat)), C::hpHi, fNorm, 5.f);
        if (b->base.manaFlat > 0) add(QStringLiteral("Восстанавливает %1 маны").arg(int(b->base.manaFlat)), C::manaHi, fNorm, 5.f);
        if (b->base.attackSpeed > 0) add(QStringLiteral("+%1 скорости атаки на 8 сек").arg(int(b->base.attackSpeed * 100)), C::ember, fNorm, 5.f);
        add(QStringLiteral("Клавиша: Пробел"), C::boneDim, fSmall, 3.f);
    } else if (b->bag == Bag_Gem) {
        add(QStringLiteral("Старый самоцвет - гнёзда убраны из игры"), C::boneDim, fSmall, 5.f);
    } else if (b->id_str == QLatin1String("m_tp")) {
        add(QStringLiteral("Мгновенно возвращает в последнее поселение"), C::arcane, fNorm, 5.f);
        add(QStringLiteral("Клавиша: Q"), C::boneDim, fSmall, 3.f);
    } else if (b->id_str == QLatin1String("m_soul")) {
        add(QStringLiteral("Полностью сбрасывает паутину навыков"), C::arcane, fNorm, 5.f);
        add(QStringLiteral("Клавиша: G"), C::boneDim, fSmall, 3.f);
    }

    // модификаторы базы
    QString baseMods = Data::describeMods(b->base);
    if (!baseMods.isEmpty() && b->bag == Bag_Equip) {
        QStringList bl = baseMods.split(QLatin1Char('\n'));
        for (int i = 0; i < bl.size(); ++i) add(bl.at(i), C::bone, fNorm, i == 0 ? 6.f : 0.f);
    }
    // аффиксы
    if (it.rarity != Rar_Unique) {
        for (int i = 0; i < it.affixes.size(); ++i) {
            const AffixDef *a = Data::affix(it.affixes.at(i));
            if (!a) continue;
            float roll = i < it.rolls.size() ? it.rolls.at(i) : 1.f;
            QString txt = Data::describeMods(a->mods, roll);
            QStringList tl = txt.split(QLatin1Char('\n'));
            for (int k = 0; k < tl.size(); ++k)
                add(tl.at(k), it.rarity == Rar_Magic ? QColor(140, 175, 255) : C::amber.darker(105), fNorm,
                    (i == 0 && k == 0) ? 6.f : 0.f);
        }
    } else {
        for (int i = 0; i < it.affixes.size(); ++i) {
            const AffixDef *a = Data::affix(it.affixes.at(i));
            if (!a) continue;
            float roll = i < it.rolls.size() ? it.rolls.at(i) : 1.f;
            QString txt = Data::describeMods(a->mods, roll);
            QStringList tl = txt.split(QLatin1Char('\n'));
            for (int k = 0; k < tl.size(); ++k)
                add(tl.at(k), QColor(240, 150, 100), fNorm, (i == 0 && k == 0) ? 6.f : 0.f);
        }
    }
    // гнёзда/самоцветы убраны из игры
    // сравнение с надетым
    if (game && b->bag == Bag_Equip && !inShop) {
        Slot sl = b->slot;
        if (sl == Slot_Ring1 && !game->equipped(Slot_Ring1).valid) sl = Slot_Ring1;
        Item cur = game->equipped(sl);
        if (sl == Slot_Ring1 && cur.valid && game->equipped(Slot_Ring2).valid) cur = game->equipped(Slot_Ring1);
        if (cur.valid) {
            int sc = game->itemScore(it) - game->itemScore(cur);
            QString cmp = sc > 0 ? QStringLiteral("Лучше текущего на %1").arg(sc)
                                 : (sc < 0 ? QStringLiteral("Хуже текущего на %1").arg(-sc)
                                           : QStringLiteral("Равно текущему"));
            add(cmp, sc > 0 ? C::good : (sc < 0 ? C::bad : C::boneDim), fSmall, 7.f);
        } else {
            add(QStringLiteral("Слот пуст"), C::boneDim, fSmall, 7.f);
        }
    }
    int pr = price >= 0 ? price : (game ? game->priceOf(it, inShop) : 0);
    if (pr > 0)
        add(inShop ? QStringLiteral("Цена: %1 золота").arg(pr) : QStringLiteral("Продажа: %1 золота").arg(pr),
            C::gold, fSmall, 6.f);
    if (b->bag == Bag_Equip)
        add(QStringLiteral("ЛКМ - надеть · ПКМ - надеть"), C::boneDim.darker(115), fSmall, 5.f);
    else if (b->bag == Bag_Gem)
        add(QStringLiteral("Перетащи на предмет с гнездом"), C::boneDim.darker(115), fSmall, 5.f);

    place(gp);
}

void Tooltip::showSkill(int skillId, int rank, const QPoint &gp)
{
    const ActiveSkill *s = Data::skill(skillId);
    if (!s) { hideNow(); return; }
    m_glyph = s->glyph;
    m_glyphColor = s->color;
    m_border = s->color;
    m_glow = 0.35f;
    m_lines.clear();
    QFont fBig = fontHead(14), fNorm = fontBody(11), fSmall = fontSmall(10);
    auto add = [&](const QString &t, const QColor &c, const QFont &f, float sb = 0.f) {
        Line l; l.text = t; l.color = c; l.font = f; l.align = Qt::AlignLeft; l.spaceBefore = sb; m_lines.append(l);
    };
    add(s->name + (rank > 0 ? QStringLiteral("  ·  ранг %1").arg(romanNum(rank)) : QString()), s->color, fBig);
    add(QStringLiteral("%1 урон").arg(dmgTypeName(s->dmgType)), C::boneDim, fSmall, 1.f);
    QStringList dl = s->desc.split(QLatin1Char('\n'));
    for (int i = 0; i < dl.size(); ++i) add(dl.at(i), C::bone, fNorm, i == 0 ? 5.f : 0.f);
    QStringList sl = Data::skillDesc(*s, qMax(1, rank)).split(QLatin1Char('\n'));
    for (int i = 1; i < sl.size(); ++i) add(sl.at(i), C::boneDim, fSmall, i == 1 ? 5.f : 0.f);
    if (rank < s->maxRank)
        add(QStringLiteral("Следующий ранг: +22% урона"), C::good, fSmall, 5.f);
    place(gp);
}

void Tooltip::showNode(int nodeId, int rank, const Game *game, const QPoint &gp)
{
    const SkillNode *n = Data::node(nodeId);
    if (!n) { hideNow(); return; }
    m_glyph = (n->kind == Nk_Active && n->activeSkill >= 0) ? Data::skill(n->activeSkill)->glyph
              : (n->kind == Nk_Keystone ? Gl_Crown2 : (n->kind == Nk_Origin ? Gl_Star : Gl_Web));
    m_glyphColor = n->color;
    m_border = n->color;
    m_glow = n->kind >= Nk_Keystone ? 0.6f : 0.25f;
    m_lines.clear();
    QFont fBig = fontHead(14), fNorm = fontBody(11), fSmall = fontSmall(10);
    auto add = [&](const QString &t, const QColor &c, const QFont &f, float sb = 0.f) {
        Line l; l.text = t; l.color = c; l.font = f; l.align = Qt::AlignLeft; l.spaceBefore = sb; m_lines.append(l);
    };
    QString kindName = n->kind == Nk_Keystone ? QStringLiteral("Ключевой камень")
                     : (n->kind == Nk_Active ? QStringLiteral("Активное умение")
                     : (n->kind == Nk_Notable ? QStringLiteral("Заметный узел")
                     : (n->kind == Nk_Origin ? QStringLiteral("Исток паутины") : QStringLiteral("Малый узел"))));
    add(n->name, n->color, fBig);
    add(kindName + QStringLiteral("  ·  круг %1").arg(n->ring), C::boneDim, fSmall, 1.f);
    QStringList dl = n->desc.split(QLatin1Char('\n'));
    for (int i = 0; i < dl.size(); ++i) add(dl.at(i), C::bone, fNorm, i == 0 ? 5.f : 0.f);

    QString mods = Data::describeMods(n->mods);
    if (!mods.isEmpty()) {
        QStringList ml = mods.split(QLatin1Char('\n'));
        for (int i = 0; i < ml.size(); ++i) add(ml.at(i), C::good, fNorm, i == 0 ? 5.f : 0.f);
    }
    if (n->maxRank > 1)
        add(QStringLiteral("Ранг: %1 / %2").arg(rank).arg(n->maxRank), rank > 0 ? C::good : C::boneDim, fSmall, 5.f);
    add(QStringLiteral("Стоимость: %1 очк.").arg(n->cost), C::boneDim, fSmall, 3.f);
    if (game) {
        QString why;
        if (rank >= n->maxRank) add(QStringLiteral("Изучено полностью"), C::good, fSmall, 5.f);
        else if (!game->canTakeNode(nodeId, &why)) add(why, C::bad, fSmall, 5.f);
        else add(QStringLiteral("ЛКМ - изучить · ПКМ - забыть"), C::amber, fSmall, 5.f);
        if (game->skillPoints() > 0)
            add(QStringLiteral("Свободных очков: %1").arg(game->skillPoints()), C::amber, fSmall, 2.f);
    }
    place(gp);
}

void Tooltip::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    // очищаем до прозрачного (дочерний виджет без фона)
    p.setCompositionMode(QPainter::CompositionMode_Clear);
    p.fillRect(rect(), Qt::transparent);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    if (m_lines.isEmpty()) return;
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QFontMetrics fmBig(fontHead(14));
    QFontMetrics fmNorm(fontBody(11));
    QFontMetrics fmSmall(fontSmall(10));

    // считаем размеры
    int padL = 14, padR = 14, padT = 12, padB = 12;
    int glyphW = m_glyph ? 40 : 0;
    float y = padT;
    QVector<QRectF> rects;
    int maxW = 0;
    for (int i = 0; i < m_lines.size(); ++i) {
        const Line &l = m_lines.at(i);
        QFontMetrics fm = (l.font.pointSize() >= 13) ? fmBig : (l.font.pointSize() == 11 ? fmNorm : fmSmall);
        y += l.spaceBefore;
        int w = fm.width(l.text);
        maxW = qMax(maxW, w);
        QRectF r(padL + glyphW, y, w + 4, fm.height());
        rects.append(r);
        y += fm.height() + 1;
    }
    int totalW = qMax(180, maxW + padL + padR + glyphW);
    int totalH = int(y) + padB;
    if (size() != QSize(totalW, totalH)) resize(totalW, totalH);

    QRectF box(2, 2, totalW - 4, totalH - 4);
    Style::panel(&p, box, 1, 0.97f);
    if (m_glow > 0.01f) {
        p.save();
        p.setPen(QPen(QColor(m_border.red(), m_border.green(), m_border.blue(), 190), 1.6));
        p.setBrush(Qt::NoBrush);
        p.drawRect(box.adjusted(3, 3, -3, -3));
        p.restore();
    }
    if (m_glyph) {
        QRectF gr(padL - 4, padT, 34, 34);
        Style::glow(&p, gr.center(), 30.f, m_glyphColor, m_glow);
        drawGlyph(&p, m_glyph, gr, m_glyphColor, m_glyphColor.lighter(130), m_glow);
        p.setPen(QPen(C::edge, 1.2));
        p.setBrush(QColor(10, 8, 16, 160));
        p.drawRect(gr.adjusted(-3, -3, 3, 3));
    }
    for (int i = 0; i < m_lines.size() && i < rects.size(); ++i) {
        const Line &l = m_lines.at(i);
        p.setFont(l.font);
        p.setPen(QColor(0, 0, 0, 190));
        p.drawText(rects.at(i).translated(1, 1), Qt::AlignLeft | Qt::AlignVCenter, l.text);
        p.setPen(l.color);
        p.drawText(rects.at(i), Qt::AlignLeft | Qt::AlignVCenter, l.text);
    }
}

} // namespace aa
