#include "UI/Panels.h"
#include "UI/Tooltip.h"
#include "Core/Style.h"
#include "Core/Data.h"
#include "Core/Game.h"
#include "Core/Audio.h"
#include "Core/Rng.h"
#include "Core/Binds.h"
#include "Core/Ach.h"
#include "Game/GameView.h"
#include "Game/Sprites.h"
#include "Game/SpriteBank.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QTime>
#include <QFontMetrics>
#include <QSettings>
#include <QApplication>
#include <QCursor>
#include <QDebug>
#include <QFileInfo>
#include <QtMath>
#include <algorithm>

namespace aa {

// перечисления действий кнопок
enum PanelAction {
    Act_Close = 1,
    // инвентарь
    Act_InvTabStats = 100, Act_InvTabHelp, Act_InvUnequipAll, Act_InvSortBag,
    Act_InvFilter0 = 110, Act_InvFilter1, Act_InvFilter2, Act_InvFilter3,
    // паутина
    Act_WebRespec = 200, Act_WebFilterAll, Act_WebFilterWar, Act_WebFilterMage,
    Act_WebFilterRogue, Act_WebFilterPriest, Act_WebCenter,
    // карта
    Act_MapTravel = 300, Act_MapClose,
    // магазин
    Act_ShopTabBuy = 400, Act_ShopTabSell, Act_ShopBuy, Act_ShopSell, Act_ShopRestock,
    // пауза
    Act_PauseResume = 500, Act_PauseSave, Act_PauseLoad, Act_PauseSettings, Act_PauseHelp,
    Act_PauseQuitMenu, Act_PauseQuitGame, Act_PauseSound, Act_PauseShake, Act_PauseFullscreen,
    Act_PauseKeys = 510, Act_PauseKeysReset = 511, Act_PauseKeysBack = 512,
    Act_PauseTabSound = 513, Act_PauseTabVideo = 514, Act_PauseTabKeys = 515, Act_PauseBack = 516,
    Act_VideoPixel = 517, Act_VideoScan = 518, Act_VideoGl = 519, Act_VideoSprites = 520,
    Act_VideoLight = 521, Act_VideoShadow = 522,
    Act_VideoAutoQ = 523, Act_VideoMinimap = 524,
    // смерть
    Act_DeathRespawn = 600, Act_DeathQuit,
    // лог/справка
    Act_LogClear = 700, Act_HelpNext = 710, Act_HelpPrev,
    // титул
    Act_TitleNew = 800, Act_TitleContinue, Act_TitleSettings, Act_TitleHelp, Act_TitleQuit,
    Act_TitleBack, Act_TitleStart, Act_TitleSound
};

static QStringList wrapText(const QString &text, const QFont &f, float maxW)
{
    QFontMetrics fm(f);
    QStringList out;
    const QStringList words = text.split(QLatin1Char(' '), QString::SkipEmptyParts);
    QString cur;
    for (int i = 0; i < words.size(); ++i) {
        QString t = cur.isEmpty() ? words.at(i) : cur + QLatin1Char(' ') + words.at(i);
        if (fm.width(t) > int(maxW) && !cur.isEmpty()) { out << cur; cur = words.at(i); }
        else cur = t;
    }
    if (!cur.isEmpty()) out << cur;
    if (out.isEmpty()) out << QString();
    return out;
}

static QString actTitle(int act)
{
    switch (act) {
    case Binds::A_Up:       return QStringLiteral("Движение: вверх");
    case Binds::A_Down:     return QStringLiteral("Движение: вниз");
    case Binds::A_Left:     return QStringLiteral("Движение: влево");
    case Binds::A_Right:    return QStringLiteral("Движение: вправо");
    case Binds::A_Potion:   return QStringLiteral("Выпить зелье");
    case Binds::A_Interact: return QStringLiteral("Взаимодействие");
    case Binds::A_Return:   return QStringLiteral("Свиток возвращения");
    case Binds::A_Shard:    return QStringLiteral("Осколок души");
    case Binds::A_Skill1:   return QStringLiteral("Умение 1");
    case Binds::A_Skill2:   return QStringLiteral("Умение 2");
    case Binds::A_Skill3:   return QStringLiteral("Умение 3");
    case Binds::A_Skill4:   return QStringLiteral("Умение 4");
    case Binds::A_Skill5:   return QStringLiteral("Умение 5");
    case Binds::A_Skill6:   return QStringLiteral("Умение 6");
    case Binds::A_Inv:      return QStringLiteral("Сумка и снаряжение");
    case Binds::A_Web:      return QStringLiteral("Паутина навыков");
    case Binds::A_Map:      return QStringLiteral("Карта мира");
    case Binds::A_Log:      return QStringLiteral("Журнал");
    case Binds::A_Help:     return QStringLiteral("Справка");
    case Binds::A_Pause:    return QStringLiteral("Привал / закрыть панель");
    case Binds::A_Save:     return QStringLiteral("Быстрое сохранение");
    case Binds::A_Load:     return QStringLiteral("Быстрая загрузка");
    case Binds::A_Ach:      return QStringLiteral("Достижения");
    default:                return QStringLiteral("?");
    }
}

void drawVolumeSlider(QPainter *p, const QRectF &r, float vol, bool active)
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    Style::textShadow(p, QRectF(r.left(), r.top() - 18, r.width(), 16),
                      QStringLiteral("Громкость: %1%").arg(int(vol * 100 + 0.5f)),
                      active ? C::amber : C::boneDim, fontSmall(11), Qt::AlignLeft | Qt::AlignVCenter);
    QRectF track(r.left(), r.top() + r.height() * 0.5f - 4, r.width(), 8);
    p->setBrush(QColor(8, 6, 12, 210));
    p->setPen(QPen(active ? C::edgeHi : C::edge.darker(130), active ? 1.6 : 1.0));
    p->drawRoundedRect(track, 4, 4);
    QRectF fill(track.left() + 1, track.top() + 1, (track.width() - 2) * clampf(vol, 0.f, 1.f), track.height() - 2);
    QLinearGradient g(fill.topLeft(), fill.bottomLeft());
    g.setColorAt(0, C::amber.lighter(140));
    g.setColorAt(1, C::edgeHi.darker(140));
    p->setPen(Qt::NoPen);
    p->fillRect(fill, g);
    p->setPen(QPen(QColor(0, 0, 0, 120), 1.0));
    for (int i = 1; i < 4; ++i) {
        float x = track.left() + track.width() * i / 4.f;
        p->drawLine(QPointF(x, track.top() + 1), QPointF(x, track.bottom() - 1));
    }
    float kx = track.left() + (track.width() - 2) * clampf(vol, 0.f, 1.f) + 1;
    QPointF kc(kx, track.center().y());
    Style::glow(p, kc, 16.f, C::amber, active ? 0.6f : 0.3f);
    p->setBrush(active ? C::amber : C::edgeHi);
    p->setPen(QPen(QColor(20, 14, 8), 1.4));
    p->drawEllipse(kc, 8.f, 8.f);
    p->setBrush(QColor(255, 250, 235));
    p->setPen(Qt::NoPen);
    p->drawEllipse(kc, 2.6f, 2.6f);
    p->restore();
}

float volumeFromX(const QRectF &r, float x)
{
    return clampf((x - r.left()) / qMax(1.f, float(r.width())), 0.f, 1.f);
}

// ============================================================
//  OverlayPanel
// ============================================================
OverlayPanel::OverlayPanel(QWidget *parent)
    : QWidget(parent), m_game(0), m_view(0), m_time(0.f), m_open(0.f), m_draggingPanel(false)
{
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    hide();
    m_anim = new QTimer(this);
    m_anim->setInterval(16);
    connect(m_anim, SIGNAL(timeout()), this, SLOT(update()));
    m_pressedBtn = -1;
}

QRectF OverlayPanel::panelRect() const
{
    float mw = qMin(54.f, float(width()) * 0.045f);
    float mh = qMin(42.f, float(height()) * 0.055f);
    return QRectF(mw, mh, float(width()) - mw * 2.f, float(height()) - mh * 2.f);
}

void OverlayPanel::openPanel()
{
    m_open = 0.f;
    m_mouse = QPointF(mapFromGlobal(QCursor::pos()));   // подсветка сразу под курсором
    refresh();
    show();
    raise();
    setFocus(Qt::OtherFocusReason);
    m_anim->start();
    update();
}

int OverlayPanel::addButton(const QRectF &r, const QString &text, int glyph, int id, bool enabled)
{
    Btn b;
    b.r = r; b.text = text; b.glyph = glyph; b.id = id; b.enabled = enabled;
    b.hover = false; b.down = false;
    m_buttons.append(b);
    return m_buttons.size() - 1;
}

QRectF OverlayPanel::debugButtonRect(int id) const
{
    for (int i = 0; i < m_buttons.size(); ++i) if (m_buttons[i].id == id) return m_buttons[i].r;
    return QRectF();
}

int OverlayPanel::buttonAt(const QPointF &p) const
{
    for (int i = 0; i < m_buttons.size(); ++i) if (m_buttons[i].r.contains(p)) return i;
    return -1;
}

// ключ состояния подсветки: id + позиция (кнопки пересоздаются каждый кадр)
static quint64 btnStateKey(const QRectF &r, int id)
{
    return (quint64(quint32(id)) << 32)
         | (quint64(quint32(int(r.left() * 2)) & 0xFFFFu) << 16)
         |  quint64(quint32(int(r.top() * 2)) & 0xFFFFu);
}

void OverlayPanel::paintButtons(QPainter *p)
{
    for (int i = 0; i < m_buttons.size(); ++i) {
        Btn &b = m_buttons[i];
        // hover/подсветка считаются заново каждый кадр и хранятся между перерисовками,
        // чтобы кнопки подсвечивались во всех панелях, а не только в главном меню
        const quint64 k = btnStateKey(b.r, b.id);
        b.hover = b.enabled && b.r.contains(m_mouse);
        float ht = m_btnHt.value(k, 0.f);
        ht += ((b.hover ? 1.f : 0.f) - ht) * 0.28f;
        if (ht < 0.004f) ht = 0.f;
        if (m_btnHt.size() > 96) m_btnHt.clear();
        m_btnHt.insert(k, ht);
        b.ht = ht;
        if (i == m_pressedBtn) b.down = true;
        if (b.ht > 0.02f) {
            Style::glow(p, b.r.center(), b.r.width() * 0.55f, C::amber, 0.10f * b.ht);
        }
        Style::button(p, b.r, b.text, b.hover, b.down, b.enabled, b.glyph, b.ht, b.active);
        if (b.ht > 0.02f) {
            p->save();
            QColor hc = C::amber;
            hc.setAlphaF(0.14f * b.ht);
            p->fillRect(b.r.adjusted(2, 2, -2, -2), hc);
            p->restore();
        }
        if (b.accent.isValid()) {
            p->save();
            p->setPen(QPen(b.accent, 2.0));
            p->setBrush(Qt::NoBrush);
            p->drawRect(b.r.adjusted(1, 1, -1, -1));
            p->restore();
        }
    }
}

void OverlayPanel::drawTitle(QPainter *p, const QRectF &r, const QString &title, const QString &subtitle, int glyph)
{
    QRectF tr(r.left() + 14, r.top() + 8, r.width() - 28, 48);
    if (glyph) {
        QRectF gr(tr.left() + 4, tr.top() + 4, 40, 40);
        Style::glow(p, gr.center(), 34.f, C::amber, 0.45f);
        drawGlyph(p, glyph, gr, C::amber, C::gold, 0.4f);
        tr.setLeft(gr.right() + 12);
    }
    Style::textShadow(p, QRectF(tr.left(), tr.top() + 2, tr.width(), 26), title, C::amber, fontTitle(22, true),
                      Qt::AlignLeft | Qt::AlignVCenter);
    if (!subtitle.isEmpty())
        Style::textShadow(p, QRectF(tr.left(), tr.top() + 27, tr.width(), 18), subtitle, C::boneDim, fontSmall(11),
                          Qt::AlignLeft | Qt::AlignVCenter);
    Style::divider(p, r.top() + 60, r.left() + 16, r.right() - 16, C::edgeHi.darker(140));
}

void OverlayPanel::drawScrollArea(QPainter *p, const QRectF &r)
{
    p->save();
    p->setBrush(QColor(10, 8, 16, 150));
    p->setPen(QPen(C::edge.darker(140), 1.2));
    p->drawRect(r);
    p->restore();
}

void OverlayPanel::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    m_time += 0.016f;
    if (m_open < 1.f) m_open = qMin(1.f, m_open + 0.016f / 0.30f);
    float eo = 1.f - (1.f - m_open) * (1.f - m_open) * (1.f - m_open);   // easeOutCubic

    // затемнение
    p.fillRect(rect(), QColor(4, 3, 8, int(190 * eo)));

    QRectF r = panelRect();
    float k = eo;
    QRectF ar = r.adjusted((1.f - k) * r.width() * 0.04f, (1.f - k) * r.height() * 0.06f,
                           -(1.f - k) * r.width() * 0.04f, -(1.f - k) * r.height() * 0.06f);
    p.save();
    p.setOpacity(k);
    Style::panel(&p, ar, 0, 1.0f);
    Style::corners(&p, ar, C::edgeHi.darker(120), 9.f);
    paintPanel(&p, ar);
    paintButtons(&p);
    p.restore();
}

void OverlayPanel::mousePressEvent(QMouseEvent *e)
{
    m_mouse = e->localPos();
    int bi = buttonAt(m_mouse);
    if (bi >= 0 && m_buttons[bi].enabled) {
        m_buttons[bi].down = true;
        m_pressedBtn = bi;
        Audio::inst().play(Sfx_Ui);
        update();
        e->accept();
        return;
    }
    m_pressedBtn = -1;
    onClick(m_mouse, e->button());
    update();
    e->accept();
}

void OverlayPanel::mouseMoveEvent(QMouseEvent *e)
{
    m_mouse = e->localPos();
    bool changed = false;
    for (int i = 0; i < m_buttons.size(); ++i) {
        bool h = m_buttons[i].r.contains(m_mouse) && m_buttons[i].enabled;
        if (h != m_buttons[i].hover) { m_buttons[i].hover = h; changed = true; }
        float tgt = h ? 1.f : 0.f;
        float nh = m_buttons[i].ht + (tgt - m_buttons[i].ht) * 0.28f;
        if (qAbs(nh - m_buttons[i].ht) > 0.002f) changed = true;
        m_buttons[i].ht = nh;
    }
    onMove(m_mouse);
    if (changed) update();
    e->accept();
}

void OverlayPanel::mouseReleaseEvent(QMouseEvent *e)
{
    m_mouse = e->localPos();
    if (m_pressedBtn >= 0 && m_pressedBtn < m_buttons.size()) {
        Btn &b = m_buttons[m_pressedBtn];
        b.down = false;
        if (b.r.contains(m_mouse) && b.enabled) {
            Audio::inst().play(Sfx_Ui);
            int id = b.id;
            m_pressedBtn = -1;
            if (id == Act_Close) { hide(); emit closed(); }
            else emit actionTriggered(id);
            update();
            e->accept();
            return;
        }
        m_pressedBtn = -1;
    }
    update();
    e->accept();
}

void OverlayPanel::keyPressEvent(QKeyEvent *e)
{
    if (Binds::matches(e->key(), Binds::A_Pause)) { hide(); emit closed(); e->accept(); return; }
    e->ignore();
}

void OverlayPanel::resizeEvent(QResizeEvent *e) { QWidget::resizeEvent(e); update(); }

// ============================================================
//  ИНВЕНТАРЬ
// ============================================================
InventoryPanel::InventoryPanel(QWidget *parent)
    : OverlayPanel(parent), m_hoverBag(-1), m_hoverEquip(Slot_Weapon), m_hoveringEquip(false),
      m_dragFrom(-1), m_dragging(false), m_tab(0), m_filter(0)
{
}

void InventoryPanel::refresh() { update(); }

// Сумка с фильтром: видимые ячейки отображают подходящие предметы подряд.
void InventoryPanel::rebuildCellMap() const
{
    m_cellMap.clear();
    if (!m_game) return;
    const QVector<Item> &bag = m_game->bag();
    for (int i = 0; i < bag.size(); ++i) {
        const Item &it = bag.at(i);
        if (!it.valid) continue;
        if (m_filter == 0) { m_cellMap.append(i); continue; }
        const ItemBaseDef *b = Data::base(it.baseId);
        int bk = b ? int(b->bag) : int(Bag_Misc);
        bool ok = (m_filter == 1) ? (bk == int(Bag_Equip))
                : (m_filter == 2) ? (bk == int(Bag_Potion))
                                  : (bk != int(Bag_Equip) && bk != int(Bag_Potion));
        if (ok) m_cellMap.append(i);
    }
}

QRectF InventoryPanel::bagRect(const QRectF &panel) const
{
    float left = panel.left() + 16.f + 250.f + 12.f + 262.f + 12.f;
    return QRectF(left, panel.top() + 72, panel.right() - left - 16, panel.height() - 72 - 56);
}

QRectF InventoryPanel::bagCell(const QRectF &panel, int idx) const
{
    QRectF br = bagRect(panel);
    // сверху внутри сумки: заголовок (20) и ряд кнопок-фильтров (28)
    QRectF gr(br.left() + 6, br.top() + 50, br.width() - 12, br.height() - 56);
    int cols = 10;
    float gap = 4.f;
    float cw = (gr.width() - gap * (cols - 1)) / float(cols);
    float ch = (gr.height() - gap * 5) / 6.f;
    float c = qMin(cw, ch);
    int col = idx % cols, row = idx / cols;
    float x0 = gr.left() + (gr.width() - (c * cols + gap * (cols - 1))) * 0.5f;
    float y0 = gr.top() + (gr.height() - (c * 6 + gap * 5)) * 0.5f;
    return QRectF(x0 + col * (c + gap), y0 + row * (c + gap), c, c);
}

QRectF InventoryPanel::equipRect(const QRectF &panel, Slot s) const
{
    float cx = panel.left() + 141.f;
    float cy = panel.top() + 72.f + (panel.height() - 128.f) * 0.40f;
    float sz = 50.f;
    QPointF c;
    switch (s) {
    case Slot_Helm:    c = QPointF(cx, cy - 150); break;
    case Slot_Amulet:  c = QPointF(cx - 72, cy - 108); break;
    case Slot_Chest:   c = QPointF(cx, cy - 60); break;
    case Slot_Gloves:  c = QPointF(cx - 78, cy - 18); break;
    case Slot_Ring2:   c = QPointF(cx + 78, cy - 78); break;
    case Slot_Ring1:   c = QPointF(cx + 78, cy - 18); break;
    case Slot_Belt:    c = QPointF(cx, cy + 18); break;
    case Slot_Weapon:  c = QPointF(cx - 96, cy + 66); break;
    case Slot_Offhand: c = QPointF(cx + 96, cy + 66); break;
    case Slot_Boots:   c = QPointF(cx, cy + 110); break;
    default:           c = QPointF(cx, cy); break;
    }
    return QRectF(c.x() - sz * 0.5f, c.y() - sz * 0.5f, sz, sz);
}

int InventoryPanel::bagIndexAt(const QPointF &p) const
{
    QRectF panel = panelRect();
    rebuildCellMap();
    for (int i = 0; i < Game::BAG_SIZE; ++i) {
        if (!bagCell(panel, i).contains(p)) continue;
        if (i < m_cellMap.size()) return m_cellMap.at(i);
        // пустая видимая ячейка: первый свободный слот сумки,
        // чтобы перетаскивание работало и при включённом фильтре
        const QVector<Item> &bag = m_game->bag();
        for (int k = 0; k < bag.size(); ++k) if (!bag.at(k).valid) return k;
        return -1;
    }
    return -1;
}

