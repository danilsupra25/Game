// Бенчмарк рендера: меряет среднее время кадра в тяжёлой сцене (offscreen).
#include <QApplication>
#include <QGridLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QElapsedTimer>
#include <QPainter>
#include <QImage>
#include <QDebug>

#include "Core/Game.h"
#include "Core/Data.h"
#include "Game/GameView.h"
#include "UI/Hud.h"

using namespace aa;

static void mouseMove(QWidget *v, const QPointF &p)
{
    QMouseEvent e(QEvent::MouseMove, p, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(v, &e);
}
static void clickHold(QWidget *v, const QPointF &p)
{
    QMouseEvent e1(QEvent::MouseButtonPress, p, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(v, &e1);
}

static QWidget *makeShell(Game *g, GameView **outView, Hud **outHud)
{
    QWidget *shell = new QWidget();
    QGridLayout *gl = new QGridLayout(shell);
    gl->setContentsMargins(0, 0, 0, 0);
    gl->setSpacing(0);
    GameView *v = new GameView(g, shell);
    gl->addWidget(v, 0, 0);
    Hud *h = new Hud(g, v, shell);
    gl->addWidget(h, 0, 0);
    *outView = v;
    *outHud = h;
    return shell;
}

static Game *makeHero(int level, int classId)
{
    Game *g = new Game();
    g->newGame(QStringLiteral("Бенч"), classId);
    if (level > 1) {
        float need = 0;
        for (int i = 1; i < level; ++i) need += Game::xpForLevel(i);
        g->addXp(need + 10.f);
    }
    for (int pass = 0; pass < 60; ++pass) {
        bool any = false;
        for (int i = 0; i < Data::nodes().size(); ++i) {
            const SkillNode &n = Data::nodes().at(i);
            bool want = (n.branch == classId) || n.kind == Nk_Origin || n.branch == -1;
            if (want && g->canTakeNode(i)) { g->takeNode(i); any = true; }
        }
        if (!any) break;
    }
    int ilvl = qMax(1, level + 2);
    Slot sl[10] = { Slot_Weapon, Slot_Offhand, Slot_Helm, Slot_Chest, Slot_Gloves,
                    Slot_Belt, Slot_Boots, Slot_Amulet, Slot_Ring1, Slot_Ring2 };
    for (int i = 0; i < 10; ++i) {
        for (int a = 0; a < 40; ++a) {
            Item it = g->rollItem(ilvl, 0.4f, -1, sl[i], true);
            if (!it.valid) continue;
            const ItemBaseDef *b = Data::base(it.baseId);
            if (b && b->reqLevel > g->level()) continue;
            int idx = -1;
            g->addItem(it, &idx);
            if (idx >= 0) g->equipFromBag(idx);
            if (g->equipped(sl[i]).valid) break;
        }
    }
    return g;
}

static double benchScene(int locId, int level, int classId, int ww, int hh, float zoom, int frames)
{
    Game *g = makeHero(level, classId);
    GameView *v; Hud *h;
    QWidget *shell = makeShell(g, &v, &h);
    v->setInteractive(true);
    v->setPaused(false);
    v->loadLocation(locId);
    for (int i = 0; i < 120; ++i) v->debugStep(1.f / 60.f);
    v->setZoom(zoom);
    // атака ближайшего врага удержанием ЛКМ - FX, частицы, снаряды
    QPointF tgt = v->debugNearestHostile();
    if (tgt.x() >= 0) {
        QPointF scr = (tgt - v->cameraTopLeft()) * v->zoom();
        mouseMove(v, scr);
        clickHold(v, scr);
    }
    for (int i = 0; i < 240; ++i) v->debugStep(1.f / 60.f);

    shell->resize(ww, hh);
    shell->show();
    QApplication::processEvents();

    QImage canvas(ww, hh, QImage::Format_ARGB32_Premultiplied);
    for (int i = 0; i < 20; ++i) {                    // прогрев кэшей
        QPainter p(&canvas);
        shell->render(&p);
    }

    QElapsedTimer t;
    t.start();
    for (int i = 0; i < frames; ++i) {
        v->debugStep(1.f / 60.f);
        QPainter p(&canvas);
        shell->render(&p);
    }
    double ms = double(t.nsecsElapsed()) / 1e6 / frames;
    qInfo("loc=%2d %dx%d zoom=%.2f actors=%d parts=%d projs=%d: %.2f ms/frame (%.1f FPS)",
          locId, ww, hh, zoom, v->debugActorCount(), v->particlesCount(), v->projectilesCount(),
          ms, 1000.0 / ms);
    delete shell;
    delete g;
    return ms;
}

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("bench"));
    app.setOrganizationName(QStringLiteral("AshAmber"));
    Data::ensureLoaded();

    double total = 0;
    total += benchScene(3, 12, 0, 1600, 900, 1.15f, 200);    // бой, воин
    total += benchScene(3, 12, 1, 1920, 1080, 1.15f, 200);   // бой, маг, фулл HD
    total += benchScene(14, 6, 2, 1600, 900, 1.0f, 150);     // город
    qInfo("ИТОГО средний кадр: %.2f ms", total / 3.0);
    return 0;
}
