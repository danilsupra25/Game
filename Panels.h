#ifndef AA_PANELS_H
#define AA_PANELS_H

#include <QWidget>
#include <QVector>
#include <QHash>
#include <QTimer>
#include <QPointF>
#include <QStringList>
#include "Core/Types.h"

class QLineEdit;

namespace aa {

class Game;
class GameView;
struct Item;

// горизонтальный регулятор громкости (меню и привал)
void drawVolumeSlider(QPainter *p, const QRectF &r, float vol, bool active);
float volumeFromX(const QRectF &r, float x);

// ============================================================
//  Базовая панель-оверлей (своя рисовальная «кнопочная» система)
// ============================================================
class OverlayPanel : public QWidget
{
    Q_OBJECT
public:
    explicit OverlayPanel(QWidget *parent = 0);

    void setGame(Game *g) { m_game = g; }
    void setView(GameView *v) { m_view = v; }
    void openPanel();
    virtual void refresh() {}
    QRectF debugButtonRect(int id) const;
    int debugButtonCount() const { return m_buttons.size(); }

signals:
    void closed();
    void actionTriggered(int id);

protected:
    struct Btn {
        QRectF r; QString text; int glyph; int id; bool enabled; bool hover; bool down;
        QColor accent;      // если задан - цветная обводка (выбранный фильтр/вкладка)
        float ht;           // сглаженный ховер 0..1
        bool active;        // «включено/выбрано»
        Btn() : glyph(0), id(-1), enabled(true), hover(false), down(false), ht(0.f), active(false) {}
    };
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void keyPressEvent(QKeyEvent *);
    void resizeEvent(QResizeEvent *);

    virtual void paintPanel(QPainter *p, const QRectF &r) { Q_UNUSED(p); Q_UNUSED(r); }
    virtual void onClick(const QPointF &pos, Qt::MouseButton button) { Q_UNUSED(pos); Q_UNUSED(button); }
    virtual void onMove(const QPointF &pos) { Q_UNUSED(pos); }
    virtual void onLeave() {}
    virtual QRectF panelRect() const;

    int addButton(const QRectF &r, const QString &text, int glyph, int id, bool enabled = true);
    void clearButtons() { m_buttons.clear(); }
    void paintButtons(QPainter *p);
    int buttonAt(const QPointF &p) const;

    void drawTitle(QPainter *p, const QRectF &r, const QString &title, const QString &subtitle, int glyph);
    void drawScrollArea(QPainter *p, const QRectF &r);

    Game *m_game;
    GameView *m_view;
    QVector<Btn> m_buttons;
    QPointF m_mouse;
    QHash<quint64, float> m_btnHt;   // сохранённая подсветка кнопок между кадрами
    float m_time;
    float m_fade;
    QVector<float> m_hoverK;
    float m_open;         // анимация появления
    QTimer *m_anim;
    bool m_draggingPanel;
    int m_pressedBtn;
};

// ============================================================
//  ИНВЕНТАРЬ + ПЕРСОНАЖ
// ============================================================
class InventoryPanel : public OverlayPanel
{
    Q_OBJECT
public:
    explicit InventoryPanel(QWidget *parent = 0);
    void refresh();
    void setFilter(int f) { m_filter = qBound(0, f, 3); update(); }
    int  filter() const { return m_filter; }

protected:
    void paintPanel(QPainter *p, const QRectF &r);
    void onClick(const QPointF &pos, Qt::MouseButton button);
    void onMove(const QPointF &pos);
    void onLeave();
    void mouseReleaseEvent(QMouseEvent *);
    void keyPressEvent(QKeyEvent *);

private:
    QRectF bagRect(const QRectF &panel) const;
    QRectF bagCell(const QRectF &panel, int idx) const;
    QRectF equipRect(const QRectF &panel, Slot s) const;
    int bagIndexAt(const QPointF &p) const;
    Slot equipSlotAt(const QPointF &p) const;
    void showItemTooltip(const Item &it, const QPoint &gp);
    void rebuildCellMap() const;   // видимая ячейка -> индекс в сумке (с учётом фильтра)
    void useOrEquipAt(int bi);      // ПКМ / Ctrl+ЛКМ: надеть, выпить, использовать