Slot InventoryPanel::equipSlotAt(const QPointF &p) const
{
    QRectF panel = panelRect();
    for (int i = 0; i < Slot_EquipCount; ++i)
        if (equipRect(panel, Slot(i)).contains(p)) return Slot(i);
    return Slot_EquipCount;
}

void InventoryPanel::showItemTooltip(const Item &it, const QPoint &gp)
{
    Tooltip::instance(window())->showItem(it, m_game, gp);
}

void InventoryPanel::paintPanel(QPainter *p, const QRectF &r)
{
    clearButtons();
    drawTitle(p, r, QStringLiteral("Снаряжение и сумка"),
              QStringLiteral("%1 · %2 уровень · золота: %3").arg(m_game->heroName()).arg(m_game->level()).arg(m_game->gold()),
              Gl_Bag);
    addButton(QRectF(r.right() - 46, r.top() + 14, 32, 32), QString(), Gl_Cross, Act_Close);

    const Derived &d = m_game->d();

    // ---------------- герой и слоты ----------------
    QRectF doll(r.left() + 16, r.top() + 72, 250, r.height() - 72 - 56);
    p->save();
    p->setBrush(QColor(12, 10, 20, 150));
    p->setPen(QPen(C::edge.darker(150), 1.2));
    p->drawRect(doll);
    QRadialGradient bgc(doll.center(), doll.width() * 0.66f);
    bgc.setColorAt(0, QColor(76, 58, 92, 110));
    bgc.setColorAt(1, QColor(0, 0, 0, 0));
    p->fillRect(doll, bgc);
    p->setClipRect(doll.adjusted(1, 1, -1, -1));
    {
        // живой герой в текущем снаряжении - полупрозрачным фоном под слотами
        Actor fake;
        QPointF feet(doll.center().x(), doll.center().y() + doll.height() * 0.30f);
        fake.pos = feet;
        fake.radius = 14.f;
        fake.facing = -1.05f;
        fake.animT = m_time * 0.7f;
        const Item wq = m_game->equipped(Slot_Weapon);
        const ItemBaseDef *wb = wq.valid ? Data::base(wq.baseId) : 0;
        const Item oq = m_game->equipped(Slot_Offhand);
        const ItemBaseDef *ob = oq.valid ? Data::base(oq.baseId) : 0;
        float sc = doll.height() / 215.f;
        p->setOpacity(0.62f);
        p->save();
        p->translate(feet);
        p->scale(sc, sc);
        p->translate(-feet);
        Spr::drawPlayer(p, fake, m_time, m_game->classId(), wb ? wb->glyph : Gl_Sword,
                        ob && ob->glyph == Gl_Shield);
        p->restore();
        p->setOpacity(1.0f);
    }
    p->restore();
    Style::textShadow(p, QRectF(doll.left(), doll.top() + 4, doll.width(), 18),
                      QStringLiteral("%1 · %2 ур.").arg(m_game->heroName(), QString::number(m_game->level())),
                      C::amber, fontHead(12), Qt::AlignCenter);

    // слоты экипировки: пустой - гравировка, надетый - пластина с полосой редкости
    m_hoveringEquip = false;
    for (int i = 0; i < Slot_EquipCount; ++i) {
        Slot sl = Slot(i);
        QRectF sr = equipRect(r, sl);
        Item it = m_game->equipped(sl);
        bool hover = sr.contains(m_mouse);
        if (hover) { m_hoverEquip = sl; m_hoveringEquip = true; }
        int slotGlyph;
        switch (sl) {
        case Slot_Weapon: slotGlyph = Gl_Sword; break;
        case Slot_Offhand: slotGlyph = Gl_Shield; break;
        case Slot_Helm: slotGlyph = Gl_Helm; break;
        case Slot_Chest: slotGlyph = Gl_Chest; break;
        case Slot_Gloves: slotGlyph = Gl_Gloves; break;
        case Slot_Belt: slotGlyph = Gl_Belt; break;
        case Slot_Boots: slotGlyph = Gl_Boots; break;
        case Slot_Amulet: slotGlyph = Gl_Amulet; break;
        default: slotGlyph = Gl_Ring; break;
        }
        p->save();
        p->setRenderHint(QPainter::Antialiasing, true);
        if (it.valid) {
            const ItemBaseDef *b = Data::base(it.baseId);
            QColor tc = rarityColor(it.rarity);
            p->setBrush(QColor(14, 11, 22, 215));
            p->setPen(QPen(hover ? C::amber : QColor(tc.red(), tc.green(), tc.blue(), 130), hover ? 2.0f : 1.3f));
            p->drawRoundedRect(sr, 5, 5);
            QRectF gr = sr.adjusted(6, 5, 6, 8);
            if (it.rarity >= Rar_Rare) Style::glow(p, gr.center(), 26.f, tc, 0.30f);
            drawGlyph(p, b ? b->glyph : slotGlyph, gr, tc.lighter(115), tc, it.rarity >= Rar_Rare ? 0.25f : 0.10f);
            p->fillRect(QRectF(sr.left() + 3, sr.bottom() - 5, sr.width() - 6, 3), tc);
        } else {
            p->setPen(QPen(QColor(158, 146, 168, hover ? 150 : 70), 1.2, Qt::DashLine));
            p->setBrush(hover ? QColor(255, 220, 150, 18) : Qt::NoBrush);
            p->drawRoundedRect(sr, 5, 5);
            p->setOpacity(hover ? 0.45f : 0.26f);
            drawGlyph(p, slotGlyph, sr.adjusted(10, 10, -10, -10), C::boneDim);
            p->setOpacity(1.0);
        }
        p->restore();
    }
    Style::textShadow(p, QRectF(doll.left(), doll.bottom() - 20, doll.width(), 16),
                      QStringLiteral("ПКМ по слоту - снять"), C::boneDim.darker(120), fontSmall(9), Qt::AlignCenter);

    // ---------------- характеристики ----------------
    float sx = doll.right() + 12;
    QRectF stats(sx, doll.top(), 262.f, doll.height());
    p->save();
    p->setBrush(QColor(12, 10, 20, 150));
    p->setPen(QPen(C::edge.darker(150), 1.2));
    p->drawRect(stats);
    p->restore();
    Style::textShadow(p, QRectF(stats.left(), stats.top() + 4, stats.width(), 20),
                      QStringLiteral("ХАРАКТЕРИСТИКИ"), C::amber, fontHead(13), Qt::AlignCenter);
    Style::divider(p, stats.top() + 26, stats.left() + 8, stats.right() - 8, C::edge.darker(120));

    struct Row { QString k; QString v; QColor c; };
    QVector<Row> rows;
    rows.append(Row{ QStringLiteral("Здоровье"), QStringLiteral("%1 / %2").arg(int(m_game->hp())).arg(int(d.maxHp)), C::hpHi });
    rows.append(Row{ QStringLiteral("Мана"), QStringLiteral("%1 / %2").arg(int(m_game->mana())).arg(int(d.maxMana)), C::manaHi });
    if (d.shieldMax > 0) rows.append(Row{ QStringLiteral("Барьер"), QStringLiteral("%1 / %2").arg(int(m_game->shield())).arg(int(d.shieldMax)), C::frost });
    rows.append(Row{ QStringLiteral("Живучесть"), QString::number(int(d.vit)), C::hpHi });
    rows.append(Row{ QStringLiteral("Мощь"), QString::number(int(d.might)), C::ember });
    rows.append(Row{ QStringLiteral("Дух"), QString::number(int(d.spirit)), C::arcane });
    rows.append(Row{ QStringLiteral("Ловкость"), QString::number(int(d.agi)), C::good });
    rows.append(Row{ QStringLiteral("Концентрация"), QString::number(int(d.focus)), C::manaHi });
    rows.append(Row{ QStringLiteral("Урон"), QStringLiteral("%1-%2").arg(int(d.dmgMin)).arg(int(d.dmgMax)), C::bone });
    rows.append(Row{ QStringLiteral("Атак/сек"), QString::number(double(d.weaponSpeed), 'f', 2), C::bone });
    rows.append(Row{ QStringLiteral("УВС"), QString::number(int((d.dmgMin + d.dmgMax) * 0.5f * d.weaponSpeed)), C::amber });
    rows.append(Row{ QStringLiteral("Броня"), QString::number(int(d.armor)), C::bone });
    rows.append(Row{ QStringLiteral("Шанс крита"), QStringLiteral("%1%").arg(QString::number(double(d.critChance * 100), 'f', 1)), C::lightning });
    rows.append(Row{ QStringLiteral("Крит. урон"), QStringLiteral("%1%").arg(int(d.critMult * 100)), C::lightning });
    rows.append(Row{ QStringLiteral("Уклонение"), QStringLiteral("%1%").arg(QString::number(double(d.dodge * 100), 'f', 1)), C::good });
    rows.append(Row{ QStringLiteral("Скорость"), QString::number(int(d.moveSpeed)), C::bone });
    rows.append(Row{ QStringLiteral("Реген. здоровья"), QStringLiteral("%1/с").arg(QString::number(double(d.hpRegen), 'f', 1)), C::good });
    rows.append(Row{ QStringLiteral("Реген. маны"), QStringLiteral("%1/с").arg(QString::number(double(d.manaRegen), 'f', 1)), C::manaHi });
    rows.append(Row{ QStringLiteral("Вампиризм"), QStringLiteral("%1%").arg(QString::number(double(d.lifeSteal * 100), 'f', 1)), C::bloodHi });
    rows.append(Row{ QStringLiteral("Откат умений"), QStringLiteral("-%1%").arg(int(d.cdReduction * 100)), C::arcane });
    rows.append(Row{ QStringLiteral("Шипы"), QString::number(int(d.thorns)), C::boneDim });
    rows.append(Row{ QStringLiteral("Сопр. физ."), QStringLiteral("%1%").arg(int(d.res[Dmg_Physical] * 100)), C::boneDim });
    rows.append(Row{ QStringLiteral("Сопр. огню"), QStringLiteral("%1%").arg(int(d.res[Dmg_Fire] * 100)), C::ember });
    rows.append(Row{ QStringLiteral("Сопр. холоду"), QStringLiteral("%1%").arg(int(d.res[Dmg_Frost] * 100)), C::frost });
    rows.append(Row{ QStringLiteral("Сопр. молнии"), QStringLiteral("%1%").arg(int(d.res[Dmg_Lightning] * 100)), C::lightning });
    rows.append(Row{ QStringLiteral("Сопр. яду"), QStringLiteral("%1%").arg(int(d.res[Dmg_Poison] * 100)), C::poison });
    rows.append(Row{ QStringLiteral("Сопр. магии"), QStringLiteral("%1%").arg(int(d.res[Dmg_Arcane] * 100)), C::arcane });
    rows.append(Row{ QStringLiteral("Редкости"), QStringLiteral("+%1%").arg(int(d.magicFind * 100)), C::gold });
    rows.append(Row{ QStringLiteral("Золото"), QStringLiteral("+%1%").arg(int(d.goldFind * 100)), C::gold });

    float yy = stats.top() + 34;
    QFont fk = fontSmall(10), fv = fontSmall(10);
    // одна колонка во всю ширину блока: и названия, и «768 / 768» влезают без обрезки
    float colW = stats.width() - 20;
    for (int i = 0; i < rows.size(); ++i) {
        float cx = stats.left() + 10;
        float ry = yy + i * 16.f;
        if (ry > stats.bottom() - 12) break;
        if (i % 2 == 0) {
            p->save();
            p->fillRect(QRectF(cx - 4, ry - 1, colW, 15), QColor(255, 255, 255, 7));
            p->restore();
        }
        Style::textShadow(p, QRectF(cx, ry, colW - 96, 14), rows.at(i).k, C::boneDim, fk,
                          Qt::AlignLeft | Qt::AlignVCenter);
        Style::textShadow(p, QRectF(cx + colW - 92, ry, 92, 14), rows.at(i).v, rows.at(i).c, fv,
                          Qt::AlignRight | Qt::AlignVCenter);
    }
    // ---------------- сумка ----------------
    QRectF br = bagRect(r);
    p->save();
    p->setBrush(QColor(12, 10, 20, 150));
    p->setPen(QPen(C::edge.darker(150), 1.2));
    p->drawRect(br);
    p->restore();

    // заголовок внутри панели (не выпирает наружу)
    int shown = 0;
    rebuildCellMap();
    shown = m_cellMap.size();
    Style::textShadow(p, QRectF(br.left(), br.top() + 4, br.width(), 18),
                      (m_filter == 0)
                        ? QStringLiteral("СУМКА  (%1/%2)").arg(Game::BAG_SIZE - m_game->bagFreeSlots()).arg(Game::BAG_SIZE)
                        : QStringLiteral("СУМКА  (%1/%2) · фильтр: %3")
                              .arg(Game::BAG_SIZE - m_game->bagFreeSlots()).arg(Game::BAG_SIZE).arg(shown),
                      C::amber, fontHead(12), Qt::AlignCenter);

    // фильтры-вкладки: всё / экипировка / зелья / прочее
    {
        static const char *fn[4] = { "\xD0\x92\xD1\x81\xD0\xB5",                 // Все
                                     "\xD0\xAD\xD0\xBA\xD0\xB8\xD0\xBF.",         // Экип.
                                     "\xD0\x97\xD0\xB5\xD0\xBB\xD1\x8C\xD1\x8F", // Зелья
                                     "\xD0\x9F\xD1\x80\xD0\xBE\xD1\x87\xD0\xB5\xD0\xB5" }; // Прочее
        float fw = (br.width() - 24 - 3 * 6) / 4.f;
        for (int i = 0; i < 4; ++i) {
            QRectF fr(br.left() + 12 + i * (fw + 6), br.top() + 24, fw, 22);
            int bi = addButton(fr, QString::fromUtf8(fn[i]), Gl_None, Act_InvFilter0 + i);
            if (bi >= 0) m_buttons[bi].active = (m_filter == i);
        }
    }

    m_hoverBag = -1;
    const QVector<Item> &bag = m_game->bag();
    for (int cell = 0; cell < Game::BAG_SIZE; ++cell) {
        QRectF cr = bagCell(r, cell);
        bool hover = cr.contains(m_mouse) && !m_dragging;
        if (hover) m_hoverBag = cell;
        int realIdx = (cell < m_cellMap.size()) ? m_cellMap.at(cell) : -1;
        Item it = (realIdx >= 0 && realIdx < bag.size()) ? bag.at(realIdx) : Item();
        bool dragging = (m_dragging && realIdx >= 0 && m_dragFrom == 100 + realIdx);
        Style::slotCell(p, cr, hover, dragging, false);
        if (!it.valid || dragging) continue;
        const ItemBaseDef *b = Data::base(it.baseId);
        if (!b) continue;
        QColor tc = rarityColor(it.rarity);

        // глиф предмета
        QRectF gr = cr.adjusted(5, 4, 5, 7);
        if (it.rarity >= Rar_Legend) Style::glow(p, gr.center(), 26.f, tc, 0.34f);
        else if (it.rarity == Rar_Rare) Style::glow(p, gr.center(), 22.f, tc, 0.18f);
        drawGlyph(p, b->glyph, gr, tc.lighter(115), tc, it.rarity >= Rar_Rare ? 0.22f : 0.f);

        // полоса редкости снизу
        p->fillRect(QRectF(cr.left() + 2, cr.bottom() - 4, cr.width() - 4, 3), tc);

        // количество
        if (it.count > 1)
            Style::textShadow(p, QRectF(cr.right() - 22, cr.bottom() - 16, 20, 13), QString::number(it.count),
                              C::bone, fontSmall(9), Qt::AlignRight);

        // уровень предмета (мелко, слева внизу)
        if (b->bag == Bag_Equip)
            Style::textShadow(p, QRectF(cr.left() + 3, cr.bottom() - 16, 20, 13), QString::number(it.ilvl),
                              C::boneDim.darker(110), fontSmall(8), Qt::AlignLeft);

        // стрелка сравнения с надетым
        if (b->bag == Bag_Equip) {
            Slot cmp = b->slot;
            Item cur = m_game->equipped(cmp);
            if (cmp == Slot_Ring1 && !cur.valid) cur = m_game->equipped(Slot_Ring2);
            int delta = m_game->itemScore(it) - (cur.valid ? m_game->itemScore(cur) : 0);
            if (!cur.valid || delta != 0) {
                QPointF ac(cr.right() - 8, cr.top() + 9);
                QColor ac_c = !cur.valid ? C::boneDim : (delta > 0 ? C::good : C::bad);
                QPolygonF ar;
                if (!cur.valid || delta > 0)
                    ar << ac + QPointF(0, -5) << ac + QPointF(4.5f, 2.5f) << ac + QPointF(-4.5f, 2.5f);
                else
                    ar << ac + QPointF(0, 5) << ac + QPointF(4.5f, -2.5f) << ac + QPointF(-4.5f, -2.5f);
                p->setPen(QPen(QColor(0, 0, 0, 160), 1.0));
                p->setBrush(ac_c);
                p->drawPolygon(ar);
            }
        }
    }
    if (m_filter != 0 && shown == 0)
        Style::textShadow(p, QRectF(br.left(), br.top() + 56, br.width(), 20),
                          QStringLiteral("Пусто - под фильтром ничего нет"), C::boneDim.darker(110),
                          fontSmall(11), Qt::AlignCenter);

    // ---------------- низ ----------------
    float by = r.bottom() - 46;
    addButton(QRectF(r.left() + 16, by, 130, 32), QStringLiteral("Закрыть"), Gl_Cross, Act_Close);
    addButton(QRectF(r.left() + 156, by, 140, 32), QStringLiteral("Снять всё"), Gl_Chest, Act_InvUnequipAll);
    int sortMode = QSettings(QStringLiteral("AshAmber"), QStringLiteral("PepeL_i_Yantar"))
                       .value(QStringLiteral("invsort"), 0).toInt() % 3;
    static const char *sn[3] = { "\xD0\xA1\xD0\xBE\xD1\x80\xD1\x82: \xD1\x82\xD0\xB8\xD0\xBF",      // Сорт: тип
                                 "\xD0\xA1\xD0\xBE\xD1\x80\xD1\x82: \xD1\x80\xD0\xB5\xD0\xB4\xD0\xBA.", // Сорт: редк.
                                 "\xD0\xA1\xD0\xBE\xD1\x80\xD1\x82: \xD1\x83\xD1\x80\xD0\xBE\xD0\xB2." }; // Сорт: уров.
    addButton(QRectF(r.left() + 306, by, 160, 32), QString::fromUtf8(sn[sortMode]), Gl_Bag, Act_InvSortBag);
    Style::textShadow(p, QRectF(r.left() + 476, by, r.right() - 476 - 190, 32),
                      QStringLiteral("ЛКМ - взять · ПКМ/Ctrl+ЛКМ - надеть или выпить · Shift+ЛКМ - в первый свободный · F - фильтр"),
                      C::boneDim.darker(110), fontSmall(9), Qt::AlignLeft | Qt::AlignVCenter);
    QRectF goldR(r.right() - 170, by, 154, 32);
    Style::panel(p, goldR, 1, 0.95f);
    drawGlyph(p, Gl_Coins, QRectF(goldR.left() + 8, goldR.top() + 6, 20, 20), C::gold);
    Style::textShadow(p, QRectF(goldR.left() + 32, goldR.top(), goldR.width() - 40, goldR.height()),
                      QString::number(m_game->gold()), C::gold, fontNum(15, true), Qt::AlignLeft | Qt::AlignVCenter);

    // ---------------- перетаскиваемый предмет ----------------
    if (m_dragging && m_dragItem.valid) {
        const ItemBaseDef *b = Data::base(m_dragItem.baseId);
        QColor tc = rarityColor(m_dragItem.rarity);
        QRectF gr(m_dragPos.x() - 22, m_dragPos.y() - 22, 44, 44);
        p->save();
        p->setOpacity(0.92f);
        p->setBrush(QColor(14, 11, 22, 200));
        p->setPen(QPen(tc, 1.8));
        p->drawRect(gr);
        Style::glow(p, gr.center(), 34.f, tc, 0.4f);
        if (b) drawGlyph(p, b->glyph, gr.adjusted(5, 5, -5, -5), tc.lighter(120), tc, 0.3f);
        p->restore();
    }
}

