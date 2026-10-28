// Рендер скриншотов игры в offscreen-режиме (для визуальной проверки).
#include <QApplication>
#include <QGridLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPixmap>
#include <QDir>
#include <QtMath>

#include "Core/Game.h"
#include "Core/Data.h"
#include "Core/Style.h"
#include "Game/GameView.h"
#include "UI/Hud.h"
#include "UI/Panels.h"
#include "UI/MainWindow.h"
#include "Game/SpriteBank.h"
#include "Game/TileBank.h"

using namespace aa;

static const int W = 1280, H = 760;
static QString g_outDir = QStringLiteral("../shots");

static void shotAt(QWidget *w, const QString &name, int ww, int hh)
{
    w->resize(ww, hh);
    w->show();
    QApplication::processEvents();
    for (int i = 0; i < 60; ++i) w->repaint();
    QPixmap pm = w->grab();
    pm.save(QStringLiteral("%1/%2.png").arg(g_outDir, name));
    qInfo("saved %s (%dx%d)", qPrintable(name), pm.width(), pm.height());
}

static void shot(QWidget *w, const QString &name)
{
    w->resize(W, H);
    w->show();
    QApplication::processEvents();
    for (int i = 0; i < 60; ++i) w->repaint();   // дождаться конца анимации появления
    QPixmap pm = w->grab();
    if (pm.isNull()) { qCritical("ПУСТАЯ картинка: %s", qPrintable(name)); return; }
    QString path = g_outDir + QLatin1Char('/') + name + QStringLiteral(".png");
    bool ok = pm.save(path);
    qInfo("saved %s (%dx%d) ok=%d", qPrintable(path), pm.width(), pm.height(), int(ok));
}