    int m_hoverBag;
    Slot m_hoverEquip;
    bool m_hoveringEquip;
    int m_dragFrom;        // <0 нет, 0..9 экипировка, 100+ сумка
    Item m_dragItem;
    QPointF m_dragPos;
    bool m_dragging;
    int m_tab;             // 0 характеристики, 1 справка
    int m_filter;          // 0 все, 1 экипировка, 2 зелья, 3 прочее
    mutable QVector<int> m_cellMap;
};

// ============================================================
//  ПАУТИНА НАВЫКОВ
// ============================================================
class SkillWebPanel : public OverlayPanel
{
    Q_OBJECT
public:
    explicit SkillWebPanel(QWidget *parent = 0);
    void refresh();
    void setFilter(int branch) { m_filterBranch = branch; update(); }
    void resetView() { m_zoom = 1.f; m_pan = QPointF(0, 0); update(); }

protected:
    void paintPanel(QPainter *p, const QRectF &r);
    void onClick(const QPointF &pos, Qt::MouseButton button);
    void onMove(const QPointF &pos);
    void onLeave();
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void wheelEvent(QWheelEvent *);

private:
    QPointF toWorld(const QPointF &screen, const QRectF &canvas) const;
    QPointF toScreen(const QPointF &world, const QRectF &canvas) const;
    QRectF canvasRect(const QRectF &panel) const;
    int nodeAt(const QPointF &screenPos, const QRectF &canvas) const;

    QPointF m_pan;
    float m_zoom;
    int m_hover;
    bool m_panning;
    QPointF m_panStart;
    QPointF m_panOrigin;
    float m_pulse;
    int m_filterBranch;
};

// ============================================================
//  КАРТА МИРА
// ============================================================
class WorldMapPanel : public OverlayPanel
{
    Q_OBJECT
public:
    explicit WorldMapPanel(QWidget *parent = 0);
    void refresh();
    int selected() const { return m_selected; }
    void setSelected(int id) { m_selected = id; update(); }

signals:
    void travelRequested(int locId);
    void enterRequested(int locId);

protected:
    void paintPanel(QPainter *p, const QRectF &r);
    void onClick(const QPointF &pos, Qt::MouseButton button);
    void onMove(const QPointF &pos);
    void onLeave();

private:
    QRectF mapRect(const QRectF &panel) const;
    QPointF locPos(int locId, const QRectF &mr) const;
    int locAt(const QPointF &p, const QRectF &mr) const;
    static int glyphForType(LocType t);

    int m_hover;
    int m_selected;
    float m_walk;
    QImage m_base;
    QSize m_baseSize;
};

// ============================================================
//  МАГАЗИН
// ============================================================
class ShopPanel : public OverlayPanel
{
    Q_OBJECT
public:
    explicit ShopPanel(QWidget *parent = 0);
    void refresh();
    void setShopTier(int tier);
    void setTab(int t) { m_tab = t; m_scroll = 0; m_hover = -1; update(); }
    void restock() { generateStock(); m_scroll = 0; m_hover = -1; update(); }

protected:
    void paintPanel(QPainter *p, const QRectF &r);
    void onClick(const QPointF &pos, Qt::MouseButton button);
    void onMove(const QPointF &pos);
    void onLeave();
    void wheelEvent(QWheelEvent *);

private:
    void generateStock();
    QVector<Item> m_stock;
    QVector<int> m_prices;
    int m_tab;          // 0 купить, 1 продать
    int m_hover;
    int m_scroll;
    int m_tier;
};

// ============================================================
//  ПАУЗА / НАСТРОЙКИ / СМЕРТЬ / ЛОГ / СПРАВКА
// ============================================================
class PausePanel : public OverlayPanel
{
    Q_OBJECT
public:
    explicit PausePanel(QWidget *parent = 0);
    void refresh();
    void setPage(int pg) { m_page = pg; m_capture = -1; update(); }
    void setSetTab(int t) { m_setTab = t; m_capture = -1; update(); }
    void setShake(bool v) { m_shake = v; }
    void setFullscreen(bool v) { m_fullscreen = v; }
    void setPixel(int v) { m_pixel = v; }
    void setScan(bool v) { m_scan = v; }
    void setGl(bool v) { m_gl = v; }
    void setLightQ(int v) { m_lightQ = v; }
    void setShadowQ(int v) { m_shadowQ = v; }
    bool glNow() const { return m_gl; }
    bool shakeOn() const { return m_shake; }
    bool fullOn() const { return m_fullscreen; }
    int pixelNow() const { return m_pixel; }
    bool scanNow() const { return m_scan; }
    int lightQNow() const { return m_lightQ; }
    int shadowQNow() const { return m_shadowQ; }