void InventoryPanel::useOrEquipAt(int bi)
{
    Item it = m_game->bag().at(bi);
    if (!it.valid) return;
    const ItemBaseDef *b = Data::base(it.baseId);
    if (!b) return;
    if (b->bag == Bag_Equip) {
        if (!m_game->equipFromBag(bi)) Audio::inst().play(Sfx_UiBad);
    } else if (b->bag == Bag_Potion) {
        if (m_game->usePotionAt(bi)) Audio::inst().play(Sfx_Heal);
        else Audio::inst().play(Sfx_UiBad);
    } else if (b->id_str == QLatin1String("m_tp")) {
        if (m_game->useScrollOfReturn()) { hide(); emit actionTriggered(900); }
        else Audio::inst().play(Sfx_UiBad);
    } else if (b->id_str == QLatin1String("m_soul")) {
        m_game->useSoulShard();
    }
}

void InventoryPanel::onClick(const QPointF &pos, Qt::MouseButton button)
{
    QRectF panel = panelRect();
    int bi = bagIndexAt(pos);
    Slot es = equipSlotAt(pos);
    Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();

    if (button == Qt::LeftButton) {
        if (bi >= 0) {
            Item it = m_game->bag().at(bi);
            if (it.valid) {
                // Ctrl+ЛКМ - быстро надеть/выпить, не беря в руку
                if (mods & Qt::ControlModifier) { useOrEquipAt(bi); return; }
                // Shift+ЛКМ - переложить в первый свободный слот
                if (mods & Qt::ShiftModifier) {
                    const QVector<Item> &bag = m_game->bag();
                    for (int k = 0; k < bag.size(); ++k)
                        if (!bag.at(k).valid && k != bi) { m_game->moveInBag(bi, k); break; }
                    return;
                }
                if (m_dragging && m_dragItem.valid) {
                    // бросаем в сумку
                    int from = m_dragFrom;
                    m_dragging = false;
                    if (from >= 0 && from < Slot_EquipCount) {
                        m_game->unequipToBag(Slot(from));
                    } else if (from >= 100) {
                        m_game->moveInBag(from - 100, bi);
                    }
                    m_dragFrom = -1;
                    m_dragItem = Item();
                    return;
                }
                m_dragFrom = 100 + bi;
                m_dragItem = it;
                m_dragging = true;
                m_dragPos = pos;
            }
            return;
        }
        if (es < Slot_EquipCount) {
            Item it = m_game->equipped(es);
            if (it.valid) {
                m_dragFrom = int(es);
                m_dragItem = it;
                m_dragging = true;
                m_dragPos = pos;
            }
            return;
        }
        // клик мимо - сброс перетаскивания
        if (m_dragging) { m_dragging = false; m_dragFrom = -1; m_dragItem = Item(); }
        return;
    }

    if (button == Qt::RightButton) {
        if (m_dragging) { m_dragging = false; m_dragFrom = -1; m_dragItem = Item(); return; }
        if (bi >= 0) { useOrEquipAt(bi); return; }
        if (es < Slot_EquipCount) { m_game->unequipToBag(es); return; }
    }
    Q_UNUSED(panel);
}

void InventoryPanel::mouseReleaseEvent(QMouseEvent *e)
{
    QPointF pos = e->localPos();
    OverlayPanel::mouseReleaseEvent(e);
    if (!m_dragging || e->button() != Qt::LeftButton) return;
    QRectF panel = panelRect();
    int bi = bagIndexAt(pos);
    Slot es = equipSlotAt(pos);
    Item it = m_dragItem;
    int from = m_dragFrom;
    m_dragging = false;
    m_dragFrom = -1;
    m_dragItem = Item();
    if (!it.valid) return;
    const ItemBaseDef *b = Data::base(it.baseId);

    if (es < Slot_EquipCount && b && b->bag == Bag_Equip) {
        Slot target = b->slot;
        if (target == Slot_Ring1) target = (es == Slot_Ring2) ? Slot_Ring2 : Slot_Ring1;
        if (target == es) {
            if (from >= 100) {
                if (!m_game->equipFromBag(from - 100)) Audio::inst().play(Sfx_UiBad);
            }
        } else {
            Audio::inst().play(Sfx_UiBad);
        }
        return;
    }
    if (bi >= 0) {
        if (from >= 100 && from - 100 != bi) m_game->moveInBag(from - 100, bi);
        else if (from >= 0 && from < Slot_EquipCount) m_game->unequipToBag(Slot(from));
        return;
    }
    Q_UNUSED(panel);
}

void InventoryPanel::keyPressEvent(QKeyEvent *e)
{
    // F - переключить фильтр сумки, S - сортировка
    if (e->key() == Qt::Key_F) {
        m_filter = (m_filter + 1) % 4;
        Audio::inst().play(Sfx_Ui);
        update();
        e->accept();
        return;
    }
    if (e->key() == Qt::Key_S) {
        emit actionTriggered(Act_InvSortBag);
        e->accept();
        return;
    }
    OverlayPanel::keyPressEvent(e);
}

void InventoryPanel::onMove(const QPointF &pos)
{
    if (m_dragging) m_dragPos = pos;
    QRectF panel = panelRect();
    int bi = bagIndexAt(pos);
    Slot es = equipSlotAt(pos);
    QPoint gp = mapToGlobal(QPoint(int(pos.x()), int(pos.y())));
    if (bi >= 0 && bi < m_game->bag().size() && m_game->bag().at(bi).valid) {
        showItemTooltip(m_game->bag().at(bi), gp);
        return;
    }
    if (es < Slot_EquipCount && m_game->equipped(es).valid) {
        showItemTooltip(m_game->equipped(es), gp);
        return;
    }
    Tooltip::instance(window())->hideNow();
    Q_UNUSED(panel);
}

void InventoryPanel::onLeave() { Tooltip::instance(window())->hideNow(); }


// ============================================================
//  ПАУТИНА НАВЫКОВ
// ============================================================
SkillWebPanel::SkillWebPanel(QWidget *parent)
    : OverlayPanel(parent), m_zoom(0.85f), m_hover(-1), m_panning(false), m_pulse(0.f), m_filterBranch(-1)
{
}

void SkillWebPanel::refresh() { update(); }

QRectF SkillWebPanel::canvasRect(const QRectF &panel) const
{
    return QRectF(panel.left() + 14, panel.top() + 68, panel.width() - 28, panel.height() - 68 - 56);
}

QPointF SkillWebPanel::toScreen(const QPointF &world, const QRectF &canvas) const
{
    float R = qMin(float(canvas.width()), float(canvas.height())) * 0.52f * m_zoom;
    return QPointF(canvas.center().x() + world.x() * R + m_pan.x(),
                   canvas.center().y() + world.y() * R + m_pan.y());
}

QPointF SkillWebPanel::toWorld(const QPointF &screen, const QRectF &canvas) const
{
    float R = qMin(float(canvas.width()), float(canvas.height())) * 0.52f * m_zoom;
    if (R < 1.f) R = 1.f;
    return QPointF((screen.x() - canvas.center().x() - m_pan.x()) / R,
                   (screen.y() - canvas.center().y() - m_pan.y()) / R);
}

int SkillWebPanel::nodeAt(const QPointF &screenPos, const QRectF &canvas) const
{
    const QVector<SkillNode> &N = Data::nodes();
    int best = -1;
    float bd = 1e9f;
    for (int i = 0; i < N.size(); ++i) {
        QPointF sp = toScreen(Data::nodePos(N.at(i)), canvas);
        float rr = (N.at(i).kind == Nk_Keystone ? 26.f : (N.at(i).kind == Nk_Minor ? 15.f : 20.f)) * qMax(0.6f, m_zoom);
        float dx = float(sp.x() - screenPos.x()), dy = float(sp.y() - screenPos.y());
        float dd = qSqrt(dx * dx + dy * dy);
        if (dd < rr + 4.f && dd < bd) { bd = dd; best = i; }
    }
    return best;
}

void SkillWebPanel::paintPanel(QPainter *p, const QRectF &r)
{
    clearButtons();
    drawTitle(p, r, QStringLiteral("Паутина навыков"),
              QStringLiteral("Свободных очков: %1  ·  изучено узлов: %2  ·  колесо - масштаб, ПКМ - двигать")
              .arg(m_game->skillPoints()).arg(m_game->spentPoints()), Gl_Web);
    addButton(QRectF(r.right() - 46, r.top() + 14, 32, 32), QString(), Gl_Cross, Act_Close);

    QRectF canvas = canvasRect(r);
    m_pulse += 0.03f;

    // фон паутины
    p->save();
    p->setClipRect(canvas);
    QRadialGradient bg(canvas.center(), qMax(float(canvas.width()), float(canvas.height())) * 0.7f);
    bg.setColorAt(0, QColor(26, 20, 40));
    bg.setColorAt(0.6, QColor(14, 11, 24));
    bg.setColorAt(1, QColor(7, 5, 13));
    p->fillRect(canvas, bg);
    // звёздная пыль
    Rng dust(4242);
    p->setPen(Qt::NoPen);
    for (int i = 0; i < 160; ++i) {
        float x = canvas.left() + dust.unit() * canvas.width();
        float y = canvas.top() + dust.unit() * canvas.height();
        float a = 30 + dust.unit() * 70;
        p->setBrush(QColor(200, 190, 255, int(a * (0.4f + 0.6f * qAbs(qSin(m_pulse * 0.6f + i))))));
        p->drawEllipse(QPointF(x, y), 1.1f, 1.1f);
    }
    float R = qMin(float(canvas.width()), float(canvas.height())) * 0.44f * m_zoom;
    QPointF C = QPointF(canvas.center().x() + m_pan.x(), canvas.center().y() + m_pan.y());

    // круги и спицы
    p->setBrush(Qt::NoBrush);
    for (int ring = 1; ring <= 5; ++ring) {
        float rr = Data::ringRadius(ring) * R;
        QColor cc = C::edgeHi;
        cc.setAlpha(ring == 5 ? 70 : 40);
        p->setPen(QPen(cc, ring == 5 ? 1.8 : 1.1, Qt::DashLine));
        p->drawEllipse(C, rr, rr);
    }
    for (int i = 0; i < 16; ++i) {
        float a = float(i) * float(M_PI) / 8.f;
        QColor cc = C::edgeHi;
        cc.setAlpha(26);
        p->setPen(QPen(cc, 1.0));
        p->drawLine(C, C + QPointF(qCos(a), qSin(a)) * R * 1.06f);
    }

    const QVector<SkillNode> &N = Data::nodes();

    // нити между узлами
    for (int i = 0; i < N.size(); ++i) {
        QPointF a = toScreen(Data::nodePos(N.at(i)), canvas);
        for (int k = 0; k < N.at(i).links.size(); ++k) {
            int j = N.at(i).links.at(k);
            if (j <= i) continue;
            QPointF b = toScreen(Data::nodePos(N.at(j)), canvas);
            bool taken = m_game->nodeUnlocked(i) && m_game->nodeUnlocked(j);
            bool half = m_game->nodeUnlocked(i) || m_game->nodeUnlocked(j);
            QColor lc = taken ? C::amber : (half ? C::edgeHi.darker(120) : C::edge.darker(190));
            lc.setAlpha(taken ? 210 : (half ? 130 : 70));
            p->setPen(QPen(lc, taken ? 2.4 : 1.3, taken ? Qt::SolidLine : Qt::SolidLine));
            // слегка провисающая нить
            QPointF mid = (a + b) * 0.5 + QPointF(0, taken ? 0 : 3);
            QPainterPath path;
            path.moveTo(a);
            path.quadTo(mid, b);
            p->drawPath(path);
            if (taken) {
                // бегущая искра
                float tt = fmodf(m_pulse * 0.35f + i * 0.13f, 1.f);
                QPointF sp = (1 - tt) * (1 - tt) * a + 2 * (1 - tt) * tt * mid + tt * tt * b;
                Style::glow(p, sp, 12.f, C::amber, 0.7f);
            }
        }
    }

    // узлы
    for (int i = 0; i < N.size(); ++i) {
        const SkillNode &n = N.at(i);
        if (m_filterBranch >= 0 && n.branch != m_filterBranch && n.kind != Nk_Origin) continue;
        QPointF sp = toScreen(Data::nodePos(n), canvas);
        if (!canvas.adjusted(-40, -40, 40, 40).contains(sp)) continue;
        int rank = m_game->nodeRank(i);
        bool taken = rank > 0;
        QString why;
        bool canTake = m_game->canTakeNode(i, &why);
        float rad = n.kind == Nk_Keystone ? 26.f : (n.kind == Nk_Minor ? 15.f : 20.f);
        rad *= qMax(0.6f, m_zoom);
        bool hover = (i == m_hover);

        // свечение доступных
        if (canTake && !taken) {
            float pulse = 0.45f + 0.35f * qSin(m_pulse * 3.f + i);
            Style::glow(p, sp, rad * 2.6f, n.color, pulse * 0.5f);
        }
        if (taken) Style::glow(p, sp, rad * 2.4f, n.color, 0.55f);

        // тело узла
        p->save();
        p->translate(sp);
        QColor fill = taken ? n.color : QColor(20, 16, 30);
        QColor line = taken ? n.color.lighter(150) : (canTake ? n.color : C::edge.darker(150));
        if (n.kind == Nk_Keystone) {
            QPainterPath path = runeRing(QPointF(0, 0), rad, 8, rad * 0.24f, m_pulse * 0.3f);
            p->setBrush(fill);
            p->setPen(QPen(line, hover ? 3.0 : 2.0));
            p->drawPath(path);
        } else if (n.kind == Nk_Active) {
            QPolygonF hex;
            for (int k = 0; k < 6; ++k) {
                float a = float(k) * float(M_PI) / 3.f + 0.52f;
                hex << QPointF(qCos(a) * rad, qSin(a) * rad);
            }
            p->setBrush(fill);
            p->setPen(QPen(line, hover ? 2.8 : 1.8));
            p->drawPolygon(hex);
        } else if (n.kind == Nk_Notable) {
            QPolygonF dia;
            dia << QPointF(0, -rad) << QPointF(rad, 0) << QPointF(0, rad) << QPointF(-rad, 0);
            p->setBrush(fill);
            p->setPen(QPen(line, hover ? 2.8 : 1.8));
            p->drawPolygon(dia);
        } else {
            p->setBrush(fill);
            p->setPen(QPen(line, hover ? 2.6 : 1.5));
            p->drawEllipse(QPointF(0, 0), rad, rad);
        }
        // иконка
        int gl = Gl_None;
        QColor gc = taken ? QColor(255, 250, 235) : (canTake ? n.color.lighter(120) : C::ash.darker(130));
        if (n.kind == Nk_Active && n.activeSkill >= 0) {
            const ActiveSkill *s = Data::skill(n.activeSkill);
            gl = s ? s->glyph : Gl_Star;
        } else if (n.kind == Nk_Keystone) gl = Gl_Crown2;
        else if (n.kind == Nk_Origin) gl = Gl_Star;
        else {
            // символ по типу бонуса
            const StatMods &m = n.mods;
            if (m.hpFlat > 0 || m.vit > 0) gl = Gl_Heart;
            else if (m.might > 0 || m.physPct > 0) gl = Gl_Swords;
            else if (m.spirit > 0 || m.arcanePct > 0 || m.firePct > 0) gl = Gl_Flask;
            else if (m.agi > 0 || m.dodge > 0 || m.moveSpeed > 0) gl = Gl_Dash;
            else if (m.focus > 0 || m.manaFlat > 0 || m.castSpeed > 0) gl = Gl_Star;
            else if (m.armor > 0 || m.armorPct > 0) gl = Gl_Shield;
            else if (m.critChance > 0 || m.critMult > 0) gl = Gl_Mark;
            else if (m.lifeSteal > 0) gl = Gl_Blood;
            else gl = Gl_Web;
        }
        QRectF gr(-rad * 0.70f, -rad * 0.70f, rad * 1.40f, rad * 1.40f);
        drawGlyph(p, gl, gr, gc, gc, taken ? 0.4f : 0.f);
        p->restore();

        // ранги
        if (n.maxRank > 1) {
            QString txt = QStringLiteral("%1/%2").arg(rank).arg(n.maxRank);
            QColor tc = taken ? C::good : C::boneDim;
            Style::textShadow(p, QRectF(sp.x() - 24, sp.y() + rad * 0.72f, 48, 13), txt, tc, fontSmall(9), Qt::AlignCenter);
        }
        if (hover) {
            p->setPen(QPen(C::bone, 1.4));
            p->setBrush(Qt::NoBrush);
            p->drawEllipse(sp, rad + 5.f, rad + 5.f);
            Style::textShadow(p, QRectF(sp.x() - 110, sp.y() - rad - 24, 220, 18), n.name, C::bone, fontHead(12),
                              Qt::AlignCenter);
        }
    }
    p->restore();

    // ---------------- низ панели ----------------
    float by = r.bottom() - 46;
    addButton(QRectF(r.left() + 16, by, 130, 32), QStringLiteral("Закрыть"), Gl_Cross, Act_Close);
    addButton(QRectF(r.left() + 154, by, 210, 32),
              QStringLiteral("Забыть всё (%1 зол.)").arg(m_game->respecCost()), Gl_Respec, Act_WebRespec,
              m_game->gold() >= m_game->respecCost());
    addButton(QRectF(r.left() + 372, by, 110, 32), QStringLiteral("В центр"), Gl_Star, Act_WebCenter);
    const char *names[5] = { "Все", "Воин", "Маг", "Стран.", "Жрец" };
    QColor cols[5] = { C::boneDim, QColor(226, 122, 74), QColor(122, 156, 246), QColor(146, 216, 140), QColor(206, 150, 236) };
    for (int i = 0; i < 5; ++i) {
        QRectF fr(r.left() + 492 + i * 68, by, 62, 32);
        bool sel = (m_filterBranch == i - 1);
        int bi = addButton(fr, QString::fromUtf8(names[i]), Gl_None, Act_WebFilterAll + i);
        if (bi >= 0) { m_buttons[bi].accent = sel ? cols[i] : QColor(); m_buttons[bi].active = sel; }
    }
    // легенда
    Style::textShadow(p, QRectF(r.left() + 840, by + 4, r.right() - r.left() - 856, 26),
                      QStringLiteral("круг - малый · ромб - заметный · шестигранник - умение · звезда - ключ"),
                      C::boneDim.darker(110), fontSmall(9), Qt::AlignLeft | Qt::AlignVCenter);
}