static void pressKey(QWidget *v, int key)
{
    QKeyEvent a(QEvent::KeyPress, key, Qt::NoModifier);
    QApplication::sendEvent(v, &a);
    QKeyEvent b(QEvent::KeyRelease, key, Qt::NoModifier);
    QApplication::sendEvent(v, &b);
}
static void mouseMove(QWidget *v, const QPointF &p)
{
    QMouseEvent e(QEvent::MouseMove, p, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(v, &e);
}
static void click(QWidget *v, const QPointF &p, Qt::MouseButton btn = Qt::LeftButton, bool hold = false)
{
    QMouseEvent e1(QEvent::MouseButtonPress, p, btn, btn, Qt::NoModifier);
    QApplication::sendEvent(v, &e1);
    if (!hold) {
        QMouseEvent e2(QEvent::MouseButtonRelease, p, btn, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(v, &e2);
    }
}

static void forceWeapon(Game *g, const char *idStr)
{
    int wid = -1;
    for (int i = 0; i < Data::bases().size(); ++i)
        if (Data::bases()[i].id_str == QLatin1String(idStr)) { wid = i; break; }
    if (wid < 0) return;
    Item it = g->makeItem(wid, Rar_Normal, qMax(1, g->level()));
    int idx = -1;
    g->addItem(it, &idx);
    if (idx >= 0) g->equipFromBag(idx);
}

static Game *makeHero(int level, int classId, const QString &name)
{
    Game *g = new Game();
    g->newGame(name, classId);
    if (level > 1) {
        float need = 0;
        for (int i = 1; i < level; ++i) need += Game::xpForLevel(i);
        g->addXp(need + 10.f);
    }
    // распределить очки по ветви класса
    for (int pass = 0; pass < 60; ++pass) {
        bool any = false;
        for (int i = 0; i < Data::nodes().size(); ++i) {
            const SkillNode &n = Data::nodes().at(i);
            bool want = (n.branch == classId) || n.kind == Nk_Origin || n.branch == -1;
            if (want && g->canTakeNode(i)) { g->takeNode(i); any = true; }
        }
        if (!any) break;
    }
    // экипировка по уровню
    int ilvl = qMax(1, level + 2);
    Slot sl[10] = { Slot_Weapon, Slot_Offhand, Slot_Helm, Slot_Chest, Slot_Gloves,
                    Slot_Belt, Slot_Boots, Slot_Amulet, Slot_Ring1, Slot_Ring2 };
    for (int i = 0; i < 10; ++i) {
        for (int a = 0; a < 40; ++a) {
            Item it = g->rollItem(ilvl, 0.4f, i == 0 ? Rar_Legend : -1, sl[i], true);
            if (!it.valid) continue;
            const ItemBaseDef *b = Data::base(it.baseId);
            if (b && b->reqLevel > g->level()) continue;
            int idx = -1;
            g->addItem(it, &idx);
            if (idx >= 0) g->equipFromBag(idx);
            if (g->equipped(sl[i]).valid) break;
        }
    }
    // несколько предметов и расходников в сумку (для скриншота инвентаря)
    for (int i = 0; i < 22; ++i) {
        Item it = g->rollItem(ilvl + i % 6, 0.8f);
        if (it.valid) g->addItem(it);
    }
    Item pot = g->makeItem(0, Rar_Normal, 1);
    for (int i = 0; i < Data::bases().size(); ++i) {
        const ItemBaseDef &b = Data::bases()[i];
        if (b.bag == Bag_Potion || b.bag == Bag_Misc) {
            Item it = g->makeItem(i, Rar_Normal, 1);
            it.count = 1 + (i % 4);
            g->addItem(it);
        }
    }
    Q_UNUSED(pot);
    g->addGold(4200 + level * 130);
    // панель умений
    QList<int> sk = g->learnedSkills();
    for (int i = 0; i < 6 && i < sk.size(); ++i) g->setHotbar(i, sk.at(i));
    g->addHp(g->maxHp());
    g->addMana(g->maxMana());
    return g;
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

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("shots"));
    app.setOrganizationName(QStringLiteral("AshAmber"));
    if (argc > 1) g_outDir = QString::fromLocal8Bit(argv[1]);
    QDir().mkpath(g_outDir);
    Data::ensureLoaded();

    // ---------- 1. титульный экран ----------
    {
        TitleScreen t;
        shot(&t, QStringLiteral("01_title"));
        click(&t, QPointF(W * 0.5f, H * 0.52f + 24));
        QApplication::processEvents();
        shot(&t, QStringLiteral("02_class"));
        shotAt(&t, QStringLiteral("20_class_small"), 1280, 520);
        shotAt(&t, QStringLiteral("21_class_tiny"), 1024, 460);
        // назад в меню и в настройки
        float x0 = (W - 430.f) * 0.5f;
        click(&t, QPointF(x0 + 102, H * 0.865f + 20));
        QApplication::processEvents();
        bool hasSave = Game::saveExists();
        int setIdx = hasSave ? 3 : 2;
        click(&t, QPointF(W * 0.5f, H * 0.52f + setIdx * 58 + 24));
        QApplication::processEvents();
        for (int i = 0; i < 40; ++i) t.repaint();
        shot(&t, QStringLiteral("16_settings"));
    }

    // ---------- 2. бой в пепельных равнинах ----------
    {
        Game *g = makeHero(9, 0, QStringLiteral("Каэн"));
        GameView *v; Hud *h;
        QWidget *shell = makeShell(g, &v, &h);
        v->setInteractive(true);
        v->setPaused(false);
        v->loadLocation(3);
        for (int i = 0; i < 90; ++i) v->debugStep(1.f / 60.f);
        QPointF tgt = v->debugNearestHostile();
        QPointF cam = v->cameraTopLeft();
        QPointF scr = (tgt.x() >= 0) ? (tgt - cam) : QPointF(900, 300);
        mouseMove(v, scr);
        click(v, scr, Qt::LeftButton, true);
        for (int i = 0; i < 26; ++i) v->debugStep(1.f / 60.f);
        g->addHp(-g->maxHp() * 0.35f);
        shell->resize(W, H);
        // журнал новой партии: строки о спрайт-листах и тайлсетах (как в главной игре)
        {
            QStringList lines = SprBank::reportLines();
            for (int i = 0; i < lines.size(); ++i) h->pushLog(lines.at(i), C::boneDim);
            lines = TileBank::reportLines();
            for (int i = 0; i < lines.size(); ++i) h->pushLog(lines.at(i), C::boneDim);
            h->pushLog(QStringLiteral("Новая игра начата. Класс: Воин"), C::amber);
            for (int i = 0; i < 2; ++i) v->debugStep(1.f / 60.f);
            shot(shell, QStringLiteral("25_journal"));
        }
        shot(shell, QStringLiteral("03_combat"));

        // ---------- 3. умение в полёте ----------
        pressKey(v, Qt::Key_2);
        for (int i = 0; i < 8; ++i) v->debugStep(1.f / 60.f);
        pressKey(v, Qt::Key_3);
        for (int i = 0; i < 6; ++i) v->debugStep(1.f / 60.f);
        shot(shell, QStringLiteral("04_skill"));

        // ---------- 3b. зум колесом ----------
        v->setZoom(1.55f);
        for (int i = 0; i < 20; ++i) v->debugStep(1.f / 60.f);
        shot(shell, QStringLiteral("18_zoom_in"));
        v->setZoom(0.7f);
        for (int i = 0; i < 20; ++i) v->debugStep(1.f / 60.f);
        shot(shell, QStringLiteral("19_zoom_out"));
        v->setZoom(1.0f);

        // ---------- 3c. пиксель-арт режим ----------
        v->setPixelMode(3);
        v->setScanlines(true);
        for (int i = 0; i < 10; ++i) v->debugStep(1.f / 60.f);
        shell->resize(W, H);
        shell->repaint();
        shot(shell, QStringLiteral("23_pixel"));
        v->setPixelMode(1);
        v->setScanlines(false);

        // ---------- 4. инвентарь ----------
        InventoryPanel *inv = new InventoryPanel(shell);
        inv->setGame(g); inv->setView(v);
        inv->resize(W, H);
        shot(inv, QStringLiteral("05_inventory"));
        inv->setFilter(1);   // фильтр «Экипировка» - новый интерфейс сумки
        shot(inv, QStringLiteral("34_inv_filter"));
        inv->setFilter(0);

        // ---------- 5. паутина ----------
        SkillWebPanel *web = new SkillWebPanel(shell);
        web->setGame(g); web->setView(v);
        web->resize(W, H);
        shot(web, QStringLiteral("06_skillweb"));

        // ---------- 6. магазин ----------
        ShopPanel *shop = new ShopPanel(shell);
        shop->setGame(g); shop->setView(v);
        shop->setShopTier(3);
        shop->resize(W, H);
        shot(shop, QStringLiteral("08_shop"));

        delete inv; delete web; delete shop;
        delete shell; delete g;
    }

    // ---------- 7. босс ----------
    {
        Game *g = makeHero(20, 1, QStringLiteral("Ильвэ"));
        GameView *v; Hud *h;
        QWidget *shell = makeShell(g, &v, &h);
        v->setInteractive(true);
        v->setPaused(false);
        v->loadLocation(14);
        int held = -1;
        v->setZoom(0.75f);
        for (int i = 0; i < 3000; ++i) {
            Actor *pl = v->player();
            QPointF tgt = v->debugNearestHostile();
            QPointF cam = v->cameraTopLeft();
            if (tgt.x() >= 0) mouseMove(v, tgt - cam);
            int want = -1;
            if (pl && tgt.x() >= 0) {
                float dx = float(tgt.x() - pl->pos.x()), dy = float(tgt.y() - pl->pos.y());
                float dd = qSqrt(dx * dx + dy * dy);
                if (dd > 240.f) want = (qAbs(dx) > qAbs(dy)) ? (dx > 0 ? Qt::Key_D : Qt::Key_A)
                                                             : (dy > 0 ? Qt::Key_S : Qt::Key_W);
            }
            if (want != held) {
                if (held > 0) { QKeyEvent kr(QEvent::KeyRelease, held, Qt::NoModifier); QApplication::sendEvent(v, &kr); }
                if (want > 0) { QKeyEvent kp(QEvent::KeyPress, want, Qt::NoModifier); QApplication::sendEvent(v, &kp); }
                held = want;
            }
            if (i % 3 == 0 && tgt.x() >= 0) click(v, tgt - cam, Qt::LeftButton);
            v->debugStep(1.f / 60.f);
            if (pl && tgt.x() >= 0) {
                float dd = qSqrt(float((tgt.x()-pl->pos.x())*(tgt.x()-pl->pos.x()) + (tgt.y()-pl->pos.y())*(tgt.y()-pl->pos.y())));
                if (dd < 430.f && i > 300) break;
            }
        }
        if (held > 0) { QKeyEvent kr(QEvent::KeyRelease, held, Qt::NoModifier); QApplication::sendEvent(v, &kr); }
        // немного сбить боссу здоровья для красивой полосы
        shell->resize(W, H);
        shot(shell, QStringLiteral("07_boss"));
        delete shell; delete g;
    }

    // ---------- 8. город ----------
    {
        Game *g = makeHero(16, 2, QStringLiteral("Тэнри"));
        GameView *v; Hud *h;
        QWidget *shell = makeShell(g, &v, &h);
        v->setInteractive(true);
        v->setPaused(false);
        v->loadLocation(8);
        for (int i = 0; i < 120; ++i) v->debugStep(1.f / 60.f);
        // подойти к площади NPC, чтобы все четверо попали в кадр
        v->setZoom(1.0f);
        int held = -1;
        for (int i = 0; i < 1600; ++i) {
            Actor *pl = v->player();
            QPointF c = v->debugNpcCenter();
            if (!pl || c.x() < 0.f) break;
            float dx = float(c.x() - pl->pos.x()), dy = float(c.y() - pl->pos.y());
            if (qSqrt(dx * dx + dy * dy) < 40.f) break;
            int want = (qAbs(dx) > qAbs(dy)) ? (dx > 0 ? Qt::Key_D : Qt::Key_A)
                                              : (dy > 0 ? Qt::Key_S : Qt::Key_W);
            if (want != held) {
                if (held > 0) { QKeyEvent kr(QEvent::KeyRelease, held, Qt::NoModifier); QApplication::sendEvent(v, &kr); }
                QKeyEvent kp(QEvent::KeyPress, want, Qt::NoModifier); QApplication::sendEvent(v, &kp);
                held = want;
            }
            v->debugStep(1.f / 60.f);
        }
        if (held > 0) { QKeyEvent kr(QEvent::KeyRelease, held, Qt::NoModifier); QApplication::sendEvent(v, &kr); }
        for (int i = 0; i < 40; ++i) v->debugStep(1.f / 60.f);
        shell->resize(W, H);
        shot(shell, QStringLiteral("09_town"));
        delete shell; delete g;
    }

    // ---------- 8b. мобы первой локации (l01 «Разбитая дорога») ----------
    {
        Game *g = makeHero(9, 0, QStringLiteral("Каэн"));
        GameView *v; Hud *h;
        QWidget *shell = makeShell(g, &v, &h);
        v->setInteractive(true);
        v->setPaused(false);
        v->loadLocation(1);
        for (int i = 0; i < 60; ++i) v->debugStep(1.f / 60.f);
        v->setZoom(1.35f);
        int held = -1;
        for (int i = 0; i < 1800; ++i) {
            Actor *pl = v->player();
            QPointF tgt = v->debugNearestHostile();
            QPointF cam = v->cameraTopLeft();
            if (tgt.x() >= 0) mouseMove(v, tgt - cam);
            int want = -1;
            float dd = 1e9f;
            if (pl && tgt.x() >= 0) {
                float dx = float(tgt.x() - pl->pos.x()), dy = float(tgt.y() - pl->pos.y());
                dd = qSqrt(dx * dx + dy * dy);
                if (dd > 110.f) want = (qAbs(dx) > qAbs(dy)) ? (dx > 0 ? Qt::Key_D : Qt::Key_A)
                                                             : (dy > 0 ? Qt::Key_S : Qt::Key_W);
            }
            if (want != held) {
                if (held > 0) { QKeyEvent kr(QEvent::KeyRelease, held, Qt::NoModifier); QApplication::sendEvent(v, &kr); }
                if (want > 0) { QKeyEvent kp(QEvent::KeyPress, want, Qt::NoModifier); QApplication::sendEvent(v, &kp); }
                held = want;
            }
            v->debugStep(1.f / 60.f);
            if (dd < 150.f && i > 240) break;
        }
        if (held > 0) { QKeyEvent kr(QEvent::KeyRelease, held, Qt::NoModifier); QApplication::sendEvent(v, &kr); }
        // пара ударов в кадре: видны кадры атак листа героя
        QPointF tgt = v->debugNearestHostile();
        QPointF cam = v->cameraTopLeft();
        if (tgt.x() >= 0) { mouseMove(v, tgt - cam); click(v, tgt - cam, Qt::LeftButton); }
        for (int i = 0; i < 8; ++i) v->debugStep(1.f / 60.f);
        shell->resize(W, H);
        shot(shell, QStringLiteral("26_mobs_l01"));
        delete shell; delete g;
    }

    // ---------- 8c. тайлсеты карт крупным планом ----------
    {
        Game *g = makeHero(9, 0, QStringLiteral("Каэн"));
        GameView *v; Hud *h;
        QWidget *shell = makeShell(g, &v, &h);
        v->setInteractive(true);
        v->setPaused(false);
        v->loadLocation(1);
        for (int i = 0; i < 200; ++i) v->debugStep(1.f / 60.f);   // баннер локации (3 c) уходит
        v->setZoom(1.9f);
        for (int i = 0; i < 4; ++i) v->debugStep(1.f / 60.f);
        shell->resize(W, H);
        shot(shell, QStringLiteral("27_tiles_l01"));
        delete shell; delete g;
    }
    {
        Game *g = makeHero(16, 2, QStringLiteral("Тэнри"));
        GameView *v; Hud *h;
        QWidget *shell = makeShell(g, &v, &h);
        v->setInteractive(true);
        v->setPaused(false);
        v->loadLocation(8);
        for (int i = 0; i < 200; ++i) v->debugStep(1.f / 60.f);   // баннер локации (3 c) уходит
        v->setZoom(1.9f);
        for (int i = 0; i < 4; ++i) v->debugStep(1.f / 60.f);
        shell->resize(W, H);
        shot(shell, QStringLiteral("28_tiles_town"));
        delete shell; delete g;
    }

    // ---------- 8d. эффекты атак крупным планом ----------
    {
        // воин: полумесяц взмаха в кадре
        Game *g = makeHero(9, 0, QStringLiteral("Каэн"));
        forceWeapon(g, "w_ashblade");   // огненный клинок: янтарный росчерк
        GameView *v; Hud *h;
        QWidget *shell = makeShell(g, &v, &h);
        v->setInteractive(true);
        v->setPaused(false);
        v->loadLocation(1);
        for (int i = 0; i < 200; ++i) v->debugStep(1.f / 60.f);
        v->setZoom(1.8f);
        QPointF tgt = v->debugNearestHostile();
        QPointF cam = v->cameraTopLeft();
        QPointF scr = (tgt.x() >= 0) ? (tgt - cam) * v->zoom() : QPointF(W * 0.5f, H * 0.4f);
        mouseMove(v, scr);
        click(v, scr, Qt::LeftButton);
        for (int i = 0; i < 7; ++i) v->debugStep(1.f / 60.f);   // середина взмаха
        shell->resize(W, H);
        shot(shell, QStringLiteral("29_fx_slash"));
        delete shell; delete g;
    }
    {
        // маг: волшебный снаряд и круг каста
        Game *g = makeHero(9, 1, QStringLiteral("Илла"));
        forceWeapon(g, "w_appstf");     // посох: базовая атака - волшебный снаряд
        GameView *v; Hud *h;
        QWidget *shell = makeShell(g, &v, &h);
        v->setInteractive(true);
        v->setPaused(false);
        v->loadLocation(1);
        for (int i = 0; i < 200; ++i) v->debugStep(1.f / 60.f);
        v->setZoom(1.8f);
        QPointF tgt = v->debugNearestHostile();
        QPointF cam = v->cameraTopLeft();
        QPointF scr = (tgt - cam) * v->zoom();
        mouseMove(v, scr);
        click(v, scr, Qt::LeftButton);
        for (int i = 0; i < 6; ++i) v->debugStep(1.f / 60.f);    // снаряд в полёте
        shell->resize(W, H);
        shot(shell, QStringLiteral("30_fx_mage"));
        delete shell; delete g;
    }
    {
        // лучник: стрелы с трейлом
        Game *g = makeHero(9, 2, QStringLiteral("Вейра"));
        forceWeapon(g, "w_windbow");    // лук: стрелы с трейлом
        GameView *v; Hud *h;
        QWidget *shell = makeShell(g, &v, &h);
        v->setInteractive(true);
        v->setPaused(false);
        v->loadLocation(1);
        for (int i = 0; i < 200; ++i) v->debugStep(1.f / 60.f);
        v->setZoom(1.8f);
        QPointF tgt = v->debugNearestHostile();
        QPointF cam = v->cameraTopLeft();
        QPointF scr = (tgt - cam) * v->zoom();
        mouseMove(v, scr);
        click(v, scr, Qt::LeftButton);
        for (int i = 0; i < 10; ++i) v->debugStep(1.f / 60.f);
        shell->resize(W, H);
        shot(shell, QStringLiteral("31_fx_archer"));
        delete shell; delete g;
    }
    {
        // мобы второго региона в спрайтах + их атаки
        Game *g = makeHero(14, 0, QStringLiteral("Каэн"));
        GameView *v; Hud *h;
        QWidget *shell = makeShell(g, &v, &h);
        v->setInteractive(true);
        v->setPaused(false);
        v->loadLocation(10);
        for (int i = 0; i < 200; ++i) v->debugStep(1.f / 60.f);
        v->setZoom(1.7f);
        for (int i = 0; i < 120; ++i) v->debugStep(1.f / 60.f);   // дать мобам поатаковать
        shell->resize(W, H);
        shot(shell, QStringLiteral("32_mobs_l10"));
        delete shell; delete g;
    }
    {
        // босс в спрайтах крупным планом + его приёмы
        Game *g = makeHero(20, 0, QStringLiteral("Каэн"));
        GameView *v; Hud *h;
        QWidget *shell = makeShell(g, &v, &h);
        v->setInteractive(true);
        v->setPaused(false);
        v->loadLocation(14);
        int held = -1;
        for (int i = 0; i < 2400; ++i) {
            Actor *pl = v->player();
            QPointF tgt = v->debugBossPos();
            if (tgt.x() < 0) tgt = v->debugNearestHostile();
            QPointF cam = v->cameraTopLeft();
            if (tgt.x() >= 0) mouseMove(v, (tgt - cam) * v->zoom());
            int want = -1;
            if (pl && tgt.x() >= 0) {
                float dx = float(tgt.x() - pl->pos.x()), dy = float(tgt.y() - pl->pos.y());
                float dd = qSqrt(dx * dx + dy * dy);
                if (dd > 260.f) want = (qAbs(dx) > qAbs(dy)) ? (dx > 0 ? Qt::Key_D : Qt::Key_A)
                                                             : (dy > 0 ? Qt::Key_S : Qt::Key_W);
            }
            if (want != held) {
                if (held > 0) { QKeyEvent kr(QEvent::KeyRelease, held, Qt::NoModifier); QApplication::sendEvent(v, &kr); }
                if (want > 0) { QKeyEvent kp(QEvent::KeyPress, want, Qt::NoModifier); QApplication::sendEvent(v, &kp); }
                held = want;
            }
            v->debugStep(1.f / 60.f);
            if (pl && tgt.x() >= 0) {
                float dd = qSqrt(float((tgt.x()-pl->pos.x())*(tgt.x()-pl->pos.x()) + (tgt.y()-pl->pos.y())*(tgt.y()-pl->pos.y())));
                if (dd < 300.f && i > 120) break;
            }
        }
        if (held > 0) { QKeyEvent kr(QEvent::KeyRelease, held, Qt::NoModifier); QApplication::sendEvent(v, &kr); }
        v->setZoom(1.15f);
        for (int i = 0; i < 70; ++i) v->debugStep(1.f / 60.f);   // босс замахивается - видны fx
        shell->resize(W, H);
        shot(shell, QStringLiteral("33_boss_sprite"));
        delete shell; delete g;
    }

    // ---------- 9. пустота / финал ----------
    {
        Game *g = makeHero(30, 3, QStringLiteral("Морвэн"));
        GameView *v; Hud *h;
        QWidget *shell = makeShell(g, &v, &h);
        v->setInteractive(true);
        v->setPaused(false);
        v->loadLocation(30);
        for (int i = 0; i < 160; ++i) {
            v->debugStep(1.f / 60.f);
            if (i % 30 == 0) {
                QPointF tgt = v->debugNearestHostile();
                QPointF cam = v->cameraTopLeft();
                if (tgt.x() >= 0) mouseMove(v, tgt - cam);
            }
        }
        shell->resize(W, H);
        shot(shell, QStringLiteral("10_void"));
        delete shell; delete g;
    }

    // ---------- 10. карта мира ----------
    {
        Game *g = makeHero(22, 0, QStringLiteral("Каэн"));
        for (int i = 0; i < 18; ++i) g->completeLocation(i, i == 7);
        WorldMapPanel *map = new WorldMapPanel();
        map->setGame(g);
        map->setSelected(13);
        map->resize(W, H);
        shot(map, QStringLiteral("11_worldmap"));
        delete map; delete g;
    }

    // ---------- 11. пауза и смерть ----------
    {
        Game *g = makeHero(12, 0, QStringLiteral("Каэн"));
        PausePanel *pz = new PausePanel();
        pz->setGame(g);
        pz->resize(W, H);
        {   // курсор на «Продолжить» - проверка постоянной подсветки кнопок
            pz->show();
            QApplication::processEvents();
            for (int i = 0; i < 60; ++i) pz->repaint();   // дождаться конца анимации открытия
            QRectF br = pz->debugButtonRect(500);   // Act_PauseResume
            if (!br.isNull()) mouseMove(pz, br.center());
        }
        shot(pz, QStringLiteral("12_pause"));
        pz->setPage(1);
        pz->setSetTab(0);
        for (int i = 0; i < 60; ++i) pz->repaint();
        shot(pz, QStringLiteral("17_settings"));
        pz->setSetTab(1);
        for (int i = 0; i < 60; ++i) pz->repaint();
        {   // подсветка кнопки света под курсором
            QRectF br = pz->debugButtonRect(521);   // Act_VideoLight
            if (!br.isNull()) mouseMove(pz, br.center());
        }
        shot(pz, QStringLiteral("24_settings_video"));
        pz->setSetTab(2);
        for (int i = 0; i < 60; ++i) pz->repaint();
        shot(pz, QStringLiteral("15_keys"));
        pz->setPage(0);
        DeathPanel *dp = new DeathPanel();
        dp->setGame(g);
        dp->resize(W, H);
        shot(dp, QStringLiteral("13_death"));
        AchPanel *ap = new AchPanel();
        ap->setGame(g);
        ap->resize(W, H);
        shot(ap, QStringLiteral("22_ach"));
        delete ap;
        HelpPanel *hp = new HelpPanel();
        hp->setGame(g);
        hp->resize(W, H);
        shot(hp, QStringLiteral("14_help"));
        delete pz; delete dp; delete hp; delete g;
    }

    qInfo("готово");
    return 0;
}