    void paintKeys(QPainter *p, const QRectF &r);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);

signals:
    void resumeRequested();
    void saveRequested();
    void loadRequested();
    void quitToMenuRequested();
    void toggleFullscreenRequested();
    void soundToggled(bool on);
    void shakeToggled(bool on);

protected:
    void paintPanel(QPainter *p, const QRectF &r);
    void onClick(const QPointF &pos, Qt::MouseButton button);

private:
    bool m_sound;
    bool m_shake;
    bool m_fullscreen;
    int m_page;
    int m_setTab;
    int m_capture;
    int m_pixel;
    bool m_scan;
    bool m_gl;
    int m_lightQ;
    int m_shadowQ;
    QVector<QRectF> m_keyRows;
    bool m_volDrag;
    QRectF volSliderRect(const QRectF &panel) const;

protected:
    void keyPressEvent(QKeyEvent *);
};

class DeathPanel : public OverlayPanel
{
    Q_OBJECT
public:
    explicit DeathPanel(QWidget *parent = 0);
    void refresh();

signals:
    void respawnRequested();
    void quitToMenuRequested();

protected:
    void paintPanel(QPainter *p, const QRectF &r);
    void onClick(const QPointF &pos, Qt::MouseButton button);
};

class LogPanel : public OverlayPanel
{
    Q_OBJECT
public:
    explicit LogPanel(QWidget *parent = 0);
    void addLine(const QString &text, const QColor &c);
    void clear() { m_lines.clear(); m_scroll = 0; update(); }

protected:
    void paintPanel(QPainter *p, const QRectF &r);
    void onClick(const QPointF &pos, Qt::MouseButton button);
    void wheelEvent(QWheelEvent *);

private:
    struct L { QString text; QColor c; };
    QVector<L> m_lines;
    int m_scroll;
};

class AchPanel : public OverlayPanel
{
    Q_OBJECT
public:
    explicit AchPanel(QWidget *parent = 0);
    void refresh();

protected:
    void paintPanel(QPainter *p, const QRectF &r);
    void onClick(const QPointF &pos, Qt::MouseButton button);
    void wheelEvent(QWheelEvent *);

private:
    int m_scroll;
};

class HelpPanel : public OverlayPanel
{
    Q_OBJECT
public:
    explicit HelpPanel(QWidget *parent = 0);
    void nextPage() { m_page = qBound(0, m_page + 1, 3); update(); }
    void prevPage() { m_page = qBound(0, m_page - 1, 3); update(); }

protected:
    void paintPanel(QPainter *p, const QRectF &r);
    void onClick(const QPointF &pos, Qt::MouseButton button);

private:
    int m_page;
};

// ============================================================
//  ГЛАВНОЕ МЕНЮ
// ============================================================
class TitleScreen : public QWidget
{
    Q_OBJECT
public:
    explicit TitleScreen(QWidget *parent = 0);
    void resetToMain();      // возврат в главное меню (сброс экрана выбора класса)

signals:
    void startNewGame(QString heroName, int classId);
    void continueGame();
    void quitGame();
    void showHelp();

protected:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void resizeEvent(QResizeEvent *);

private:
    enum Scr { Scr_Main = 0, Scr_Class, Scr_Settings };
    QRectF volSliderRect() const;
    int m_screen;
    int m_hover;
    int m_class;
    bool m_volDrag;
    QPointF m_mouse;
    float m_time;
    float m_fade;
    QVector<float> m_hoverK;
    QTimer *m_anim;
    QLineEdit *m_nameEdit;
    QImage m_bg;
    void buildBackground();
    float b_bottom_placeholder() const;
    QRectF nameDiceRect() const;
    void rollRandomName();
    QRectF buttonRect(int i) const;
    int buttonCount() const;
    QString buttonText(int i) const;
    int buttonGlyph(int i) const;
};

} // namespace aa

#endif // AA_PANELS_H