void SkillWebPanel::onClick(const QPointF &pos, Qt::MouseButton button)
{
    QRectF canvas = canvasRect(panelRect());
    int id = nodeAt(pos, canvas);
    if (button == Qt::LeftButton) {
        if (id >= 0) {
            if (!m_game->takeNode(id)) Audio::inst().play(Sfx_UiBad);
            else Audio::inst().play(Sfx_SkillPoint);
        }
        return;
    }
    if (button == Qt::RightButton) {
        if (id >= 0) {
            if (!m_game->refundNode(id)) Audio::inst().play(Sfx_UiBad);
            else Audio::inst().play(Sfx_Ui);
        } else {
            m_panning = true;
            m_panStart = pos;
            m_panOrigin = m_pan;
        }
    }
}

void SkillWebPanel::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::MiddleButton) {
        m_panning = true;
        m_panStart = e->localPos();
        m_panOrigin = m_pan;
        e->accept();
        return;
    }
    OverlayPanel::mousePressEvent(e);
}

void SkillWebPanel::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::MiddleButton || (e->button() == Qt::RightButton && m_panning)) {
        m_panning = false;
        e->accept();
        return;
    }
    OverlayPanel::mouseReleaseEvent(e);
}

void SkillWebPanel::onMove(const QPointF &pos)
{
    QRectF canvas = canvasRect(panelRect());
    if (m_panning) {
        m_pan = m_panOrigin + (pos - m_panStart);
        update();
        return;
    }
    int id = nodeAt(pos, canvas);
    if (id != m_hover) { m_hover = id; update(); }
    if (id >= 0) {
        Tooltip::instance(window())->showNode(id, m_game->nodeRank(id), m_game,
                                              mapToGlobal(QPoint(int(pos.x()), int(pos.y()))));
    } else Tooltip::instance(window())->hideNow();
}

void SkillWebPanel::onLeave()
{
    m_hover = -1;
    m_panning = false;
    Tooltip::instance(window())->hideNow();
}

void SkillWebPanel::wheelEvent(QWheelEvent *e)
{
    float k = e->angleDelta().y() > 0 ? 1.12f : 0.89f;
    m_zoom = qBound(0.42f, m_zoom * k, 2.6f);
    update();
    e->accept();
}

// ============================================================
//  КАРТА МИРА
// ============================================================
int WorldMapPanel::glyphForType(LocType t)
{
    switch (t) {
    case Loc_Town: return Gl_Town;
    case Loc_Camp: return Gl_Camp;
    case Loc_Ruins: return Gl_Ruins;
    case Loc_Cave: return Gl_Cave;
    case Loc_Forest: return Gl_Forest;
    case Loc_Swamp: return Gl_Swamp;
    case Loc_Fortress: return Gl_Fortress;
    case Loc_Shrine: return Gl_Shrine;
    case Loc_Elite: return Gl_Elite;
    case Loc_Boss: return Gl_Boss;
    case Loc_Final: return Gl_Crown2;
    default: return Gl_Camp;
    }
}

WorldMapPanel::WorldMapPanel(QWidget *parent)
    : OverlayPanel(parent), m_hover(-1), m_selected(-1), m_walk(0.f)
{
}

void WorldMapPanel::refresh()
{
    if (m_selected < 0) m_selected = m_game ? m_game->currentLocation() : 0;
    update();
}

QRectF WorldMapPanel::mapRect(const QRectF &panel) const
{
    return QRectF(panel.left() + 16, panel.top() + 68, panel.width() - 32 - 300, panel.height() - 68 - 56);
}

QPointF WorldMapPanel::locPos(int locId, const QRectF &mr) const
{
    const LocationDef *L = Data::loc(locId);
    if (!L) return mr.center();
    return QPointF(mr.left() + L->mapPos.x() * mr.width(), mr.top() + L->mapPos.y() * mr.height());
}

int WorldMapPanel::locAt(const QPointF &p, const QRectF &mr) const
{
    int best = -1;
    float bd = 1e9f;
    for (int i = 0; i < Data::locationCount(); ++i) {
        QPointF lp = locPos(i, mr);
        float dx = float(lp.x() - p.x()), dy = float(lp.y() - p.y());
        float dd = qSqrt(dx * dx + dy * dy);
        const LocationDef *L = Data::loc(i);
        float rr = (L && (L->type == Loc_Boss || L->type == Loc_Final)) ? 30.f : 23.f;
        if (dd < rr && dd < bd) { bd = dd; best = i; }
    }
    return best;
}

void WorldMapPanel::paintPanel(QPainter *p, const QRectF &r)
{
    clearButtons();
    drawTitle(p, r, QStringLiteral("Карта мира"),
              QStringLiteral("Выбери локацию и отправляйся. Зачищенные открывают соседние пути."), Gl_Map);
    addButton(QRectF(r.right() - 46, r.top() + 14, 32, 32), QString(), Gl_Cross, Act_Close);

    QRectF mr = mapRect(r);
    m_walk += 0.02f;

    // ---- статичный слой (пергамент, регионы, дороги, компас) кешируется ----
    if (m_baseSize != mr.size().toSize()) {
        m_base = QImage(mr.size().toSize(), QImage::Format_ARGB32);
        m_baseSize = mr.size().toSize();
        QPainter bp(&m_base);
        bp.setRenderHint(QPainter::Antialiasing, true);
        bp.fillRect(m_base.rect(), QColor(74, 60, 44));
        QImage tex = Style::noiseTile(77, QColor(74, 60, 44), 0.22f);
        bp.setPen(Qt::NoPen);
        bp.setBrush(QBrush(tex));
        bp.drawRect(m_base.rect());
        bp.setBrush(Qt::NoBrush);
        bp.fillRect(m_base.rect(), QColor(255, 226, 176, 22));
        QRadialGradient vg(QPointF(m_base.width() * 0.5, m_base.height() * 0.5),
                           qMax(m_base.width(), m_base.height()) * 0.72f);
        vg.setColorAt(0.4, QColor(255, 240, 200, 18));
        vg.setColorAt(1.0, QColor(20, 12, 6, 150));
        bp.fillRect(m_base.rect(), vg);
        QRectF local(0, 0, m_base.width(), m_base.height());
        for (int reg = 0; reg < Data::regionCount(); ++reg) {
            QVector<int> ls = Data::locationsOfRegion(reg);
            if (ls.isEmpty()) continue;
            QPointF c(0, 0);
            for (int i = 0; i < ls.size(); ++i) c += locPos(ls.at(i), mr);
            c /= qreal(ls.size());
            c -= mr.topLeft();
            float rad = local.width() * 0.13f;
            QRadialGradient g(c, rad);
            QColor rc = Data::regionColor(reg);
            rc.setAlpha(58);
            g.setColorAt(0, rc);
            rc.setAlpha(0);
            g.setColorAt(1, rc);
            bp.fillRect(QRectF(c.x() - rad, c.y() - rad, rad * 2, rad * 2), QBrush(g));
            Style::textShadow(&bp, QRectF(c.x() - 110, c.y() - rad - 16, 220, 20), Data::regionName(reg),
                              Data::regionColor(reg).lighter(130), fontHead(13), Qt::AlignCenter);
        }
        for (int i = 0; i < Data::locationCount(); ++i) {
            const LocationDef *L = Data::loc(i);
            if (!L) continue;
            QPointF a = locPos(i, mr) - mr.topLeft();
            for (int k = 0; k < L->links.size(); ++k) {
                int j = L->links.at(k);
                if (j <= i) continue;
                QPointF b = locPos(j, mr) - mr.topLeft();
                bool open = m_game->isUnlocked(i) && m_game->isUnlocked(j);
                bool walked = m_game->isCleared(i) && m_game->isCleared(j);
                QColor cc = walked ? C::amber.darker(120) : (open ? QColor(180, 150, 100) : QColor(90, 78, 62));
                bp.setPen(QPen(cc, walked ? 2.6 : 1.6, walked ? Qt::SolidLine : Qt::DashLine));
                QPointF mid = (a + b) * 0.5 + QPointF(0, -qMin(18.0, local.width() * 0.012));
                QPainterPath path;
                path.moveTo(a);
                path.quadTo(mid, b);
                bp.drawPath(path);
            }
        }
        QPointF cc(m_base.width() - 40, 40);
        bp.setPen(QPen(C::parchHi, 1.6));
        bp.setBrush(QColor(30, 22, 14, 120));
        bp.drawEllipse(cc, 26, 26);
        bp.setBrush(C::bad);
        QPolygonF nn;
        nn << cc + QPointF(0, -22) << cc + QPointF(6, 0) << cc + QPointF(0, 22) << cc + QPointF(-6, 0);
        bp.drawPolygon(nn);
        Style::textShadow(&bp, QRectF(cc.x() - 20, cc.y() - 40, 40, 14), QStringLiteral("С"), C::bone, fontSmall(10),
                          Qt::AlignCenter);
        bp.end();
    }
    p->save();
    p->setClipRect(mr);
    p->drawImage(mr.topLeft(), m_base);

    // бегущие искры по пройденным дорогам
    for (int i = 0; i < Data::locationCount(); ++i) {
        const LocationDef *L = Data::loc(i);
        if (!L) continue;
        for (int k = 0; k < L->links.size(); ++k) {
            int j = L->links.at(k);
            if (j <= i) continue;
            if (!(m_game->isCleared(i) && m_game->isCleared(j))) continue;
            QPointF a = locPos(i, mr), b = locPos(j, mr);
            QPointF mid = (a + b) * 0.5 + QPointF(0, -qMin(18.0, mr.width() * 0.012));
            float tt = fmodf(m_walk * 0.22f + i * 0.17f, 1.f);
            QPointF sp = (1 - tt) * (1 - tt) * a + 2 * (1 - tt) * tt * mid + tt * tt * b;
            p->setPen(Qt::NoPen);
            p->setBrush(QColor(255, 210, 130, 190));
            p->drawEllipse(sp, 2.0, 2.0);
        }
    }

    // ---- точки локаций ----
    for (int i = 0; i < Data::locationCount(); ++i) {
        const LocationDef *L = Data::loc(i);
        if (!L) continue;
        QPointF c = locPos(i, mr);
        bool unlocked = m_game->isUnlocked(i);
        bool cleared = m_game->isCleared(i);
        bool big = (L->type == Loc_Boss || L->type == Loc_Final || L->type == Loc_Town);
        float rad = big ? 21.f : 16.f;
        bool hover = (i == m_hover);
        bool sel = (i == m_selected);

        QColor col = unlocked ? (cleared ? C::amber : C::bone) : QColor(96, 88, 80);
        if (L->type == Loc_Boss || L->type == Loc_Final) col = unlocked ? C::bad : QColor(120, 70, 66);
        if (L->type == Loc_Town) col = unlocked ? C::gold : QColor(110, 96, 60);

        if (sel || hover) {
            float pulse = 0.5f + 0.5f * qSin(m_walk * 4.f);
            Style::glow(p, c, rad * (sel ? 3.4f : 2.6f), col, (sel ? 0.55f : 0.35f) * (0.6f + 0.4f * pulse));
            p->setPen(QPen(sel ? C::amber : C::bone, sel ? 2.4 : 1.6, sel ? Qt::DashLine : Qt::SolidLine));
            p->setBrush(Qt::NoBrush);
            p->drawEllipse(c, rad + 6.f, rad + 6.f);
        }
        // основание
        p->setBrush(QColor(18, 14, 22, 220));
        p->setPen(QPen(col, 2.0));
        p->drawEllipse(c, rad, rad);
        QRectF gr(c.x() - rad * 0.70f, c.y() - rad * 0.70f, rad * 1.40f, rad * 1.40f);
        drawGlyph(p, glyphForType(L->type), gr, unlocked ? col : col.darker(180), col.lighter(120),
                  unlocked ? 0.3f : 0.f);
        if (!unlocked) {
            drawGlyph(p, Gl_Lock, QRectF(c.x() + rad * 0.4f, c.y() - rad * 1.2f, 14, 14), QColor(150, 140, 130));
        } else if (cleared) {
            drawGlyph(p, Gl_Check, QRectF(c.x() + rad * 0.5f, c.y() - rad * 1.25f, 13, 13), C::good);
        }
        // подпись
        if (big || hover || sel) {
            QFont f = fontSmall(hover || sel ? 11 : 9);
            QFontMetrics fm(f);
            float w = fm.width(L->name) + 12;
            QRectF tr(c.x() - w * 0.5f, c.y() + rad + 2, w, 15);
            p->fillRect(tr, QColor(10, 8, 14, 170));
            Style::textShadow(p, tr, L->name, unlocked ? C::bone : C::ash.darker(120), f, Qt::AlignCenter);
        }
    }

    // отметка игрока
    {
        QPointF c = locPos(m_game->currentLocation(), mr);
        p->setPen(QPen(C::bone, 1.6));
        p->setBrush(Qt::NoBrush);
        float rr = 22.f + qSin(m_walk * 5.f) * 3.f;
        p->drawEllipse(c, rr, rr);
        Style::textShadow(p, QRectF(c.x() - 60, c.y() - 40, 120, 14), QStringLiteral("ты здесь"), C::bone,
                          fontSmall(9), Qt::AlignCenter);
    }

    p->restore();
    Style::frame(p, mr, C::edgeHi.darker(130), 2.4);
    Style::corners(p, mr, C::edgeHi.darker(110), 10.f);

    // ---- боковая карточка ----
    QRectF info(mr.right() + 14, mr.top(), r.right() - mr.right() - 30, mr.height());
    Style::panel(p, info, 1, 0.95f);
    int sel = m_selected >= 0 ? m_selected : m_game->currentLocation();
    const LocationDef *L = Data::loc(sel);
    if (L) {
        float y = info.top() + 12;
        QRectF iconR(info.left() + 12, y, 44, 44);
        Style::glow(p, iconR.center(), 36.f, Data::regionColor(L->region), 0.4f);
        drawGlyph(p, glyphForType(L->type), iconR, Data::regionColor(L->region).lighter(120), C::gold, 0.3f);
        Style::textShadow(p, QRectF(iconR.right() + 10, y + 2, info.right() - iconR.right() - 20, 22), L->name,
                          C::amber, fontHead(15), Qt::AlignLeft | Qt::AlignVCenter);
        {
            QFont fSub = fontSmall(10);
            QFontMetrics fmSub(fSub);
            float wSub = info.right() - iconR.right() - 24;
            Style::textShadow(p, QRectF(iconR.right() + 10, y + 24, wSub, 16),
                              fmSub.elidedText(QStringLiteral("%1 · %2").arg(locTypeName(L->type), biomeName(L->biome)),
                                               Qt::ElideRight, int(wSub)), C::boneDim, fSub,
                              Qt::AlignLeft | Qt::AlignVCenter);
        }
        y += 54;
        Style::divider(p, y, info.left() + 12, info.right() - 12, C::edge.darker(120));
        y += 8;

        QStringList facts;
        facts << QStringLiteral("Регион: %1").arg(Data::regionName(L->region));
        facts << QStringLiteral("Порог сложности: %1").arg(romanNum(L->tier));
        facts << QStringLiteral("Рекомендуемый уровень: ~%1").arg(L->reqLevel);
        if (L->enemyCount > 0) facts << QStringLiteral("Врагов: %1%2").arg(L->enemyCount)
                .arg(L->eliteCount > 0 ? QStringLiteral(" + элитных: %1").arg(L->eliteCount) : QString());
        if (L->bossId >= 0) {
            const EnemyDef *b = Data::enemy(L->bossId);
            facts << QStringLiteral("Босс: %1").arg(b ? (b->title.isEmpty() ? b->name : b->title) : QString("?"));
        }
        facts << QStringLiteral("Статус: %1").arg(m_game->isCleared(sel) ? QStringLiteral("зачищено")
                : (m_game->isUnlocked(sel) ? QStringLiteral("доступно") : QStringLiteral("закрыто")));
        if (!L->rewardHint.isEmpty()) facts << L->rewardHint;
        for (int i = 0; i < facts.size(); ++i) {
            QRectF fr(info.left() + 14, y, info.width() - 28, 16);
            Style::textShadow(p, fr, facts.at(i), i == facts.size() - 1 && !L->rewardHint.isEmpty() ? C::gold : C::bone,
                              fontSmall(11), Qt::AlignLeft | Qt::AlignVCenter);
            y += 17;
        }
        y += 4;
        // описание: перенос по словам, автоуменьшение шрифта, многоточие если не влезло
        QRectF dr(info.left() + 14, y, info.width() - 28, info.bottom() - y - 52);
        QFont f = fontSmall(11);
        f.setItalic(true);
        QStringList lines = wrapText(L->desc, f, dr.width());
        float lh = 15.f;
        int maxLines = qMax(1, int(dr.height() / lh));
        if (lines.size() > maxLines) {
            f = fontSmall(10);
            f.setItalic(true);
            lh = 13.f;
            lines = wrapText(L->desc, f, dr.width());
            maxLines = qMax(1, int(dr.height() / lh));
        }
        QFontMetrics fmD(f);
        for (int i = 0; i < lines.size() && i < maxLines; ++i) {
            QString t = lines.at(i);
            if (i == maxLines - 1 && lines.size() > maxLines)
                t = fmD.elidedText(t, Qt::ElideRight, int(dr.width() - 8)) + QStringLiteral("…");
            Style::textShadow(p, QRectF(dr.left(), dr.top() + i * lh, dr.width(), lh), t, C::boneDim, f,
                              Qt::AlignLeft | Qt::AlignVCenter);
        }

        bool canGo = m_game->isUnlocked(sel);
        float by = info.bottom() - 78;
        addButton(QRectF(info.left() + 12, by, info.width() - 24, 34),
                  canGo ? QStringLiteral("Отправиться в путь") : QStringLiteral("Путь закрыт"),
                  canGo ? Gl_Play : Gl_Lock, Act_MapTravel, canGo);
    }

    // ---- низ ----
    float by = r.bottom() - 46;
    addButton(QRectF(r.left() + 16, by, 130, 32), QStringLiteral("Закрыть"), Gl_Cross, Act_Close);
    Style::textShadow(p, QRectF(r.left() + 160, by, 500, 32),
                      QStringLiteral("ЛКМ - выбрать локацию · двойной клик или «Отправиться» - войти"),
                      C::boneDim, fontSmall(10), Qt::AlignLeft | Qt::AlignVCenter);
}

void WorldMapPanel::onClick(const QPointF &pos, Qt::MouseButton button)
{
    Q_UNUSED(button);
    QRectF mr = mapRect(panelRect());
    int id = locAt(pos, mr);
    if (id >= 0) {
        if (m_selected == id && m_game->isUnlocked(id)) { emit enterRequested(id); return; }
        m_selected = id;
        Audio::inst().play(Sfx_Ui);
        update();
    }
}

void WorldMapPanel::onMove(const QPointF &pos)
{
    QRectF mr = mapRect(panelRect());
    int id = locAt(pos, mr);
    if (id != m_hover) { m_hover = id; update(); }
    if (id >= 0) {
        const LocationDef *L = Data::loc(id);
        if (!L) return;
        QStringList body;
        body << locTypeName(L->type) + QStringLiteral(" · ") + biomeName(L->biome);
        body << QStringLiteral("Порог %1 · рекомендован ~%2 уровень").arg(L->tier).arg(L->reqLevel);
        if (m_game->isCleared(id)) body << QStringLiteral("Зачищено");
        else if (!m_game->isUnlocked(id)) body << QStringLiteral("Ещё не открыто");
        if (!L->rewardHint.isEmpty()) body << L->rewardHint;
        Tooltip::instance(window())->showText(L->name, body.join(QLatin1Char('\n')),
                                              mapToGlobal(QPoint(int(pos.x()), int(pos.y()))),
                                              Data::regionColor(L->region));
    } else Tooltip::instance(window())->hideNow();
}

void WorldMapPanel::onLeave() { m_hover = -1; Tooltip::instance(window())->hideNow(); }



// ============================================================
//  МАГАЗИН
// ============================================================
ShopPanel::ShopPanel(QWidget *parent)
    : OverlayPanel(parent), m_tab(0), m_hover(-1), m_scroll(0), m_tier(1)
{
}

void ShopPanel::setShopTier(int tier)
{
    m_tier = qBound(1, tier, 6);
    generateStock();
}

void ShopPanel::generateStock()
{
    m_stock.clear();
    m_prices.clear();
    if (!m_game) return;
    int ilvl = m_tier * 6;
    Rng r(quint64(ilvl) * 7717ull + 99ull);
    int n = 12 + r.pick(5);
    for (int i = 0; i < n; ++i) {
        float roll = r.unit();
        Item it;
        if (roll < 0.18f) it = m_game->rollPotion(ilvl);
        else {
            int forced = -1;
            if (r.chance(0.16f)) forced = Rar_Rare;
            if (r.chance(0.05f + m_tier * 0.012f)) forced = Rar_Legend;
            Slot sl = Slot(r.pick(int(Slot_EquipCount)));
            if (sl == Slot_Ring2) sl = Slot_Ring1;
            it = m_game->rollItem(ilvl + r.range(-1, 2), 0.f, forced, sl, true);
        }
        if (!it.valid) continue;
        int price = m_game->priceOf(it, false);
        price = int(float(price) * (1.0f + (m_tier - 1) * 0.04f));
        m_stock.append(it);
        m_prices.append(qMax(5, price));
    }
    // гарантированные расходники
    for (int i = 0; i < Data::bases().size(); ++i) {
        const ItemBaseDef &b = Data::bases()[i];
        if (b.bag == Bag_Potion || b.id_str == QLatin1String("m_tp") || b.id_str == QLatin1String("m_soul")) {
            Item it = m_game->makeItem(i, Rar_Normal, 1);
            it.count = 1;
            if (b.bag == Bag_Potion && b.tier > m_tier) continue;
            m_stock.append(it);
            m_prices.append(qMax(8, m_game->priceOf(it, false)));
        }
    }
}

void ShopPanel::refresh()
{
    if (m_stock.isEmpty()) generateStock();
    update();
}

void ShopPanel::paintPanel(QPainter *p, const QRectF &r)
{
    clearButtons();
    drawTitle(p, r, QStringLiteral("Лавка"),
              QStringLiteral("Золото: %1  ·  порог товара: %2").arg(m_game->gold()).arg(romanNum(m_tier)), Gl_Coins);
    addButton(QRectF(r.right() - 46, r.top() + 14, 32, 32), QString(), Gl_Cross, Act_Close);

    // вкладки
    float by = r.bottom() - 46;
    addButton(QRectF(r.left() + 16, by, 150, 32), QStringLiteral("Закрыть"), Gl_Cross, Act_Close);
    addButton(QRectF(r.left() + 176, by, 200, 32), QStringLiteral("Обновить товар"), Gl_Respec, Act_ShopRestock,
              m_game->gold() >= 20 + m_tier * 10);

    QRectF tabBuy(r.left() + 16, r.top() + 70, 190, 34);
    QRectF tabSell(r.left() + 212, r.top() + 70, 190, 34);
    int bbi = addButton(tabBuy, QStringLiteral("Купить"), Gl_Coins, Act_ShopTabBuy);
    if (bbi >= 0) m_buttons[bbi].accent = (m_tab == 0) ? C::gold : QColor();
    int bsi = addButton(tabSell, QStringLiteral("Продать"), Gl_Bag, Act_ShopTabSell);
    if (bsi >= 0) m_buttons[bsi].accent = (m_tab == 1) ? C::gold : QColor();

    QRectF list(r.left() + 16, r.top() + 114, r.width() - 32, by - (r.top() + 114) - 10);
    drawScrollArea(p, list);

    float rowH = 52.f;
    int visible = int(list.height() / rowH);
    m_hover = -1;
    p->save();
    p->setClipRect(list.adjusted(1, 1, -1, -1));

    if (m_tab == 0) {
        for (int i = 0; i < m_stock.size(); ++i) {
            int vi = i - m_scroll;
            if (vi < 0 || vi >= visible) continue;
            const Item &it = m_stock.at(i);
            const ItemBaseDef *b = Data::base(it.baseId);
            if (!b) continue;
            QRectF row(list.left() + 6, list.top() + 6 + vi * rowH, list.width() - 12, rowH - 6);
            bool hover = row.contains(m_mouse);
            if (hover) m_hover = i;
            int price = m_prices.at(i);
            bool afford = m_game->gold() >= price;
            p->fillRect(row, hover ? QColor(255, 220, 150, 26) : QColor(255, 255, 255, 8));
            p->setPen(QPen(hover ? C::amber : C::edge.darker(140), 1.2));
            p->setBrush(Qt::NoBrush);
            p->drawRect(row);
            QColor tc = rarityColor(it.rarity);
            QRectF ir(row.left() + 6, row.top() + 4, row.height() - 8, row.height() - 8);
            p->fillRect(ir, QColor(10, 8, 16, 200));
            p->setPen(QPen(tc, 1.4));
            p->drawRect(ir);
            Style::glow(p, ir.center(), 24.f, tc, it.rarity >= Rar_Rare ? 0.35f : 0.12f);
            drawGlyph(p, b->glyph, ir.adjusted(4, 4, -4, -4), tc.lighter(115), tc, 0.2f);

            float tx = ir.right() + 12;
            Style::textShadow(p, QRectF(tx, row.top() + 4, row.width() - (tx - row.left()) - 150, 18),
                              Data::itemNameFull(it), afford ? tc.lighter(120) : C::ash, fontHead(13),
                              Qt::AlignLeft | Qt::AlignVCenter);
            QString sub = b->bag == Bag_Equip ? QStringLiteral("%1 · порог %2 · уровень %3")
                              .arg(slotName(b->slot)).arg(romanNum(b->tier)).arg(b->reqLevel)
                          : QStringLiteral("расходник");
            Style::textShadow(p, QRectF(tx, row.top() + 22, row.width() - (tx - row.left()) - 150, 15),
                              sub, C::boneDim, fontSmall(10), Qt::AlignLeft | Qt::AlignVCenter);
            if (b->bag == Bag_Equip) {
                QString mods = Data::describeMods(m_game->itemMods(it)).split(QLatin1Char('\n')).value(0);
                Style::textShadow(p, QRectF(tx, row.top() + 35, row.width() - (tx - row.left()) - 150, 13),
                                  mods, C::boneDim.darker(110), fontSmall(9), Qt::AlignLeft | Qt::AlignVCenter);
            }
            QRectF pr(row.right() - 138, row.top() + 8, 128, row.height() - 16);
            Style::panel(p, pr, 1, 0.9f);
            drawGlyph(p, Gl_Coin, QRectF(pr.left() + 6, pr.center().y() - 9, 18, 18), C::gold);
            Style::textShadow(p, QRectF(pr.left() + 26, pr.top(), pr.width() - 32, pr.height()),
                              QString::number(price), afford ? C::gold : C::bad, fontNum(14, true),
                              Qt::AlignLeft | Qt::AlignVCenter);
        }
        if (m_stock.isEmpty())
            Style::textShadow(p, list, QStringLiteral("Товар закончился - обнови запас"), C::boneDim, fontHead(14),
                              Qt::AlignCenter);
    } else {
        const QVector<Item> &bag = m_game->bag();
        int shown = 0;
        for (int i = 0; i < bag.size(); ++i) {
            if (!bag.at(i).valid) continue;
            int vi = shown - m_scroll;
            ++shown;
            if (vi < 0 || vi >= visible) continue;
            const Item &it = bag.at(i);
            const ItemBaseDef *b = Data::base(it.baseId);
            if (!b) continue;
            QRectF row(list.left() + 6, list.top() + 6 + vi * rowH, list.width() - 12, rowH - 6);
            bool hover = row.contains(m_mouse);
            if (hover) m_hover = 1000 + i;
            int price = m_game->priceOf(it, true);
            p->fillRect(row, hover ? QColor(255, 220, 150, 26) : QColor(255, 255, 255, 8));
            p->setPen(QPen(hover ? C::amber : C::edge.darker(140), 1.2));
            p->setBrush(Qt::NoBrush);
            p->drawRect(row);
            QColor tc = rarityColor(it.rarity);
            QRectF ir(row.left() + 6, row.top() + 4, row.height() - 8, row.height() - 8);
            p->fillRect(ir, QColor(10, 8, 16, 200));
            p->setPen(QPen(tc, 1.4));
            p->drawRect(ir);
            drawGlyph(p, b->glyph, ir.adjusted(4, 4, -4, -4), tc.lighter(115), tc, 0.2f);
            float tx = ir.right() + 12;
            Style::textShadow(p, QRectF(tx, row.top() + 4, row.width() - (tx - row.left()) - 150, 18),
                              Data::itemNameFull(it), tc.lighter(110), fontHead(13), Qt::AlignLeft | Qt::AlignVCenter);
            Style::textShadow(p, QRectF(tx, row.top() + 22, row.width() - (tx - row.left()) - 150, 15),
                              QStringLiteral("сила предмета: %1").arg(m_game->itemScore(it)), C::boneDim, fontSmall(10),
                              Qt::AlignLeft | Qt::AlignVCenter);
            QRectF pr(row.right() - 138, row.top() + 8, 128, row.height() - 16);
            Style::panel(p, pr, 1, 0.9f);
            drawGlyph(p, Gl_Coin, QRectF(pr.left() + 6, pr.center().y() - 9, 18, 18), C::gold);
            Style::textShadow(p, QRectF(pr.left() + 26, pr.top(), pr.width() - 32, pr.height()),
                              QString::number(price), C::gold, fontNum(14, true), Qt::AlignLeft | Qt::AlignVCenter);
        }
        if (shown == 0)
            Style::textShadow(p, list, QStringLiteral("В сумке пусто"), C::boneDim, fontHead(14), Qt::AlignCenter);
    }
    p->restore();

    // полоса прокрутки
    int total = (m_tab == 0) ? m_stock.size() : m_game->bag().size();
    if (total > visible) {
        QRectF sb(list.right() - 8, list.top() + 4, 5, list.height() - 8);
        p->fillRect(sb, QColor(0, 0, 0, 120));
        float k = float(visible) / float(total);
        float hh = sb.height() * k;
        float yy = sb.top() + (sb.height() - hh) * (float(m_scroll) / float(qMax(1, total - visible)));
        p->fillRect(QRectF(sb.left(), yy, sb.width(), hh), C::edgeHi.darker(120));
    }
    Style::textShadow(p, QRectF(r.left() + 400, by, r.width() - 600, 32),
                      QStringLiteral("Клик по строке - %1 · колесо - прокрутка").arg(m_tab == 0 ? QStringLiteral("купить") : QStringLiteral("продать")),
                      C::boneDim, fontSmall(10), Qt::AlignLeft | Qt::AlignVCenter);
}

void ShopPanel::onClick(const QPointF &pos, Qt::MouseButton button)
{
    Q_UNUSED(pos);
    if (button != Qt::LeftButton) return;
    if (m_hover >= 0 && m_hover < 1000 && m_tab == 0) {
        if (m_hover >= m_stock.size()) return;
        int price = m_prices.at(m_hover);
        if (m_game->gold() < price) { Audio::inst().play(Sfx_UiBad); return; }
        Item it = m_stock.at(m_hover);
        if (!m_game->addItem(it)) { Audio::inst().play(Sfx_UiBad); return; }
        m_game->addGold(-price);
        Audio::inst().play(Sfx_Gold);
        m_stock.removeAt(m_hover);
        m_prices.removeAt(m_hover);
        m_hover = -1;
        update();
        return;
    }
    if (m_hover >= 1000 && m_tab == 1) {
        int idx = m_hover - 1000;
        if (idx < m_game->bag().size() && m_game->bag().at(idx).valid) {
            m_game->sellItem(idx);
            Audio::inst().play(Sfx_Gold);
            m_hover = -1;
            update();
        }
    }
}

void ShopPanel::onMove(const QPointF &pos)
{
    QRectF list(panelRect().left() + 16, panelRect().top() + 114, panelRect().width() - 32,
                panelRect().bottom() - 46 - (panelRect().top() + 114) - 10);
    Q_UNUSED(list);
    if (m_tab == 0 && m_hover >= 0 && m_hover < m_stock.size()) {
        Tooltip::instance(window())->showItem(m_stock.at(m_hover), m_game,
                                              mapToGlobal(QPoint(int(pos.x()), int(pos.y()))), true, m_prices.at(m_hover));
    } else if (m_tab == 1 && m_hover >= 1000) {
        int idx = m_hover - 1000;
        if (idx < m_game->bag().size() && m_game->bag().at(idx).valid)
            Tooltip::instance(window())->showItem(m_game->bag().at(idx), m_game,
                                                  mapToGlobal(QPoint(int(pos.x()), int(pos.y()))), true,
                                                  m_game->priceOf(m_game->bag().at(idx), true));
    } else Tooltip::instance(window())->hideNow();
}

void ShopPanel::onLeave() { m_hover = -1; Tooltip::instance(window())->hideNow(); }

void ShopPanel::wheelEvent(QWheelEvent *e)
{
    int delta = e->angleDelta().y() > 0 ? -1 : 1;
    m_scroll = qMax(0, m_scroll + delta);
    update();
    e->accept();
}

// ============================================================
//  ПАУЗА
// ============================================================
PausePanel::PausePanel(QWidget *parent)
    : OverlayPanel(parent), m_sound(true), m_shake(true), m_fullscreen(false), m_page(0), m_capture(-1),
      m_volDrag(false), m_pixel(1), m_scan(false), m_gl(false), m_setTab(0), m_lightQ(2), m_shadowQ(1)
{
}

void PausePanel::refresh()
{
    m_sound = Audio::inst().enabled();
    update();
}

void PausePanel::paintPanel(QPainter *p, const QRectF &r)
{
    clearButtons();
    addButton(QRectF(r.right() - 46, r.top() + 14, 32, 32), QString(), Gl_Cross, Act_Close);

    if (m_page == 0) {
        drawTitle(p, r, QStringLiteral("Привал"), QStringLiteral("Игра на паузе. Мир подождёт."), Gl_Torch);
        float cw = qMin(420.f, float(r.width()) * 0.42f);
        float cx = r.left() + (r.width() - cw) * 0.5f;
        float y = r.top() + 96;
        float bh = 44, gap = 12;
        addButton(QRectF(cx, y, cw, bh), QStringLiteral("Продолжить"), Gl_Play, Act_PauseResume); y += bh + gap;
        addButton(QRectF(cx, y, cw, bh), QStringLiteral("Сохранить игру"), Gl_Save, Act_PauseSave); y += bh + gap;
        addButton(QRectF(cx, y, cw, bh), QStringLiteral("Загрузить игру"), Gl_Load, Act_PauseLoad,
                  Game::saveExists()); y += bh + gap;
        addButton(QRectF(cx, y, cw, bh), QStringLiteral("Настройки"), Gl_Settings, Act_PauseSettings); y += bh + gap;
        addButton(QRectF(cx, y, cw, bh), QStringLiteral("Управление и правила"), Gl_Info, Act_PauseHelp); y += bh + gap;
        addButton(QRectF(cx, y, cw, bh), QStringLiteral("Выйти в главное меню"), Gl_Quit, Act_PauseQuitMenu); y += bh + gap + 10;

        QRectF sum(cx, y, cw, qMax(0.f, float(r.bottom()) - 24.f - y));
        if (sum.height() > 60) {
            p->save();
            p->setBrush(QColor(12, 10, 20, 150));
            p->setPen(QPen(C::edge.darker(150), 1.2));
            p->drawRect(sum);
            p->setClipRect(sum.adjusted(2, 2, -2, -2));
            QStringList sl;
            sl << QStringLiteral("Герой: %1 · %2 уровень").arg(m_game->heroName()).arg(m_game->level());
            sl << QStringLiteral("Убийств: %1 · Смертей: %2").arg(m_game->kills()).arg(m_game->deaths());
            sl << QStringLiteral("Локаций зачищено: %1 из %2").arg(m_game->clearedSet().size()).arg(Data::locationCount());
            int mm = int(m_game->playTime()) / 60, ss = int(m_game->playTime()) % 60;
            sl << QStringLiteral("Время в пути: %1 мин %2 сек").arg(mm).arg(ss);
            float yy = sum.top() + 10;
            for (int i = 0; i < sl.size(); ++i) {
                Style::textShadow(p, QRectF(sum.left() + 14, yy, sum.width() - 28, 17), sl.at(i),
                                  i == 0 ? C::amber : C::boneDim, fontSmall(12), Qt::AlignLeft | Qt::AlignVCenter);
                yy += 19;
            }
            p->restore();
        }
        return;
    }

    // ================= страница настроек со вкладками =================
    drawTitle(p, r, QStringLiteral("Настройки"), QStringLiteral("Звук, видео и управление - по вкладкам"), Gl_Settings);

    float tw = 200, th = 38, tgap = 12;
    float total = tw * 3 + tgap * 2;
    float tx = r.left() + (r.width() - total) * 0.5f;
    float ty = r.top() + 80;
    const char *tn[3] = { "Звук", "Видео", "Управление" };
    for (int i = 0; i < 3; ++i) {
        QRectF tr(tx + i * (tw + tgap), ty, tw, th);
        int bi = addButton(tr, QString::fromUtf8(tn[i]), Gl_None, Act_PauseTabSound + i);
        if (bi >= 0) m_buttons[bi].active = (m_setTab == i);
    }
    QRectF ca(r.left() + 40, ty + th + 16, r.width() - 80, r.bottom() - 56 - (ty + th + 16));
    p->save();
    p->setBrush(QColor(12, 10, 20, 140));
    p->setPen(QPen(C::edge.darker(150), 1.2));
    p->drawRect(ca);
    p->restore();

    if (m_setTab == 0) {
        drawVolumeSlider(p, volSliderRect(r), Audio::inst().volume(), m_volDrag);
        float cw = qMin(420.f, float(ca.width()) * 0.6f);
        float cx = ca.left() + (ca.width() - cw) * 0.5f;
        float y = volSliderRect(r).bottom() + 34;
        int bi = addButton(QRectF(cx, y, cw, 40),
                           Audio::inst().enabled() ? QStringLiteral("Звук: включён") : QStringLiteral("Звук: выключен"),
                           Gl_Settings, Act_PauseSound);
        if (bi >= 0) m_buttons[bi].active = Audio::inst().enabled();
        y += 56;
        QStringList info;
        QString gm = Audio::inst().musicFile(0), mm2 = Audio::inst().musicFile(1), tm = Audio::inst().musicFile(2);
        info << QStringLiteral("Музыка в бою: %1").arg(gm.isEmpty() ? QStringLiteral("нет") : QFileInfo(gm).fileName());
        info << QStringLiteral("Музыка меню: %1").arg(mm2.isEmpty() ? QStringLiteral("нет") : QFileInfo(mm2).fileName());
        info << QStringLiteral("Музыка поселений: %1").arg(tm.isEmpty() ? QStringLiteral("нет") : QFileInfo(tm).fileName());
        info << QStringLiteral("WAV играется встроенным конвейером, mp3/ogg/flac - медиаплеером Qt.");
        info << QStringLiteral("Файлы кладутся в папку sounds/ или assets/sounds/ рядом с игрой.");
        for (int i = 0; i < info.size(); ++i) {
            Style::textShadow(p, QRectF(ca.left() + 24, y, ca.width() - 48, 18), info.at(i),
                              i < 2 ? C::bone : C::boneDim.darker(110), fontSmall(11), Qt::AlignLeft | Qt::AlignVCenter);
            y += 21;
        }
    } else if (m_setTab == 1) {
        struct VBtn { QString label; int glyph; int act; bool on; };
        static const char *lqn[3] = { "\xD0\xB2\xD1\x8B\xD0\xBA\xD0\xBB",           // выкл
                                      "\xD0\xBD\xD0\xB8\xD0\xB7\xD0\xBA\xD0\xB8\xD0\xB9", // низкий
                                      "\xD0\xB2\xD1\x8B\xD1\x81\xD0\xBE\xD0\xBA\xD0\xB8\xD0\xB9" }; // высокий
        static const char *sqn[2] = { "\xD0\xB2\xD1\x8B\xD0\xBA\xD0\xBB",           // выкл
                                      "\xD0\xB2\xD0\xBA\xD0\xBB" };                   // вкл
        VBtn vb[10];
        vb[0].label = m_shake ? QStringLiteral("Тряска экрана: да") : QStringLiteral("Тряска экрана: нет");
        vb[0].glyph = Gl_Settings; vb[0].act = Act_PauseShake; vb[0].on = m_shake;
        vb[1].label = m_pixel <= 1 ? QStringLiteral("Пиксель-арт: выкл")
                                   : QStringLiteral("Пиксель-арт: x%1").arg(m_pixel);
        vb[1].glyph = Gl_Crystal; vb[1].act = Act_VideoPixel; vb[1].on = m_pixel > 1;
        vb[2].label = m_scan ? QStringLiteral("CRT-сканлайны: да") : QStringLiteral("CRT-сканлайны: нет");
        vb[2].glyph = Gl_Torch; vb[2].act = Act_VideoScan; vb[2].on = m_scan;
        vb[3].label = m_gl ? QStringLiteral("GL-постобработка: вкл") : QStringLiteral("GL-постобработка: выкл");
        vb[3].glyph = Gl_Crystal; vb[3].act = Act_VideoGl; vb[3].on = m_gl;
        vb[4].label = SprBank::enabled() ? QStringLiteral("Спрайты и тайлы: вкл")
                                         : QStringLiteral("Спрайты и тайлы: выкл");
        vb[4].glyph = Gl_Star; vb[4].act = Act_VideoSprites; vb[4].on = SprBank::enabled();
        vb[5].label = m_fullscreen ? QStringLiteral("Во весь экран: да") : QStringLiteral("Во весь экран: нет");
        vb[5].glyph = Gl_Settings; vb[5].act = Act_PauseFullscreen; vb[5].on = m_fullscreen;
        vb[6].label = QStringLiteral("Свет: %1").arg(QString::fromUtf8(lqn[qBound(0, m_lightQ, 2)]));
        vb[6].glyph = Gl_Torch; vb[6].act = Act_VideoLight; vb[6].on = m_lightQ > 0;
        vb[7].label = QStringLiteral("Тени: %1").arg(QString::fromUtf8(sqn[qBound(0, m_shadowQ, 1)]));
        vb[7].glyph = Gl_Ward; vb[7].act = Act_VideoShadow; vb[7].on = m_shadowQ > 0;
        vb[8].label = m_autoQ ? QStringLiteral("Авто-качество по FPS: вкл") : QStringLiteral("Авто-качество по FPS: выкл");
        vb[8].glyph = Gl_Star; vb[8].act = Act_VideoAutoQ; vb[8].on = m_autoQ;
        vb[9].label = m_miniOn ? QStringLiteral("Миникарта: вкл") : QStringLiteral("Миникарта: выкл");
        vb[9].glyph = Gl_Map; vb[9].act = Act_VideoMinimap; vb[9].on = m_miniOn;
        float y;
        if (ca.width() >= 760) {
            // широкий экран: две колонки по пять кнопок, чтобы всё влезло
            float cw = qMin(430.f, (float(ca.width()) - 100.f) * 0.5f);
            float gapC = 36.f;
            float x0 = ca.left() + (ca.width() - (cw * 2 + gapC)) * 0.5f;
            float y0 = ca.top() + 18;
            for (int i = 0; i < 10; ++i) {
                int col = i / 5, row = i % 5;
                int bi = addButton(QRectF(x0 + col * (cw + gapC), y0 + row * 44, cw, 38),
                                   vb[i].label, vb[i].glyph, vb[i].act);
                if (bi >= 0) m_buttons[bi].active = vb[i].on;
            }
            y = y0 + 5 * 44 + 12;
        } else {
            float cw = qMin(460.f, float(ca.width()) * 0.66f);
            float cx = ca.left() + (ca.width() - cw) * 0.5f;
            float y0 = ca.top() + 14;
            for (int i = 0; i < 10; ++i) {
                int bi = addButton(QRectF(cx, y0 + i * 40, cw, 34), vb[i].label, vb[i].glyph, vb[i].act);
                if (bi >= 0) m_buttons[bi].active = vb[i].on;
            }
            y = y0 + 10 * 40 + 8;
        }
        QStringList info;
        info << QStringLiteral("Свет: высокое - мягкие ореолы факелов; низкое - грубее, но быстрее; выкл - без темноты.");
        info << QStringLiteral("Тени: вкл - мягкие контактные пятна под существами и предметами; выкл - нет.");
        info << QStringLiteral("Авто-облегчение эффектов: %1").arg(m_view && m_view->quality() == 0
                 ? QStringLiteral("сейчас облегчённое") : QStringLiteral("сейчас полное"));
        if (m_view) info << QStringLiteral("Текущая частота кадров: %1").arg(int(m_view->fps()));
        for (int i = 0; i < info.size(); ++i) {
            Style::textShadow(p, QRectF(ca.left() + 24, y, ca.width() - 48, 18), info.at(i),
                              C::boneDim, fontSmall(11), Qt::AlignLeft | Qt::AlignVCenter);
            y += 21;
        }
    } else {
        paintKeys(p, ca);
    }

    float bw = 200;
    addButton(QRectF(r.left() + (r.width() - bw) * 0.5f, r.bottom() - 46, bw, 34),
              QStringLiteral("Назад"), Gl_ArrowRight, Act_PauseBack);
}

void PausePanel::paintKeys(QPainter *p, const QRectF &r)
{
    m_keyRows.clear();
    Style::textShadow(p, QRectF(r.left(), r.top() + 10, r.width(), 22),
                      m_capture >= 0 ? QStringLiteral("Нажмите клавишу для действия…  (Esc - отмена)")
                                     : QStringLiteral("Клик по строке - переназначить клавишу"),
                      m_capture >= 0 ? C::amber : C::boneDim, fontHead(13), Qt::AlignCenter);
    float cw = qMin(470.f, (float(r.width()) - 60.f) * 0.5f);
    float gapC = 40.f;
    float totalW = cw * 2 + gapC;
    float cx0 = r.left() + (r.width() - totalW) * 0.5f;
    float y0 = r.top() + 44;
    float rh = 26.f;
    int perCol = (Binds::A_Count + 1) / 2;
    for (int act = 0; act < Binds::A_Count; ++act) {
        int col = act / perCol;
        int rowI = act % perCol;
        float cx = cx0 + col * (cw + gapC);
        float y = y0 + rowI * rh;
        QRectF row(cx, y, cw, rh - 3);
        m_keyRows.append(row);
        bool hov = row.contains(m_mouse);
        bool cap = (m_capture == act);
        p->save();
        p->fillRect(row, cap ? QColor(242, 181, 68, 46) : (hov ? QColor(255, 220, 150, 26) : QColor(255, 255, 255, 7)));
        p->setPen(QPen(cap ? C::amber : (hov ? C::edgeHi : C::edge.darker(140)), cap ? 1.8 : 1.0));
        p->setBrush(Qt::NoBrush);
        p->drawRect(row);
        Style::textShadow(p, QRectF(row.left() + 12, row.top(), row.width() * 0.58f, row.height()),
                          actTitle(act), hov || cap ? C::bone : C::boneDim, fontSmall(11),
                          Qt::AlignLeft | Qt::AlignVCenter);
        QColor kc = cap ? C::amber : (Binds::key(act) == Binds::defKey(act) ? C::boneDim : C::good);
        Style::textShadow(p, QRectF(row.left() + row.width() * 0.58f, row.top(), row.width() * 0.42f - 12, row.height()),
                          cap ? QStringLiteral("…") : Binds::keyName(Binds::key(act)), kc, fontSmall(11),
                          Qt::AlignRight | Qt::AlignVCenter);
        p->restore();
    }
    float by = r.bottom() - 40;
    addButton(QRectF(r.left() + (r.width() - 200) * 0.5f, by, 200, 32),
              QStringLiteral("Сбросить клавиши"), Gl_Respec, Act_PauseKeysReset);
}

QRectF PausePanel::volSliderRect(const QRectF &panel) const
{
    float cw = qMin(520.f, float(panel.width()) * 0.62f);
    float cx = panel.left() + (panel.width() - cw) * 0.5f;
    float ty = panel.top() + 80;
    float y = ty + 38 + 16 + 60;
    return QRectF(cx, y, cw, 30);
}

void PausePanel::mousePressEvent(QMouseEvent *e)
{
    if (m_page == 1 && m_setTab == 0 && e->button() == Qt::LeftButton
        && volSliderRect(panelRect()).adjusted(-8, -12, 8, 12).contains(e->localPos())) {
        m_volDrag = true;
        Audio::inst().setVolume(volumeFromX(volSliderRect(panelRect()), float(e->localPos().x())));
        Audio::inst().play(Sfx_Ui);
        update();
        e->accept();
        return;
    }
    OverlayPanel::mousePressEvent(e);
}

void PausePanel::mouseMoveEvent(QMouseEvent *e)
{
    if (m_volDrag) {
        Audio::inst().setVolume(volumeFromX(volSliderRect(panelRect()), float(e->localPos().x())));
        update();
        e->accept();
        return;
    }
    OverlayPanel::mouseMoveEvent(e);
}

void PausePanel::mouseReleaseEvent(QMouseEvent *e)
{
    if (m_volDrag) { m_volDrag = false; update(); }
    OverlayPanel::mouseReleaseEvent(e);
}

void PausePanel::keyPressEvent(QKeyEvent *e)
{
    if (m_capture >= 0) {
        if (e->key() == Qt::Key_Escape) m_capture = -1;
        else { Binds::setKey(m_capture, e->key()); m_capture = -1; Audio::inst().play(Sfx_Ui); }
        update();
        e->accept();
        return;
    }
    OverlayPanel::keyPressEvent(e);
}

void PausePanel::onClick(const QPointF &pos, Qt::MouseButton button)
{
    Q_UNUSED(button);
    if (m_page != 1) return;
    for (int i = 0; i < m_keyRows.size(); ++i) {
        if (m_keyRows.at(i).contains(pos)) {
            m_capture = i;
            Audio::inst().play(Sfx_Ui);
            update();
            return;
        }
    }
}

// ============================================================
//  СМЕРТЬ
// ============================================================
DeathPanel::DeathPanel(QWidget *parent) : OverlayPanel(parent) {}

void DeathPanel::refresh() { update(); }

void DeathPanel::paintPanel(QPainter *p, const QRectF &r)
{
    clearButtons();
    p->save();
    p->fillRect(r, QColor(20, 4, 6, 150));
    p->restore();
    QFont f = fontTitle(56, true);
    QRectF tr(r.left(), r.top() + r.height() * 0.16f, r.width(), 80);
    Style::textShadow(p, tr, QStringLiteral("ТЫ ПАЛ"), QColor(214, 60, 50), f, Qt::AlignCenter);
    Style::textShadow(p, QRectF(r.left(), tr.bottom() + 4, r.width(), 26),
                      QStringLiteral("Пепел принял тебя. Но не удержал."), C::boneDim, fontHead(16), Qt::AlignCenter);

    QRectF iconR(r.center().x() - 46, tr.bottom() + 44, 92, 92);
    Style::glow(p, iconR.center(), 90.f, C::blood, 0.5f);
    drawGlyph(p, Gl_Skull, iconR, QColor(226, 214, 196), C::blood, 0.5f);

    float y = iconR.bottom() + 26;
    const bool doom = m_game->doomPermadeath();
    QStringList sl;
    sl << QStringLiteral("Уровень %1 · убийств %2 · смертей %3").arg(m_game->level()).arg(m_game->kills()).arg(m_game->deaths());
    if (doom)
        sl << QStringLiteral("ГОРНИЛО ПУСТОТЫ: одна жизнь. Сохранение сгорело вместе с тобой.");
    else
        sl << QStringLiteral("Потеряно 10% золота. Возрождение: %1")
              .arg(Data::loc(m_game->lastTown()) ? Data::loc(m_game->lastTown())->name : QStringLiteral("Лагерь «Искра»"));
    for (int i = 0; i < sl.size(); ++i) {
        Style::textShadow(p, QRectF(r.left(), y, r.width(), 20), sl.at(i),
                          doom ? QColor(220, 90, 80) : C::boneDim, fontSmall(13), Qt::AlignCenter);
        y += 22;
    }
    float cw = 300, bh = 46;
    addButton(QRectF(r.center().x() - cw * 0.5f, y + 14, cw, bh),
              doom ? QStringLiteral("Принять гибель") : QStringLiteral("Восстать из пепла"), Gl_Play,
              Act_DeathRespawn);
    addButton(QRectF(r.center().x() - cw * 0.5f, y + 14 + bh + 12, cw, bh - 8),
              QStringLiteral("В главное меню"), Gl_Quit, Act_DeathQuit);
}

void DeathPanel::onClick(const QPointF &pos, Qt::MouseButton button) { Q_UNUSED(pos); Q_UNUSED(button); }

// ============================================================
//  ЖУРНАЛ
// ============================================================
LogPanel::LogPanel(QWidget *parent) : OverlayPanel(parent), m_scroll(0) {}

void LogPanel::addLine(const QString &text, const QColor &c)
{
    L l; l.text = text; l.c = c;
    m_lines.append(l);
    while (m_lines.size() > 400) m_lines.removeFirst();
    update();
}

void LogPanel::paintPanel(QPainter *p, const QRectF &r)
{
    clearButtons();
    drawTitle(p, r, QStringLiteral("Журнал странствий"), QStringLiteral("Всё, что помнит пепел."), Gl_Scroll);
    addButton(QRectF(r.right() - 46, r.top() + 14, 32, 32), QString(), Gl_Cross, Act_Close);
    QRectF list(r.left() + 18, r.top() + 72, r.width() - 36, r.height() - 72 - 56);
    drawScrollArea(p, list);
    p->save();
    p->setClipRect(list.adjusted(2, 2, -2, -2));
    float y = list.bottom() - 8 - float(m_scroll) * 18.f;
    QFont f = fontSmall(12);
    for (int i = m_lines.size() - 1; i >= 0; --i) {
        if (y < list.top() - 20) break;
        if (y > list.top() && y < list.bottom())
            Style::textShadow(p, QRectF(list.left() + 10, y - 16, list.width() - 20, 17), m_lines.at(i).text,
                              m_lines.at(i).c, f, Qt::AlignLeft | Qt::AlignVCenter);
        y -= 18.f;
    }
    p->restore();
    if (m_lines.isEmpty())
        Style::textShadow(p, list, QStringLiteral("Пока пусто"), C::boneDim, fontHead(14), Qt::AlignCenter);
    float by = r.bottom() - 46;
    addButton(QRectF(r.left() + 18, by, 150, 32), QStringLiteral("Закрыть"), Gl_Cross, Act_Close);
    addButton(QRectF(r.left() + 176, by, 170, 32), QStringLiteral("Очистить"), Gl_Cross, Act_LogClear);
}

void LogPanel::onClick(const QPointF &pos, Qt::MouseButton button) { Q_UNUSED(pos); Q_UNUSED(button); }

void LogPanel::wheelEvent(QWheelEvent *e)
{
    m_scroll = qMax(0, m_scroll + (e->angleDelta().y() > 0 ? -2 : 2));
    update();
    e->accept();
}

// ============================================================
//  СПРАВКА
// ============================================================
HelpPanel::HelpPanel(QWidget *parent) : OverlayPanel(parent), m_page(0) {}

void HelpPanel::paintPanel(QPainter *p, const QRectF &r)
{
    clearButtons();
    drawTitle(p, r, QStringLiteral("Как выживать в Пепле"),
              QStringLiteral("Страница %1 из 4").arg(m_page + 1), Gl_Info);
    addButton(QRectF(r.right() - 46, r.top() + 14, 32, 32), QString(), Gl_Cross, Act_Close);

    QRectF body(r.left() + 22, r.top() + 74, r.width() - 44, r.height() - 74 - 60);
    p->save();
    p->setBrush(QColor(12, 10, 20, 150));
    p->setPen(QPen(C::edge.darker(150), 1.2));
    p->drawRect(body);
    p->restore();

    struct Sec { QString head; QStringList lines; };
    QVector<QVector<Sec> > pages;

    QVector<Sec> pg0;
    pg0.append(Sec{ QStringLiteral("Управление"), QStringList()
        << QStringLiteral("W A S D / стрелки - движение")
        << QStringLiteral("Мышь - прицел, ЛКМ - атака оружием")
        << QStringLiteral("Колесо мыши - приблизить / отдалить камеру")
        << QStringLiteral("ПКМ - кувырок (короткая неуязвимость)")
        << QStringLiteral("1 … 6 - умения с панели")
        << QStringLiteral("Пробел - выпить зелье исцеления")
        << QStringLiteral("F или E - взаимодействие (портал, NPC, алтарь)")
        << QStringLiteral("Q - свиток возвращения, G - осколок души (сброс навыков)") });
    pg0.append(Sec{ QStringLiteral("Панели"), QStringList()
        << QStringLiteral("I - снаряжение и сумка")
        << QStringLiteral("K - паутина навыков")
        << QStringLiteral("M - карта мира")
        << QStringLiteral("L - журнал, J - достижения, H - эта справка")
        << QStringLiteral("Esc - привал (пауза, сохранение, настройки)")
        << QStringLiteral("F11 - полный экран / оконный режим") });
    pages.append(pg0);

    QVector<Sec> pg1;
    pg1.append(Sec{ QStringLiteral("Бой"), QStringList()
        << QStringLiteral("Урон оружия зависит от мощи (физический) или духа (магия).")
        << QStringLiteral("Броня снижает физический урон, сопротивления - стихийный (предел 75%).")
        << QStringLiteral("Критический удар наносит в полтора раза больше, а с узлами - ещё больше.")
        << QStringLiteral("Кувырок даёт неуязвимость - учись проходить сквозь удары.")
        << QStringLiteral("Статусы: горение и яд наносят урон со временем, холод замедляет,")
        << QStringLiteral("молния повышает получаемый урон, метка смерти усиливает всё на 35%.") });
    pg1.append(Sec{ QStringLiteral("Элитные враги"), QStringList()
        << QStringLiteral("Пылающий - оставляет огонь. Ледяной - замедляет. Грозовой - бьёт молнией.")
        << QStringLiteral("Вампирский - лечится от ударов. Бронированный - вдвое крепче.")
        << QStringLiteral("Бешеный - быстрее и злее. Рой - при смерти выпускает отпрысков.")
        << QStringLiteral("Отражающий - возвращает часть урона. Смотри на ауру и адаптируйся.") });
    pages.append(pg1);

    QVector<Sec> pg2;
    pg2.append(Sec{ QStringLiteral("Паутина навыков"), QStringList()
        << QStringLiteral("Каждый уровень даёт очки: 2 обычно и 3 дополнительных каждый пятый.")
        << QStringLiteral("Узел можно изучить, только если взят соседний по нити.")
        << QStringLiteral("Круги паутины открываются уровнем: 1-й сразу, 2-й с 3-го, 3-й с 8-го,")
        << QStringLiteral("4-й с 14-го, ключевые камни - с 18-го.")
        << QStringLiteral("Активные умения - шестигранники. Изучил - ставь на панель 1…6.")
        << QStringLiteral("Ключевые камни меняют правила игры: «Перегрузка», «Жатва пепла»,")
        << QStringLiteral("«Танцующий с тенью», «Несокрушимость», «Раскалённая сталь»,")
        << QStringLiteral("«Сквозь время», «Кровь и пепел», «Костяная корона».") });
    pg2.append(Sec{ QStringLiteral("Переобучение"), QStringList()
        << QStringLiteral("ПКМ по узлу - забыть один ранг. Кузнец в поселении - забыть всё за золото.")
        << QStringLiteral("Осколок души (падает с боссов) сбрасывает паутину бесплатно.") });
    pages.append(pg2);

    QVector<Sec> pg3;
    pg3.append(Sec{ QStringLiteral("Снаряжение"), QStringList()
        << QStringLiteral("Редкость: обычный → магический → редкий → легендарный → уникальный.")
        << QStringLiteral("Чем выше порог локации, тем сильнее выпадают вещи.")
        << QStringLiteral("В сумке: фильтры (Все/Экип./Зелья/Прочее), сортировка по типу,")
        << QStringLiteral("редкости или уровню; Ctrl+ЛКМ - быстро надеть, Shift+ЛКМ - в свободный слот.")
        << QStringLiteral("Уникальные вещи имеют собственные имена и историю.") });
    pg3.append(Sec{ QStringLiteral("Мир"), QStringList()
        << QStringLiteral("33 локации в пяти регионах: от Пепельных равнин до Горнила пустоты.")
        << QStringLiteral("Зачистка открывает соседние пути на карте. Боссы охраняют переходы.")
        << QStringLiteral("В поселениях: торговец, кузнец (переобучение) и целительница.")
        << QStringLiteral("Святилища дают временное благословение - не пропускай их.")
        << QStringLiteral("Портал в конце локации возвращает к карте мира (клавиша F).") });
    pages.append(pg3);

    float y = body.top() + 14;
    const QVector<Sec> &pg = pages.at(qBound(0, m_page, pages.size() - 1));
    for (int i = 0; i < pg.size(); ++i) {
        Style::textShadow(p, QRectF(body.left() + 18, y, body.width() - 36, 24), pg.at(i).head, C::amber,
                          fontHead(16), Qt::AlignLeft | Qt::AlignVCenter);
        y += 28;
        const QStringList &ls = pg.at(i).lines;
        for (int k = 0; k < ls.size(); ++k) {
            Style::textShadow(p, QRectF(body.left() + 30, y, body.width() - 48, 19), ls.at(k), C::bone,
                              fontSmall(12), Qt::AlignLeft | Qt::AlignVCenter);
            y += 20;
        }
        y += 12;
    }

    float by = r.bottom() - 46;
    addButton(QRectF(r.left() + 22, by, 150, 32), QStringLiteral("Закрыть"), Gl_Cross, Act_Close);
    addButton(QRectF(r.right() - 22 - 320, by, 150, 32), QStringLiteral("Назад"), Gl_ArrowRight, Act_HelpPrev, m_page > 0);
    addButton(QRectF(r.right() - 22 - 160, by, 160, 32), QStringLiteral("Дальше"), Gl_ArrowRight, Act_HelpNext,
              m_page < 3);
}

void HelpPanel::onClick(const QPointF &pos, Qt::MouseButton button) { Q_UNUSED(pos); Q_UNUSED(button); }



// ============================================================
//  ГЛАВНОЕ МЕНЮ
// ============================================================
namespace {

struct ClassInfo {
    const char *name;
    const char *title;
    const char *desc;
    int glyph;
    QColor color;
    const char *stat1;
    const char *stat2;
    const char *skill;
};

static const ClassInfo g_classes[4] = {
    { "Воин", "Уголь",
      "Ты выкован из того же пепла, что сжёг этот мир. Держи строй - и не отступай.",
      Gl_Swords, QColor(226, 122, 74), "Много здоровья и брони", "Урон в ближнем бою",
      "Рассекающий удар" },
    { "Маг", "Искра",
      "Ты помнишь мир до пожара. Слово и жест - и пламя вспоминает вместе с тобой.",
      Gl_Flask, QColor(122, 156, 246), "Мана и магический урон", "Урон по площади",
      "Огненный шар" },
    { "Странник", "Ветер",
      "Ты не дерёшься честно. Ты просто оказываешься там, где тебя уже нет.",
      Gl_Bow, QColor(146, 216, 140), "Скорость и критический удар", "Бой на дистанции",
      "Град стрел" },
    { "Жрец пепла", "Прах",
      "Ты хоронил этот мир по частям. Смерть задолжала тебе - и отдаёт.",
      Gl_Skull, QColor(206, 150, 236), "Вампиризм и дух", "Выживание и прислужники",
      "Исцеление пеплом" }
};

} // anonymous

TitleScreen::TitleScreen(QWidget *parent)
    : QWidget(parent), m_screen(Scr_Main), m_hover(-1), m_class(0), m_diff(1), m_time(0.f), m_fade(1.f), m_volDrag(false)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    m_anim = new QTimer(this);
    m_anim->setInterval(33);
    connect(m_anim, SIGNAL(timeout()), this, SLOT(update()));
    m_anim->start();

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setText(QStringLiteral("Безымянный"));
    m_nameEdit->setMaxLength(18);
    m_nameEdit->setFont(fontHead(15));
    QPalette pal = m_nameEdit->palette();
    pal.setColor(QPalette::Base, QColor(14, 11, 22));
    pal.setColor(QPalette::Text, C::bone);
    pal.setColor(QPalette::Highlight, C::edgeHi.darker(140));
    pal.setColor(QPalette::HighlightedText, C::amber);
    m_nameEdit->setPalette(pal);
    m_nameEdit->setStyleSheet(QStringLiteral("border: 1px solid #4a3a26; padding: 4px 8px; background: #0e0b16;"));
    m_nameEdit->hide();
}

void TitleScreen::resetToMain()
{
    Audio::inst().initAssets();   // пользователь мог положить звуки уже после запуска
    m_screen = Scr_Main;
    m_fade = 1.f;
    m_hover = -1;
    if (m_nameEdit) m_nameEdit->hide();
    update();
}

void TitleScreen::mouseReleaseEvent(QMouseEvent *e)
{
    if (m_volDrag) { m_volDrag = false; update(); }
    e->accept();
}

void TitleScreen::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    buildBackground();
    QRectF nr = buttonRect(99);
    m_nameEdit->setGeometry(int(nr.left()), int(nr.top()), int(nr.width()), int(nr.height()));
}

void TitleScreen::buildBackground()
{
    m_bg = QImage();
}

int TitleScreen::buttonCount() const
{
    if (m_screen == Scr_Main) return Game::saveExists() ? 5 : 4;
    if (m_screen == Scr_Class) return 6;   // 4 класса + назад + начать
    return 2;                              // настройки: звук и назад
}

QString TitleScreen::buttonText(int i) const
{
    if (m_screen == Scr_Main) {
        bool hasSave = Game::saveExists();
        if (!hasSave) {
            switch (i) {
            case 0: return QStringLiteral("Новая игра");
            case 1: return QStringLiteral("Как играть");
            case 2: return QStringLiteral("Настройки");
            default: return QStringLiteral("Выйти");
            }
        }
        switch (i) {
        case 0: return QStringLiteral("Новая игра");
        case 1: return QStringLiteral("Продолжить путь");
        case 2: return QStringLiteral("Как играть");
        case 3: return QStringLiteral("Настройки");
        default: return QStringLiteral("Выйти");
        }
    }
    if (m_screen == Scr_Class) {
        if (i < 4) return QString::fromUtf8(g_classes[i].name);
        if (i == 4) return QStringLiteral("Назад");
        return QStringLiteral("Начать путь");
    }
    if (i == 0) return Audio::inst().enabled() ? QStringLiteral("Звук: включён") : QStringLiteral("Звук: выключен");
    return QStringLiteral("Назад");
}

int TitleScreen::buttonGlyph(int i) const
{
    if (m_screen == Scr_Main) {
        bool hasSave = Game::saveExists();
        if (!hasSave) return i == 0 ? Gl_Play : (i == 1 ? Gl_Info : (i == 2 ? Gl_Settings : Gl_Quit));
        return i == 0 ? Gl_Play : (i == 1 ? Gl_Load : (i == 2 ? Gl_Info : (i == 3 ? Gl_Settings : Gl_Quit)));
    }
    if (m_screen == Scr_Class) {
        if (i < 4) return g_classes[i].glyph;
        return i == 4 ? Gl_ArrowRight : Gl_Play;
    }
    return i == 0 ? Gl_Settings : Gl_ArrowRight;
}

QRectF TitleScreen::nameDiceRect() const
{
    QRectF nr = buttonRect(99);
    return QRectF(nr.right() + 10, nr.top() - 3, 40, 40);
}

// ---- выбор сложности на экране создания героя ----
QString TitleScreen::diffName(int i) const
{
    static const char *nm[] = { "Тлеющий уголёк", "Обычный пепел", "Янтарный жар", "Горнило Пустоты" };
    return QString::fromUtf8(nm[qBound(0, i, 3)]);
}

QString TitleScreen::diffDesc(int i) const
{
    switch (i) {
    case 0:  return QString::fromUtf8("Путешествие для отдыха.");
    case 1:  return QString::fromUtf8("Так задумано. Задуманный баланс.");
    case 2:  return QString::fromUtf8("Пепел любит смелых.");
    default: return QString::fromUtf8("Одна жизнь. Смерть стирает сохранение.");
    }
}

QStringList TitleScreen::diffBullets(int i) const
{
    QStringList l;
    switch (i) {
    case 0:
        l << QStringLiteral("Враги бьют на 25% слабее и получают +40% урона")
          << QStringLiteral("Опыт и золото x1.25");
        break;
    case 1:
        l << QStringLiteral("Урон, опыт и золото - как задумано")
          << QStringLiteral("Смерть отбрасывает к истокам без потери героя");
        break;
    case 2:
        l << QStringLiteral("Враги злее (+35% урона) и крепче (+20% здоровья)")
          << QStringLiteral("Опыт и золото x0.85");
        break;
    default:
        l << QStringLiteral("Враги смертоносны: +60% урона, +35% здоровья")
          << QStringLiteral("Опыт и золото x0.7 · смерть стирает сохранение");
        break;
    }
    return l;
}

// блок сложности: горизонтальная полоса над полем имени
QRectF TitleScreen::diffRowRect() const
{
    float w = qMin(660.f, float(width()) - 60.f);
    float x = (float(width()) - w) * 0.5f;
    float y = float(height()) - 216.f;   // поле имени на -118, ниже резерв под подпись
    return QRectF(x, y, w, 92.f);
}

QRectF TitleScreen::diffTabRect(int i) const
{
    QRectF row = diffRowRect();
    float gap = 8.f;
    float tw = (row.width() - gap * 3.f) / 4.f;
    QRectF r(row.left() + float(i) * (tw + gap), row.top() + 28.f, tw, 34.f);
    return r;
}

void TitleScreen::rollRandomName()
{
    static const char *first[] = {
        "Каэн", "Морвэн", "Ильвэ", "Тэнри", "Грим", "Ульв", "Рагна", "Эйра", "Драг", "Вель",
        "Орвин", "Тира", "Хальд", "Сигга", "Бран", "Ирма", "Келд", "Астра", "Варг", "Нима",
        "Эйрик", "Хельга", "Торвин", "Сольва", "Руна", "Кайл", "Мира", "Одд", "Фрейя", "Стур"
    };
    static const char *epith[] = {
        "Пепельный", "из Уголья", "Тихий", "Полынный", "Безымённый", "с Провала", "Янтарный",
        "Горелый", "из-под Инея", "Молчаливый", "Ржавый", "последний из дозора", "Теневой",
        "с Топей", "Негасимый", "из Раслома"
    };
    Rng r(quint64(QTime::currentTime().msecsSinceStartOfDay()) * 2654435761u + quint64(m_time * 1000.f) + 7u);
    QString nm = QString::fromUtf8(first[r.pick(30)]);
    if (r.chance(0.65f)) nm += QLatin1Char(' ') + QString::fromUtf8(epith[r.pick(16)]);
    m_nameEdit->setText(nm);
    Audio::inst().play(Sfx_Ui);
}

QRectF TitleScreen::volSliderRect() const
{
    float bw = 330.f;
    return QRectF((width() - bw) * 0.5f, height() * 0.50f - 62, bw, 26);
}

QRectF TitleScreen::buttonRect(int i) const
{
    if (i == 99) {
        // поле имени на экране выбора класса (справа оставлено место под кубик)
        float w = 300.f;
        return QRectF((float(width()) - w) * 0.5f - 26.f, float(height()) - 118.f, w, 34);
    }
    float bw = 330.f, bh = 48.f, gap = 10.f;
    if (m_screen == Scr_Main) {
        int n = buttonCount();
        float y0 = height() * 0.52f;
        return QRectF((width() - bw) * 0.5f, y0 + float(i) * (bh + gap), bw, bh);
    }
    if (m_screen == Scr_Class) {
        if (i < 4) {
            float cw = qMin(250.f, (float(width()) - 100.f) / 4.f);
            float totalW = cw * 4 + 12 * 3;
            float x0 = (float(width()) - totalW) * 0.5f;
            float top = float(height()) * 0.16f + 62.f;
            float bottom = float(height()) - 236.f;
            float cardH = qBound(150.f, bottom - top, 430.f);
            float cardTop = top + qMax(0.f, (bottom - top - cardH) * 0.5f);
            return QRectF(x0 + float(i) * (cw + 12), cardTop, cw, cardH);
        }
        float x0 = (float(width()) - 430.f) * 0.5f;
        if (i == 4) return QRectF(x0, float(height()) - 66.f, 205.f, 40.f);
        return QRectF(x0 + 225.f, float(height()) - 66.f, 205.f, 40.f);
    }
    float y0 = height() * 0.50f;
    return QRectF((width() - bw) * 0.5f, y0 + float(i) * (bh + gap), bw, bh);
}

void TitleScreen::paintEvent(QPaintEvent *)
{
    static bool rescanned = false;
    if (!rescanned) { rescanned = true; Audio::inst().initAssets(); }
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    m_time += 0.033f;

    // ---------- ФОН ----------
    QLinearGradient sky(0, 0, 0, height());
    sky.setColorAt(0.00, QColor(10, 8, 18));
    sky.setColorAt(0.34, QColor(38, 20, 34));
    sky.setColorAt(0.55, QColor(96, 44, 32));
    sky.setColorAt(0.66, QColor(176, 92, 40));
    sky.setColorAt(0.72, QColor(58, 34, 30));
    sky.setColorAt(1.00, QColor(12, 9, 14));
    p.fillRect(rect(), sky);

    // светило
    QPointF sun(width() * 0.72f, height() * 0.56f);
    Style::glow(&p, sun, height() * 0.42f, QColor(255, 150, 60), 0.55f);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 214, 150, 220));
    p.drawEllipse(sun, height() * 0.075f, height() * 0.075f);

    // дальние хребты
    Rng r(90210);
    for (int layer = 0; layer < 3; ++layer) {
        float baseY = height() * (0.62f + layer * 0.05f);
        float amp = height() * (0.13f - layer * 0.028f);
        QColor col = layer == 0 ? QColor(30, 20, 30, 235) : (layer == 1 ? QColor(20, 14, 22, 245) : QColor(12, 9, 15));
        QPainterPath path;
        path.moveTo(0, height());
        int seg = 22;
        for (int i = 0; i <= seg; ++i) {
            float x = float(i) / float(seg) * float(width());
            float y = baseY - qAbs(r.gauss()) * amp - amp * 0.35f * qSin(i * (0.7f + layer * 0.4f));
            path.lineTo(x, y);
        }
        path.lineTo(width(), height());
        path.closeSubpath();
        p.setBrush(col);
        p.drawPath(path);
    }
    // силуэты руин
    Rng r2(1337);
    p.setBrush(QColor(8, 6, 11));
    p.setPen(Qt::NoPen);
    for (int i = 0; i < 9; ++i) {
        float x = r2.unit() * float(width());
        float w = 14.f + r2.unit() * 30.f;
        float h = 40.f + r2.unit() * 130.f;
        float y = height() * 0.74f - r2.unit() * 20.f;
        p.drawRect(QRectF(x, y - h, w, h + 40));
        if (r2.chance(0.5f)) {
            QPainterPath tri;
            tri.moveTo(x - 4, y - h);
            tri.lineTo(x + w * 0.5f, y - h - 26.f);
            tri.lineTo(x + w + 4, y - h);
            tri.closeSubpath();
            p.drawPath(tri);
        }
    }
    // земля
    QLinearGradient gr(0, height() * 0.74f, 0, height());
    gr.setColorAt(0, QColor(24, 17, 18));
    gr.setColorAt(1, QColor(8, 6, 10));
    p.fillRect(QRectF(0, height() * 0.74f, width(), height() * 0.26f), gr);

    // угли в воздухе
    for (int i = 0; i < 90; ++i) {
        float seed = float(i) * 0.7f;
        float x = fmodf(seed * 137.3f + m_time * (12.f + float(i % 5) * 4.f), float(width()) + 60.f) - 30.f;
        float y = height() - fmodf(seed * 91.7f + m_time * (16.f + float(i % 7) * 5.f), float(height()) + 60.f);
        float sz = 1.0f + float(i % 4) * 0.7f;
        float a = 0.25f + 0.6f * qAbs(qSin(m_time * 1.6f + seed));
        QColor c(255, int(140 + (i % 60)), 60, int(200 * a));
        p.setBrush(c);
        p.drawEllipse(QPointF(x, y), sz, sz);
        if (i % 6 == 0) Style::glow(&p, QPointF(x, y), sz * 8.f, QColor(255, 140, 50), 0.18f * a);
    }
    Style::vignette(&p, QRectF(0, 0, width(), height()), 0.72f);

    // ---------- ЗАГОЛОВОК ----------
    if (m_screen == Scr_Main) {
        float ty = height() * 0.16f;
        QRectF tr(0, ty, width(), height() * 0.16f);
        QFont f = fontTitle(qBound(34, int(width() * 0.055f), 78), true);
        Style::glow(&p, QPointF(width() * 0.5f, tr.center().y()), width() * 0.34f, C::ember, 0.22f);
        // «выжженное» золото
        p.save();
        p.setFont(f);
        QFontMetrics fm(f);
        QRectF b = fm.boundingRect(tr.toRect(), Qt::AlignCenter, QStringLiteral("ПЕПЕЛ И ЯНТАРЬ"));
        QLinearGradient tg(b.topLeft(), b.bottomLeft());
        tg.setColorAt(0.0, QColor(255, 236, 190));
        tg.setColorAt(0.45, QColor(242, 181, 68));
        tg.setColorAt(0.55, QColor(196, 122, 34));
        tg.setColorAt(1.0, QColor(120, 62, 22));
        p.setPen(QColor(0, 0, 0, 200));
        p.drawText(b.translated(3, 4), Qt::AlignCenter, QStringLiteral("ПЕПЕЛ И ЯНТАРЬ"));
        p.setPen(QPen(QColor(60, 30, 10), 3.0));
        p.setBrush(QBrush(tg));
        QPainterPath tp;
        tp.addText(b.center().x() - fm.width(QStringLiteral("ПЕПЕЛ И ЯНТАРЬ")) * 0.5f,
                   b.center().y() + fm.ascent() * 0.42f, f, QStringLiteral("ПЕПЕЛ И ЯНТАРЬ"));
        p.drawPath(tp);
        p.restore();

        Style::divider(&p, b_bottom_placeholder() + 10, width() * 0.30f, width() * 0.70f, C::edgeHi.darker(130));
    } else if (m_screen == Scr_Class) {
        Style::textShadow(&p, QRectF(0, height() * 0.10f, width(), 46), QStringLiteral("Выбери, кем войти в пепел"),
                          C::amber, fontTitle(30, true), Qt::AlignCenter);
        Style::textShadow(&p, QRectF(0, height() * 0.10f + 46, width(), 24),
                          QStringLiteral("Класс задаёт стартовое снаряжение и первое умение. Дальше всё решает паутина."),
                          C::boneDim, fontSmall(12), Qt::AlignCenter);
    } else {
        Style::textShadow(&p, QRectF(0, height() * 0.24f, width(), 46), QStringLiteral("Настройки"),
                          C::amber, fontTitle(30, true), Qt::AlignCenter);
        drawVolumeSlider(&p, volSliderRect(), Audio::inst().volume(), m_volDrag);
    }

    // ---------- КНОПКИ ----------
    int n = buttonCount();
    if (m_hoverK.size() < 12) m_hoverK.resize(12);
    for (int i = 0; i < 12; ++i) {
        float tgt = (i < n && m_hover == i) ? 1.f : 0.f;
        m_hoverK[i] += (tgt - m_hoverK[i]) * 0.22f;
    }
    for (int i = 0; i < n; ++i) {
        QRectF br = buttonRect(i);
        if (m_screen == Scr_Class && i < 4) {
            // карточка класса
            bool sel = (m_class == i);
            bool hov = br.contains(m_mouse);
            QColor cc = g_classes[i].color;
            p.save();
            if (sel || hov) Style::glow(&p, br.center(), br.width() * 0.9f, cc, sel ? 0.45f : 0.22f);
            Style::panel(&p, br, 1, sel ? 1.0f : 0.86f);
            p.setPen(QPen(sel ? cc : (hov ? cc.darker(130) : C::edge.darker(150)), sel ? 2.6 : 1.4));
            p.setBrush(Qt::NoBrush);
            p.drawRect(br.adjusted(3, 3, -3, -3));
            QRectF gr2(br.center().x() - 30, br.top() + 18, 60, 60);
            drawGlyph(&p, g_classes[i].glyph, gr2, cc.lighter(120), cc, 0.5f);
            Style::textShadow(&p, QRectF(br.left() + 8, gr2.bottom() + 6, br.width() - 16, 22),
                              QString::fromUtf8(g_classes[i].name), sel ? cc.lighter(130) : C::bone, fontHead(16),
                              Qt::AlignCenter);
            Style::textShadow(&p, QRectF(br.left() + 8, gr2.bottom() + 26, br.width() - 16, 15),
                              QStringLiteral("«%1»").arg(QString::fromUtf8(g_classes[i].title)), C::boneDim,
                              fontSmall(10), Qt::AlignCenter);
            bool compact = br.height() < 250.f;
            QRectF dr(br.left() + 12, gr2.bottom() + 44, br.width() - 24,
                      br.bottom() - (compact ? 26.f : 62.f) - (gr2.bottom() + 44));
            p.save();
            p.setClipRect(dr);
            QString desc = QString::fromUtf8(g_classes[i].desc);
            int fs = 10;
            QFont f = fontSmall(fs);
            QStringList lines = wrapText(desc, f, dr.width());
            float lh = 14.f;
            int maxLines = qMax(1, int(dr.height() / lh));
            while (lines.size() > maxLines && fs > 8) {
                --fs;
                f = fontSmall(fs);
                lh = float(fs) + 4.f;
                maxLines = qMax(1, int(dr.height() / lh));
                lines = wrapText(desc, f, dr.width());
            }
            QFontMetrics fmC(f);
            for (int li = 0; li < lines.size() && li < maxLines; ++li) {
                QString t = lines.at(li);
                if (li == maxLines - 1 && lines.size() > maxLines)
                    t = fmC.elidedText(t, Qt::ElideRight, int(dr.width() - 4)) + QStringLiteral("…");
                Style::textShadow(&p, QRectF(dr.left(), dr.top() + li * lh, dr.width(), lh), t,
                                  C::boneDim, f, Qt::AlignLeft | Qt::AlignVCenter);
            }
            p.restore();
            if (compact) {
                Style::textShadow(&p, QRectF(br.left() + 10, br.bottom() - 18, br.width() - 20, 15),
                                  QStringLiteral("Умение: %1").arg(QString::fromUtf8(g_classes[i].skill)), C::amber,
                                  fontSmall(9), Qt::AlignLeft | Qt::AlignVCenter);
            } else {
                float ly = br.bottom() - 52;
                Style::textShadow(&p, QRectF(br.left() + 10, ly, br.width() - 20, 14),
                                  QString::fromUtf8(g_classes[i].stat1), cc.lighter(110), fontSmall(10), Qt::AlignLeft);
                Style::textShadow(&p, QRectF(br.left() + 10, ly + 15, br.width() - 20, 14),
                                  QString::fromUtf8(g_classes[i].stat2), cc.lighter(110), fontSmall(10), Qt::AlignLeft);
                Style::textShadow(&p, QRectF(br.left() + 10, ly + 32, br.width() - 20, 16),
                                  QStringLiteral("Умение: %1").arg(QString::fromUtf8(g_classes[i].skill)), C::amber,
                                  fontSmall(10), Qt::AlignLeft);
            }
            if (sel) {
                drawGlyph(&p, Gl_Check, QRectF(br.right() - 28, br.top() + 8, 20, 20), C::good);
            }
            p.restore();
            continue;
        }
        Style::button(&p, br, buttonText(i), br.contains(m_mouse), false, true, buttonGlyph(i), m_hoverK[i], false);
    }

    if (m_screen == Scr_Class) {
        QRectF nr = buttonRect(99);
        Style::panel(&p, nr.adjusted(-6, -6, 6, 6), 1, 0.9f);
        QRectF dr = nameDiceRect();
        bool dhov = dr.contains(m_mouse);
        Style::button(&p, dr, QString(), dhov, false, true, Gl_Star, dhov ? 1.f : 0.f, false);

        // --- выбор сложности: аккуратный блок над полем имени ---
        static const QColor dCol[4] = { QColor(90, 170, 110), C::bone, QColor(235, 150, 60), QColor(200, 60, 60) };
        QRectF row = diffRowRect();
        Style::panel(&p, row, 1, 0.85f);
        Style::textShadow(&p, QRectF(row.left() + 14, row.top() + 8, row.width() - 28, 16),
                          QStringLiteral("С Л О Ж Н О С Т Ь"), C::boneDim, fontSmall(9), Qt::AlignLeft);
        for (int i = 0; i < 4; ++i) {
            QRectF r = diffTabRect(i);
            bool sel = (m_diff == i);
            bool hov = r.contains(m_mouse);
            QColor cc = dCol[i];
            p.setPen(QPen(sel ? cc : (hov ? cc.darker(140) : C::edge.darker(140)), sel ? 2.0 : 1.2));
            p.setBrush(sel ? QColor(cc.red(), cc.green(), cc.blue(), 46)
                           : (hov ? QColor(255, 255, 255, 14) : QColor(10, 8, 16, 120)));
            p.drawRoundedRect(r, 6, 6);
            Style::textShadow(&p, r, diffName(i), sel ? cc.lighter(125) : (hov ? C::bone : C::boneDim),
                              fontSmall(10), Qt::AlignCenter);
        }
        // описание выбранной сложности
        QColor dc = dCol[qBound(0, m_diff, 3)];
        QStringList bl = diffBullets(m_diff);
        float ty = row.bottom() + 4.f;
        Style::textShadow(&p, QRectF(row.left(), ty, row.width(), 15),
                          QStringLiteral("%1 — %2").arg(diffName(m_diff), diffDesc(m_diff)),
                          dc.lighter(120), fontSmall(10), Qt::AlignCenter);
        Style::textShadow(&p, QRectF(row.left(), ty + 16.f, row.width(), 14),
                          bl.join(QStringLiteral("  ·  ")), C::boneDim, fontSmall(9), Qt::AlignCenter);

        if (height() >= 640)
            Style::textShadow(&p, QRectF(0, float(height()) * 0.16f + 26.f, width(), 22),
                              QStringLiteral("Выбери класс, назови себя и иди. Смерть здесь - часть пути."),
                              C::boneDim.darker(110), fontSmall(11), Qt::AlignCenter);
    }

    // плавный вход экрана
    if (m_fade > 0.01f) {
        m_fade = qMax(0.f, m_fade - 0.045f);
        p.fillRect(rect(), QColor(5, 4, 9, int(235 * m_fade)));
    }
    // версия
    Style::textShadow(&p, QRectF(8, height() - 22, 400, 16),
                      QStringLiteral("«Пепел и Янтарь» · Qt 5.9.1 · вся графика нарисована кодом"),
                      QColor(120, 110, 100, 170), fontSmall(9), Qt::AlignLeft);
    if (m_screen == Scr_Main && Game::saveExists())
        Style::textShadow(&p, QRectF(width() - 320, height() - 22, 312, 16),
                          QStringLiteral("Найдено сохранение"), C::good.darker(110), fontSmall(9), Qt::AlignRight);
}

float TitleScreen::b_bottom_placeholder() const
{
    return height() * 0.16f + height() * 0.16f + 6.f;
}

void TitleScreen::mouseMoveEvent(QMouseEvent *e)
{
    m_mouse = e->localPos();
    if (m_volDrag && m_screen == Scr_Settings) {
        Audio::inst().setVolume(volumeFromX(volSliderRect(), float(m_mouse.x())));
        update();
        e->accept();
        return;
    }
    int old = m_hover;
    m_hover = -1;
    for (int i = 0; i < buttonCount(); ++i)
        if (buttonRect(i).contains(m_mouse)) { m_hover = i; break; }
    if (m_hover != old) update();
    e->accept();
}

void TitleScreen::mousePressEvent(QMouseEvent *e)
{
    m_mouse = e->localPos();
    if (e->button() != Qt::LeftButton) { e->accept(); return; }
    if (m_screen == Scr_Settings && volSliderRect().adjusted(-8, -10, 8, 10).contains(m_mouse)) {
        m_volDrag = true;
        Audio::inst().setVolume(volumeFromX(volSliderRect(), float(m_mouse.x())));
        Audio::inst().play(Sfx_Ui);
        update();
        e->accept();
        return;
    }
    bool hasSave = Game::saveExists();

    if (m_screen == Scr_Main) {
        int idx = -1;
        for (int i = 0; i < buttonCount(); ++i)
            if (buttonRect(i).contains(m_mouse)) { idx = i; break; }
        if (idx < 0) { e->accept(); return; }
        Audio::inst().play(Sfx_Ui);
        if (hasSave) {
            if (idx == 0) { m_screen = Scr_Class; m_nameEdit->show(); m_nameEdit->raise(); m_fade = 1.f; }
            else if (idx == 1) emit continueGame();
            else if (idx == 2) emit showHelp();
            else if (idx == 3) m_screen = Scr_Settings;
            else emit quitGame();
        } else {
            if (idx == 0) { m_screen = Scr_Class; m_nameEdit->show(); m_nameEdit->raise(); m_fade = 1.f; }
            else if (idx == 1) emit showHelp();
            else if (idx == 2) m_screen = Scr_Settings;
            else emit quitGame();
        }
    } else if (m_screen == Scr_Class) {
        if (nameDiceRect().contains(m_mouse)) { rollRandomName(); update(); e->accept(); return; }
        for (int i = 0; i < 4; ++i) {
            QRectF r = diffTabRect(i);
            if (r.contains(m_mouse)) { m_diff = i; Audio::inst().play(Sfx_Ui); update(); e->accept(); return; }
        }
        int idx = -1;
        for (int i = 0; i < buttonCount(); ++i)
            if (buttonRect(i).contains(m_mouse)) { idx = i; break; }
        if (idx >= 0 && idx < 4) { m_class = idx; Audio::inst().play(Sfx_Ui); }
        else if (idx == 4) { m_screen = Scr_Main; m_nameEdit->hide(); Audio::inst().play(Sfx_Ui); m_fade = 1.f; }
        else if (idx == 5) {
            Audio::inst().play(Sfx_LevelUp);
            m_nameEdit->hide();
            emit startNewGame(m_nameEdit->text().trimmed(), m_class, m_diff);
        }
    } else {
        int idx = -1;
        for (int i = 0; i < buttonCount(); ++i)
            if (buttonRect(i).contains(m_mouse)) { idx = i; break; }
        if (idx == 0) { Audio::inst().setEnabled(!Audio::inst().enabled()); Audio::inst().play(Sfx_Ui); }
        else if (idx == 1) { m_screen = Scr_Main; Audio::inst().play(Sfx_Ui); m_fade = 1.f; }
    }
    update();
    e->accept();
}


// ============================================================
//  ДОСТИЖЕНИЯ
// ============================================================
AchPanel::AchPanel(QWidget *parent) : OverlayPanel(parent), m_scroll(0) {}

void AchPanel::refresh() { m_scroll = 0; update(); }

void AchPanel::wheelEvent(QWheelEvent *e)
{
    m_scroll = qMax(0, m_scroll + (e->angleDelta().y() > 0 ? -2 : 2));
    update();
    e->accept();
}

void AchPanel::paintPanel(QPainter *p, const QRectF &r)
{
    clearButtons();
    AchTracker &AC = AchTracker::inst();
    drawTitle(p, r, QStringLiteral("Достижения"),
              QStringLiteral("Открыто %1 из %2 · прогресс общий для всех героев").arg(AC.unlockedCount()).arg(Ach::count()),
              Gl_Crown2);
    addButton(QRectF(r.right() - 46, r.top() + 14, 32, 32), QString(), Gl_Cross, Act_Close);

    QRectF list(r.left() + 18, r.top() + 72, r.width() - 36, r.height() - 72 - 56);
    drawScrollArea(p, list);
    p->save();
    p->setClipRect(list.adjusted(2, 2, -2, -2));

    float colW = (list.width() - 24) * 0.5f;
    float rowH = 62.f;
    int perCol = qMax(1, int((list.height() - 8) / rowH));
    int total = Ach::count();
    int rowsTotal = (total + 1) / 2;
    int maxScroll = qMax(0, rowsTotal - perCol);
    m_scroll = qBound(0, m_scroll, maxScroll);

    for (int id = 0; id < total; ++id) {
        int col = id / perCol;
        int row = id % perCol;
        int vis = row - m_scroll;
        if (vis < 0 || vis >= perCol) continue;
        QRectF rowR(list.left() + 10 + col * (colW + 6), list.top() + 6 + vis * rowH, colW, rowH - 6);
        bool un = AC.unlocked(id);
        bool hov = rowR.contains(m_mouse);
        const Ach::Def &d = Ach::def(id);

        p->save();
        p->fillRect(rowR, hov ? QColor(255, 220, 150, 20) : QColor(255, 255, 255, un ? 12 : 5));
        p->setPen(QPen(un ? C::amber.darker(120) : C::edge.darker(160), un ? 1.6 : 1.0));
        p->setBrush(Qt::NoBrush);
        p->drawRect(rowR);
        QRectF ir(rowR.left() + 8, rowR.top() + 7, rowR.height() - 14, rowR.height() - 14);
        if (un) {
            Style::glow(p, ir.center(), ir.width() * 1.1f, C::amber, 0.45f);
            drawGlyph(p, d.glyph, ir, C::amber, C::gold, 0.4f);
        } else {
            p->setOpacity(0.42f);
            drawGlyph(p, d.glyph, ir, C::ash, C::ash, 0.f);
            p->setOpacity(1.0);
            drawGlyph(p, Gl_Lock, QRectF(ir.right() - 8, ir.top() - 4, 12, 12), QColor(150, 140, 130));
        }
        float tx = ir.right() + 10;
        Style::textShadow(p, QRectF(tx, rowR.top() + 5, rowR.right() - tx - 30, 17),
                          QString::fromUtf8(d.name), un ? C::amber : C::bone, fontHead(12),
                          Qt::AlignLeft | Qt::AlignVCenter);
        Style::textShadow(p, QRectF(tx, rowR.top() + 22, rowR.right() - tx - 8, 15),
                          QString::fromUtf8(d.desc), un ? C::boneDim : C::boneDim.darker(120), fontSmall(10),
                          Qt::AlignLeft | Qt::AlignVCenter);
        int cur = AC.progressCur(id), goal = AC.progressGoal(id);
        QRectF pb(tx, rowR.bottom() - 12, rowR.right() - tx - 10, 6);
        Style::bar(p, pb, goal > 0 ? clampf(float(cur) / float(goal), 0.f, 1.f) : (un ? 1.f : 0.f),
                   un ? C::amber : C::edgeHi.darker(130), C::edge.darker(150));
        if (goal > 1)
            Style::textShadow(p, QRectF(pb.right() - 74, pb.top() - 13, 74, 12),
                              QStringLiteral("%1/%2").arg(cur).arg(goal), un ? C::amber : C::boneDim, fontSmall(8),
                              Qt::AlignRight);
        if (un)
            drawGlyph(p, Gl_Check, QRectF(rowR.right() - 24, rowR.top() + 6, 16, 16), C::good);
        p->restore();
    }
    p->restore();

    if (rowsTotal > perCol) {
        QRectF sb(list.right() - 8, list.top() + 4, 5, list.height() - 8);
        p->fillRect(sb, QColor(0, 0, 0, 120));
        float hh = sb.height() * float(perCol) / float(rowsTotal);
        float yy = sb.top() + (sb.height() - hh) * (float(m_scroll) / float(qMax(1, rowsTotal - perCol)));
        p->fillRect(QRectF(sb.left(), yy, sb.width(), hh), C::edgeHi.darker(120));
    }

    float by = r.bottom() - 46;
    addButton(QRectF(r.left() + 18, by, 150, 32), QStringLiteral("Закрыть"), Gl_Cross, Act_Close);
    Style::textShadow(p, QRectF(r.left() + 180, by, r.width() - 380, 32),
                      QStringLiteral("Достижения сохраняются в профиле и не сбрасываются со смертью или новой игрой"),
                      C::boneDim.darker(110), fontSmall(10), Qt::AlignLeft | Qt::AlignVCenter);
}

void AchPanel::onClick(const QPointF &pos, Qt::MouseButton button) { Q_UNUSED(pos); Q_UNUSED(button); }

} // namespace aa
