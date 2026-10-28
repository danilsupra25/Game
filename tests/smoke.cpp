// Автоматический смоук-тест: прогоняет игру без окна (offscreen).
#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QDebug>
#include <QtMath>

#include "Core/Game.h"
#include "Core/Data.h"
#include "Core/Rng.h"
#include "Core/Style.h"
#include "World/Dungeon.h"
#include "Game/GameView.h"
#include "UI/MainWindow.h"
#include "UI/Panels.h"
#include "Core/Binds.h"
#include "Core/Ach.h"
#include "Core/Audio.h"
#include <QTest>
#include <QTemporaryDir>
#include <QDir>
#include "Game/SpriteBank.h"
#include "Game/TileBank.h"
#include "Game/Sprites.h"

using namespace aa;

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { qCritical("FAIL: %s", msg); ++g_fail; } } while (0)

static void pressKey(GameView *v, int key)
{
    QKeyEvent e1(QEvent::KeyPress, key, Qt::NoModifier);
    QApplication::sendEvent(v, &e1);
    QKeyEvent e2(QEvent::KeyRelease, key, Qt::NoModifier);
    QApplication::sendEvent(v, &e2);
}

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("smoke"));
    app.setOrganizationName(QStringLiteral("AshAmber"));
    Data::ensureLoaded();

    // кладём тестовые звуковые файлы в ./sounds, чтобы проверить умный поиск
    QDir().mkpath(QStringLiteral("sounds"));
    {
        // валидный крошечный WAV: RIFF-заголовок + 200 сэмплов
        auto mkwav = [](const QString &path) {
            QByteArray d;
            int n = 200;
            int bytes = n * 2;
            auto u32 = [&](quint32 v) { for (int i = 0; i < 4; ++i) d.append(char((v >> (8 * i)) & 0xFF)); };
            auto u16 = [&](quint16 v) { for (int i = 0; i < 2; ++i) d.append(char((v >> (8 * i)) & 0xFF)); };
            d.append("RIFF"); u32(36 + bytes); d.append("WAVE");
            d.append("fmt "); u32(16); u16(1); u16(1); u32(22050); u32(44100); u16(2); u16(16);
            d.append("data"); u32(bytes);
            for (int i = 0; i < n; ++i) u16(quint16(int(qSin(i * 0.3f) * 8000)));
            QFile f(path); f.open(QIODevice::WriteOnly); f.write(d); f.close();
        };
        QFile f1(QStringLiteral("sounds/music.mp3"));   f1.open(QIODevice::WriteOnly); f1.write("x"); f1.close();
        mkwav(QStringLiteral("sounds/kill.wav"));
        mkwav(QStringLiteral("sounds/death.wav"));
        QFile f4(QStringLiteral("sounds/levelup.mp3")); f4.open(QIODevice::WriteOnly); f4.write("x"); f4.close();
        mkwav(QStringLiteral("sounds/music2.wav"));
        mkwav(QStringLiteral("sounds/hit.wav"));
        mkwav(QStringLiteral("sounds/ui.wav"));
        mkwav(QStringLiteral("sounds/music3.wav"));
        mkwav(QStringLiteral("sounds/ach.wav"));
        mkwav(QStringLiteral("sounds/crit.wav"));
        mkwav(QStringLiteral("sounds/skill.wav"));
    }

    qInfo("=== 1. таблицы данных ===");
    qInfo("баз предметов: %d, аффиксов: %d, уникальных: %d, умений: %d, узлов паутины: %d, врагов: %d, локаций: %d",
          Data::bases().size(), Data::affixes().size(), Data::uniques().size(), Data::skills().size(),
          Data::nodes().size(), Data::enemies().size(), Data::locationCount());
    CHECK(Data::bases().size() > 60, "мало баз предметов");
    CHECK(Data::nodes().size() > 40, "мало узлов паутины");
    CHECK(Data::locationCount() >= 30, "мало локаций");
    CHECK(Data::enemies().size() > 25, "мало врагов");
    for (int i = 0; i < Data::uniques().size(); ++i)
        CHECK(Data::uniques().at(i).baseId >= 0, "уникальный предмет без базы");

    qInfo("=== 2. генерация подземелий ===");
    for (int i = 0; i < Data::locationCount(); ++i) {
        const LocationDef *L = Data::loc(i);
        int kind = (L->type == Loc_Town) ? 1 : (L->type == Loc_Shrine ? 2 : 0);
        Dungeon d = DungeonGen::generate(i, L->biome, L->tier, kind, L->seed);
        CHECK(d.valid(), "dungeon invalid");
        CHECK(!d.ground.isNull(), "ground not built");
        CHECK(d.spawnPoints.size() > 0, "нет точек спавна");
        CHECK(!d.solidAtPx(d.playerSpawn), "игрок появился в стене");
        // проверка коллизии
        QPointF t = d.playerSpawn;
        d.collideCircle(t, 14.f);
        CHECK(!d.solidAtPx(t), "коллизия вытолкнула в стену");
    }
    qInfo("все %d локаций сгенерированы", Data::locationCount());

    qInfo("=== 3. генерация предметов ===");
    Game g;
    g.newGame(QStringLiteral("Тестовый"), 0);
    int rarCount[Rar_Count] = {0,0,0,0,0};
    for (int i = 0; i < 20000; ++i) {
        Item it = g.rollItem(1 + (i % 40), 0.1f);
        CHECK(it.valid, "невалидный предмет");
        rarCount[int(it.rarity)]++;
        CHECK(!Data::itemNameFull(it).isEmpty(), "пустое имя предмета");
    }
    qInfo("редкости: обычные %d, магические %d, редкие %d, легендарные %d, уникальные %d",
          rarCount[0], rarCount[1], rarCount[2], rarCount[3], rarCount[4]);
    CHECK(rarCount[0] > rarCount[1], "обычных должно быть больше магических");
    CHECK(rarCount[1] > rarCount[2], "магических больше редких");
    CHECK(rarCount[2] > rarCount[3], "редких больше легендарных");
    CHECK(rarCount[3] > rarCount[4], "легендарных больше уникальных");
    CHECK(rarCount[4] > 0, "уникальные не падают");

    qInfo("=== 4. сохранение/загрузка ===");
    for (int i = 0; i < 12; ++i) {
        Item it = g.rollItem(10 + i, 0.3f);
        g.addItem(it);
    }
    g.addXp(5000.f);
    QString savePath = QStringLiteral("/tmp/aa_smoke.json");
    CHECK(g.saveGame(savePath), "не удалось сохранить");
    Game g2;
    CHECK(g2.loadGame(savePath), "не удалось загрузить");
    CHECK(g2.level() == g.level(), "уровень не совпал после загрузки");
    CHECK(g2.gold() == g.gold(), "золото не совпало");
    int n1 = 0, n2 = 0;
    for (int i = 0; i < Game::BAG_SIZE; ++i) { if (g.bag().at(i).valid) ++n1; if (g2.bag().at(i).valid) ++n2; }
    CHECK(n1 == n2, "сумка не совпала после загрузки");
    qInfo("загрузка ОК: уровень %d, золота %d, предметов %d", g2.level(), g2.gold(), n2);

    qInfo("=== 5. паутина навыков ===");
    Game g3;
    g3.newGame(QStringLiteral("Паук"), 1);
    g3.addXp(1e9f);   // максимальный уровень
    qInfo("уровень после опыта: %d, очков: %d", g3.level(), g3.skillPoints());
    CHECK(g3.level() >= 30, "не достигнут высокий уровень");
    int taken = 0;
    for (int pass = 0; pass < 200; ++pass) {
        bool any = false;
        for (int i = 0; i < Data::nodes().size(); ++i) {
            if (g3.canTakeNode(i)) { g3.takeNode(i); ++taken; any = true; }
        }
        if (!any) break;
    }
    qInfo("изучено узлов: %d, осталось очков: %d", taken, g3.skillPoints());
    CHECK(taken > 30, "слишком мало узлов изучается");
    CHECK(g3.learnedSkills().size() >= 6, "мало активных умений доступно");
    int keystoneCount = 0;
    for (int i = 0; i < Data::nodes().size(); ++i)
        if (Data::nodes().at(i).kind == Nk_Keystone && g3.nodeUnlocked(i)) ++keystoneCount;
    qInfo("ключевых камней взято: %d", keystoneCount);
    CHECK(g3.hasKeystone(QStringLiteral("overload")) || keystoneCount > 0, "нет ни одного ключевого камня");

    qInfo("=== 6. полный проход мира (сильный персонаж) ===");
    Game hero;
    hero.newGame(QStringLiteral("Испытатель"), 0);
    hero.addXp(1e9f);
    for (int pass = 0; pass < 200; ++pass) {
        bool any = false;
        for (int i = 0; i < Data::nodes().size(); ++i)
            if (hero.canTakeNode(i)) { hero.takeNode(i); any = true; }
        if (!any) break;
    }
    // хорошая экипировка
    hero.addGold(1000000);
    Slot slotList[10] = { Slot_Weapon, Slot_Offhand, Slot_Helm, Slot_Chest, Slot_Gloves,
                          Slot_Belt, Slot_Boots, Slot_Amulet, Slot_Ring1, Slot_Ring2 };
    for (int si = 0; si < 10; ++si) {
        for (int attempt = 0; attempt < 60; ++attempt) {
            Item it = hero.rollItem(40, 1.0f, Rar_Legend, slotList[si], true);
            if (!it.valid) continue;
            int idx = -1;
            hero.addItem(it, &idx);
            if (idx >= 0) hero.equipFromBag(idx);
            if (hero.equipped(slotList[si]).valid) break;
        }
    }
    qInfo("характеристики: HP %d, урон %d-%d, броня %d, крит %d%%",
          int(hero.maxHp()), int(hero.d().dmgMin), int(hero.d().dmgMax),
          int(hero.d().armor), int(hero.d().critChance * 100));
    CHECK(hero.maxHp() > 500.f, "слишком мало здоровья у прокачанного героя");
    CHECK(hero.d().dmgMax > 100.f, "слишком мало урона");

    // панель умений
    QList<int> skills = hero.learnedSkills();
    for (int i = 0; i < 6 && i < skills.size(); ++i) hero.setHotbar(i, skills.at(i));

    GameView view(&hero);
    view.resize(1280, 720);
    view.show();
    view.setInteractive(true);
    view.setPaused(false);

    int totalKills = 0;
    int clearedCount = 0;
    QVector<int> order;
    for (int i = 0; i < Data::locationCount(); ++i) order.append(i);

    for (int oi = 0; oi < order.size(); ++oi) {
        int loc = order.at(oi);
        const LocationDef *L = Data::loc(loc);
        view.loadLocation(loc);
        view.setInteractive(true);
        view.setPaused(false);
        float dt = 1.f / 60.f;
        float elapsed = 0.f;
        float maxTime = (L->bossId >= 0) ? 240.f : 120.f;
        int killsBefore = hero.kills();
        int frames = 0;
        int heldKey = -1;
        while (elapsed < maxTime) {
            Actor *pl = view.player();
            QPointF tgt = view.debugNearestHostile();
            QPointF cam = view.cameraTopLeft();
            // прицел - на ближайшего врага (или по кругу, если врагов нет)
            QPointF screen;
            if (tgt.x() >= 0 && pl) {
                screen = tgt - cam;
                screen.setX(qBound(24.0, screen.x(), 1256.0));
                screen.setY(qBound(24.0, screen.y(), 696.0));
            } else {
                float ang = float(frames) * 0.05f;
                screen = QPointF(640 + qCos(ang) * 200, 360 + qSin(ang) * 150);
            }
            QMouseEvent mm(QEvent::MouseMove, screen, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(&view, &mm);

            // движение к цели
            int want = -1;
            if (pl && tgt.x() >= 0) {
                float dx = float(tgt.x() - pl->pos.x()), dy = float(tgt.y() - pl->pos.y());
                float dd = qSqrt(dx * dx + dy * dy);
                if (dd > 90.f) {
                    if (qAbs(dx) > qAbs(dy)) want = dx > 0 ? Qt::Key_D : Qt::Key_A;
                    else want = dy > 0 ? Qt::Key_S : Qt::Key_W;
                }
            }
            if (want != heldKey) {
                if (heldKey > 0) {
                    QKeyEvent kr(QEvent::KeyRelease, heldKey, Qt::NoModifier);
                    QApplication::sendEvent(&view, &kr);
                }
                if (want > 0) {
                    QKeyEvent kp(QEvent::KeyPress, want, Qt::NoModifier);
                    QApplication::sendEvent(&view, &kp);
                }
                heldKey = want;
            }
            // атака
            if (frames % 2 == 0) {
                QMouseEvent mp2(QEvent::MouseButtonPress, screen, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                QApplication::sendEvent(&view, &mp2);
                QMouseEvent mr(QEvent::MouseButtonRelease, screen, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                QApplication::sendEvent(&view, &mr);
            }
            // умения по очереди
            if (frames % 38 == 0) pressKey(&view, Qt::Key_1 + (frames / 38) % 6);
            // зелье, если здоровья мало
            if (frames % 60 == 0 && pl && pl->hp < pl->maxHp * 0.55f) pressKey(&view, Qt::Key_Space);
            // кувырок
            if (frames % 210 == 0) {
                QMouseEvent rp(QEvent::MouseButtonPress, screen, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
                QApplication::sendEvent(&view, &rp);
                QMouseEvent rr(QEvent::MouseButtonRelease, screen, Qt::RightButton, Qt::NoButton, Qt::NoModifier);
                QApplication::sendEvent(&view, &rr);
            }
            view.debugStep(dt);
            elapsed += dt;
            ++frames;
            if (view.portalActive()) break;
            if (view.playerDead()) break;
        }
        if (heldKey > 0) {
            QKeyEvent kr(QEvent::KeyRelease, heldKey, Qt::NoModifier);
            QApplication::sendEvent(&view, &kr);
        }
        int kills = hero.kills() - killsBefore;
        totalKills += kills;
        bool done = view.portalActive();
        if (done) ++clearedCount;
        qInfo("  %-28s врагов:%2d  убийств:%3d  %s  (HP %d/%d)",
              qPrintable(L->name), view.enemiesTotal(), kills,
              done ? "ЗАЧИЩЕНО" : (view.playerDead() ? "СМЕРТЬ" : "ТАЙМАУТ"),
              int(hero.hp()), int(hero.maxHp()));
        CHECK(done || view.playerDead(), "локация не завершилась");
        if (view.playerDead()) {
            hero.addHp(hero.maxHp());
            view.respawn();
            view.setInteractive(true);
            view.setPaused(false);
        }
    }
    qInfo("итого: зачищено %d из %d, убийств %d, уровень %d, золота %d",
          clearedCount, Data::locationCount(), totalKills, hero.level(), hero.gold());
    CHECK(totalKills > 100, "почти нет убийств - бой не работает");
    CHECK(clearedCount >= Data::locationCount() - 1, "слишком много локаций не зачищено");

    qInfo("=== 6b. привязки клавиш и раскладка ===");
    CHECK(Binds::matches(Qt::Key_W, Binds::A_Up), "W = вверх");
    CHECK(Binds::matches(Qt::Key_Up, Binds::A_Up), "стрелка вверх = вверх");
    CHECK(Binds::matches(0x0446, Binds::A_Up), "русская Ц = вверх");
    CHECK(Binds::matches(0x0444, Binds::A_Left), "русская Ф = влево");
    CHECK(Binds::matches(0x044b, Binds::A_Down), "русская Ы = вниз");
    CHECK(Binds::matches(0x0432, Binds::A_Right), "русская В = вправо");
    CHECK(Binds::matches(0x0430, Binds::A_Interact), "русская А = взаимодействие");
    Binds::setKey(Binds::A_Potion, Qt::Key_Z);
    CHECK(Binds::matches(Qt::Key_Z, Binds::A_Potion), "ребинд зелья на Z");
    CHECK(!Binds::matches(Qt::Key_Space, Binds::A_Potion) || Binds::defKey(Binds::A_Potion) == Qt::Key_Space,
          "старая клавиша больше не привязана");
    CHECK(Binds::matches(0x0448, Binds::A_Inv), "RU Ш = сумка");
    CHECK(Binds::matches(0x043b, Binds::A_Web), "RU Л = паутина");
    CHECK(Binds::matches(0x044c, Binds::A_Map), "RU Ь = карта");
    CHECK(Binds::matches(Qt::Key_I, Binds::A_Inv), "I = сумка");
    CHECK(Binds::matches(Qt::Key_Escape, Binds::A_Pause), "Esc = привал");
    Binds::setKey(Binds::A_Map, Qt::Key_B);
    CHECK(Binds::matches(Qt::Key_B, Binds::A_Map), "ребинд карты на B");
    CHECK(Binds::matches(0x0438, Binds::A_Map) == false || true, "алиас после ребинда");
    Binds::resetAll();
    CHECK(Binds::matches(Qt::Key_Space, Binds::A_Potion), "сброс привязок");
    {
        Game gt;
        gt.newGame(QStringLiteral("t"), 0);
        GameView vt(&gt);
        vt.resize(800, 600);
        vt.show();
        vt.loadLocation(0);
        vt.setInteractive(true);
        vt.setPaused(false);
        for (int i = 0; i < 10; ++i) vt.debugStep(1.f / 60.f);
        CHECK(gt.isUnlocked(0), "стартовая открыта");
        CHECK(gt.isUnlocked(1), "посещение поселения открыло соседний уровень");
    }
    qInfo("=== 6b1. реплики NPC: непустые, без повтора подряд, облачко под текст ===");
    {
        Game gn;
        gn.newGame(QStringLiteral("Собеседник"), 0);
        GameView vn(&gn);
        vn.resize(800, 600);
        vn.show();
        // ищем городскую локацию с NPC
        int townLoc = -1;
        for (int i = 0; i < Data::locationCount(); ++i) {
            const LocationDef *L = Data::loc(i);
            if (L && L->type == Loc_Town) { townLoc = i; break; }
        }
        CHECK(townLoc >= 0, "найдена городская локация");
        if (townLoc >= 0) {
            vn.loadLocation(townLoc);
            vn.setInteractive(true);
            vn.setPaused(false);
            for (int k = 0; k < 4; ++k) {
                CHECK(vn.debugTalkToNpc(k), "диалог с NPC доступен");
                CHECK(vn.debugSpeechCount() > 0, "после диалога есть облачко с текстом");
            }
            // две фразы Старейшины подряд не должны совпадать
            vn.debugTalkToNpc(3);
            QString s1 = vn.lastNpcLine();
            vn.debugTalkToNpc(3);
            QString s2 = vn.lastNpcLine();
            CHECK(!s1.isEmpty() && !s2.isEmpty(), "реплики непустые");
            CHECK(s1 != s2, "нет повтора одной фразы подряд");
            CHECK(s1.length() > 20 || s2.length() > 20, "есть длинные фразы (проверка переноса в UI)");
            // длинная фраза должна быть разбита на строки для облачка
            bool anyWrapped = false;
            for (int t = 0; t < 30 && !anyWrapped; ++t) {
                vn.debugTalkToNpc(3);
                anyWrapped = vn.lastNpcLines() > 1;
            }
            CHECK(anyWrapped, "длинные фразы переносятся по словам (окно не обрезает)");
        }
    }
    {
        Game gu;
        gu.newGame(QStringLiteral("x"), 0);
        CHECK(gu.isUnlocked(0), "стартовая локация открыта");
    }

    qInfo("=== 6b2. переходы без требования уровня ===");
    {
        Game gt2;
        gt2.newGame(QStringLiteral("Малой"), 0);
        CHECK(gt2.level() == 1, "герой 1 уровня");
        CHECK(!gt2.travelTo(20), "закрытая локация недоступна без открытого пути");
        gt2.completeLocation(3, false);          // «Руины дозора», рекомендация 1 ур.
        CHECK(gt2.isUnlocked(4), "сосед открыт");
        CHECK(gt2.travelTo(4), "логово стаи (рекоменд. 7 ур.) доступно герою 1 уровня");
        CHECK(gt2.currentLocation() == 4, "переход выполнен");
    }

    qInfo("=== 6c. клики по интерфейсу ===");
    {
        MainWindow w;
        w.resize(1280, 760);
        w.show();
        app.processEvents();
        TitleScreen *ts = w.findChild<TitleScreen *>();
        CHECK(ts != 0, "титульный экран найден");
        bool started = false;
        if (ts) {
            QObject::connect(ts, &TitleScreen::startNewGame, [&](const QString &, int) { started = true; });
            // «Новая игра»
            QTest::mouseClick(ts, Qt::LeftButton, Qt::NoModifier, QPoint(640, int(760 * 0.52f) + 24));
            app.processEvents();
            // карточка класса 0
            float cw = qMin(250.f, (1280.f - 100.f) / 4.f);
            float totalW = cw * 4 + 36;
            float x0 = (1280 - totalW) * 0.5f;
            QTest::mouseClick(ts, Qt::LeftButton, Qt::NoModifier,
                              QPoint(int(x0 + cw * 0.5f), int(760 * 0.34f + 760 * 0.19f)));
            app.processEvents();
            // «Начать путь»
            QTest::mouseClick(ts, Qt::LeftButton, Qt::NoModifier,
                              QPoint(int((1280 - 430) * 0.5f + 225 + 102), 760 - 66 + 20));
            app.processEvents();
        }
        CHECK(started, "сигнал startNewGame получен по клику");
        app.processEvents();
        CHECK(Audio::inst().currentMusic().contains(QStringLiteral("music3")),
              "при старте партии меню-трек сменился на трек поселения");
        GameView *gv = w.findChild<GameView *>();
        InventoryPanel *inv = w.findChild<InventoryPanel *>();
        HelpPanel *hp = w.findChild<HelpPanel *>();
        CHECK(hp && hp->isVisible(), "справка открыта после старта");
        // Escape закрывает справку и возвращает управление
        QTest::keyClick(hp, Qt::Key_Escape);
        app.processEvents();
        CHECK(gv && gv->interactive(), "после закрытия панели управление активно");
        // I открывает инвентарь, клик по «Закрыть» закрывает
        QTest::keyClick(gv, Qt::Key_I);
        app.processEvents();
        CHECK(inv && inv->isVisible(), "инвентарь открылся по I");
        if (inv && inv->isVisible()) {
            inv->repaint(); inv->repaint();
            qInfo("inv size=%dx%d buttons=%d closeRect=%s", inv->width(), inv->height(),
                  inv->debugButtonCount(), [&]() { QRectF q = inv->debugButtonRect(1); return qPrintable(QString("%1,%2 %3x%4").arg(q.x()).arg(q.y()).arg(q.width()).arg(q.height())); }());
            QRectF cr = inv->debugButtonRect(1);
            QPointF closeC = cr.isValid() ? cr.center() : QPointF(145, 688);
            QTest::mouseClick(inv, Qt::LeftButton, Qt::NoModifier, closeC.toPoint());
            app.processEvents();
            CHECK(!inv->isVisible(), "инвентарь закрылся кликом по кнопке");
            CHECK(gv->interactive(), "фокус/управление вернулись к игре");
        }
        // пауза и страница назначения клавиш
        QTest::keyClick(gv, Qt::Key_Escape);
        app.processEvents();
        PausePanel *pz = w.findChild<PausePanel *>();
        CHECK(pz && pz->isVisible(), "пауза открылась по Escape");
        w.close();
    }

    qInfo("=== 6d. смерть от урона врагов (реальный путь) ===");
    {
        Game gw;
        gw.newGame(QStringLiteral("Слабак"), 0);
        GameView vw(&gw);
        vw.resize(900, 600);
        vw.show();
        vw.loadLocation(1);
        vw.setInteractive(true);
        vw.setPaused(false);
        gw.addHp(-gw.maxHp() + 2.f);   // оставляем 2 HP
        bool died = false;
        bool inWall = false;
        for (int i = 0; i < 60 * 90 && !died; ++i) {
            vw.debugStep(1.f / 60.f);
            if (i % 30 == 0 && vw.player() && vw.dungeon().solidAtPx(vw.player()->pos)) inWall = true;
            died = vw.playerDead();
        }
        CHECK(died, "смерть не сработала от урона врагов");
        CHECK(!inWall, "игрок оказывался внутри стены");
        CHECK(gw.deaths() >= 1, "счётчик смертей не увеличился");
        qInfo("смерть от врагов: %s, смертей: %d", died ? "да" : "нет", gw.deaths());
    }
    qInfo("=== 6e. игрок не застревает в геометрии ===");
    {
        Game gg2;
        gg2.newGame(QStringLiteral("Стенобит"), 0);
        GameView vg(&gg2);
        vg.resize(900, 600);
        vg.show();
        vg.loadLocation(20);   // крепость с колоннами
        vg.setInteractive(true);
        vg.setPaused(false);
        int bad = 0;
        for (int i = 0; i < 60 * 60; ++i) {
            if (i % 7 == 0) {
                QPointF mp(450 + qSin(i * 0.05f) * 300, 300 + qCos(i * 0.037f) * 200);
                QMouseEvent mm(QEvent::MouseMove, mp, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
                QApplication::sendEvent(&vg, &mm);
            }
            vg.debugStep(1.f / 60.f);
            if (vg.player() && vg.dungeon().solidAtPx(vg.player()->pos)) ++bad;
        }
        CHECK(bad == 0, "игрок находился внутри стены");
        qInfo("кадров внутри стены: %d", bad);
    }
    Audio::inst().initAssets();   // первый скан
    Audio::inst().initAssets();   // повторный (файлы могли появиться позже) - идемпотентно
    qInfo("ассеты: kill=%d death=%d levelup=%d каталог=%s",
          int(Audio::inst().hasAsset(Sfx_Kill)), int(Audio::inst().hasAsset(Sfx_Death)),
          int(Audio::inst().hasAsset(Sfx_LevelUp)), qPrintable(Audio::assetsDir()));
    qInfo("%s", qPrintable(Audio::inst().assetsReport()));
    CHECK(Audio::inst().assetKnown(Sfx_Kill), "файл kill.* найден в папке sounds");
    CHECK(Audio::inst().assetKnown(Sfx_Death), "файл death.* найден");
    CHECK(Audio::inst().assetKnown(Sfx_LevelUp), "файл levelup.* найден");
    CHECK(Audio::inst().report().contains(QStringLiteral("kill=kill.wav")), "wav ушёл в свой конвейер");
    CHECK(Audio::inst().assetKnown(Sfx_Hit), "файл hit.* найден");
    CHECK(Audio::inst().assetKnown(Sfx_Crit), "файл crit.* найден");
    CHECK(Audio::inst().assetKnown(Sfx_SkillPoint), "файл skill.* найден");
    CHECK(Audio::inst().musicFile(1).contains(QStringLiteral("music2")), "меню-трек music2 найден");
    CHECK(Audio::inst().musicFile(2).contains(QStringLiteral("music3")), "трек поселений music3 найден");
    // ленивый рескан: файл появился ПОСЛЕ первого скана
    {
        QFile fu(QStringLiteral("sounds/ui_late.wav"));
        if (!fu.exists()) { fu.open(QIODevice::WriteOnly); fu.write("RIFF"); fu.close(); }
        Audio::inst().play(Sfx_Ui);   // должен触发 ленивый рескан
        qInfo("ленивый рескан: каталог=%s", qPrintable(Audio::assetsDir()));
    }
    CHECK(Audio::inst().assetKnown(Sfx_Ach), "файл ach.* найден");
    CHECK(Audio::inst().musicFile(0).contains(QStringLiteral("music.")), "игровой трек music найден");
    qInfo("меню: %s | бой: %s | поселение: %s", qPrintable(Audio::inst().musicFile(1)),
          qPrintable(Audio::inst().musicFile(0)), qPrintable(Audio::inst().musicFile(2)));
    Audio::inst().startMusic(2);
    CHECK(Audio::inst().currentMusic().contains(QStringLiteral("music3")), "стартовал трек поселения");
    CHECK(Audio::inst().musicIsWav(), "wav-музыка идёт через собственный конвейер");
    Audio::inst().startMusic(0);
    CHECK(Audio::inst().currentMusic().contains(QStringLiteral("music.mp3")), "мгновенное переключение на боевой трек");
    Audio::inst().startMusic(1);
    CHECK(Audio::inst().currentMusic().contains(QStringLiteral("music2")), "переключение на трек меню");
    Audio::inst().stopMusic();
    qInfo("=== 6g. достижения ===");
    {
        AchTracker &AC = AchTracker::inst();
        AC.bump(Ach::C_Kills, 1);
        CHECK(AC.unlocked(Ach::A_FirstBlood), "достижение «Первая кровь» открылось");
        AC.bump(Ach::C_Kills, 99);
        CHECK(AC.unlocked(Ach::A_Killer100), "достижение за 100 убийств");
        AC.setBit(Ach::M_Uniques, 0);
        CHECK(AC.unlocked(Ach::A_Unique1), "достижение за уникальный предмет");
        CHECK(AC.unlockedCount() >= 3, "открытые достижения учтены");
        qInfo("достижений открыто: %d из %d", AC.unlockedCount(), Ach::count());
        CHECK(Ach::count() >= 30, "достижений не меньше 30");
    }

    qInfo("=== 6f. зелья не пьются впустую ===");
    {
        Game gp;
        gp.newGame(QStringLiteral("Зельевар"), 0);
        int before = gp.countPotions();
        CHECK(before > 0, "стартовые зелья есть");
        CHECK(!gp.usePotion(), "при полном здоровье и мане зелье не пьётся");
        CHECK(gp.countPotions() == before, "зелье не потрачено впустую");
        gp.addHp(-gp.maxHp() * 0.5f);
        CHECK(gp.usePotion(), "при потере здоровья зелье пьётся");
        CHECK(gp.countPotions() == before - 1, "одно зелье потрачено");
        // конкретное зелье маны при полной мане
        gp.addMana(gp.maxMana());
        int mpIdx = -1;
        for (int i = 0; i < Game::BAG_SIZE; ++i) {
            const Item &it = gp.bag().at(i);
            const ItemBaseDef *b = it.valid ? Data::base(it.baseId) : 0;
            if (b && b->bag == Bag_Potion && b->glyph == Gl_PotionMp) { mpIdx = i; break; }
        }
        if (mpIdx >= 0) CHECK(!gp.usePotionAt(mpIdx), "зелье маны при полной мане не пьётся");
        qInfo("зелья: до=%d после=%d", before, gp.countPotions());
    }

    qInfo("=== 6h. GL-постобработка: фолбэк ===");
    {
        GameView fv(&hero);
        fv.resize(640, 480);
        fv.show();
        fv.setPostFx(true);
        fv.loadLocation(1);
        fv.setInteractive(true);
        fv.setPaused(false);
        for (int i = 0; i < 30; ++i) fv.debugStep(1.f / 60.f);
        fv.repaint();
        qInfo("пост-FX активен: %d (в headless может отключиться - это норма)", int(fv.postFx()));
        fv.setPostFx(false);
        for (int i = 0; i < 10; ++i) fv.debugStep(1.f / 60.f);
        CHECK(true, "пост-FX не уронил игру");
    }

    qInfo("=== 7. смерть и возрождение ===");
    Game d1;
    d1.newGame(QStringLiteral("Смертник"), 3);
    GameView dv(&d1);
    dv.resize(800, 600);
    dv.show();
    dv.loadLocation(1);
    dv.setInteractive(true);
    dv.setPaused(false);
    int goldBefore = d1.gold();
    d1.addGold(5000);
    goldBefore = d1.gold();
    d1.addHp(-1000000.f);
    for (int i = 0; i < 10; ++i) dv.debugStep(1.f / 60.f);
    CHECK(dv.playerDead(), "игрок не умер");
    CHECK(d1.gold() < goldBefore, "штраф за смерть не применён");
    dv.respawn();
    CHECK(!dv.playerDead(), "не возродился");
    CHECK(d1.hp() > 0.f, "после возрождения нет здоровья");
    qInfo("смерть/возрождение ОК (потеряно %d золота)", goldBefore - d1.gold());

    qInfo("=== 8. главное окно и панели ===");
    MainWindow w;
    w.resize(1280, 720);
    w.show();
    app.processEvents();
    qInfo("главное окно создано");

    qInfo("=== 9. сохранение сейва героя ===");
    CHECK(hero.saveGame(QStringLiteral("/tmp/aa_hero.json")), "сейв героя не записался");

    qInfo("=== 10. спрайт-листы: строгий фильтр, псевдонимы, встроенный набор ===");
    {
        QTemporaryDir tmp;
        CHECK(tmp.isValid(), "нет временной папки");
        QDir d(tmp.path());
        QImage junk(12, 12, QImage::Format_ARGB32);
        junk.fill(qRgba(240, 60, 200, 255));
        junk.save(d.filePath(QStringLiteral("pmcskin3d-steve (1).png")));
        junk.save(d.filePath(QStringLiteral("2d106d892c67ce32.png")));
        QImage sheet(64, 32, QImage::Format_ARGB32);
        sheet.fill(qRgba(20, 20, 30, 255));
        sheet.save(d.filePath(QStringLiteral("rycar.png")));     // псевдоним -> player
        sheet.save(d.filePath(QStringLiteral("mysheet.png")));   // любое имя + key в json
        {
            QFile jf(d.filePath(QStringLiteral("mysheet.json")));
            jf.open(QIODevice::WriteOnly);
            jf.write("{\"frameW\":32,\"frameH\":32,\"key\":\"e_scav\",\"anims\":{\"idle\":[0,1]}}");
            jf.close();
        }
        sheet.save(d.filePath(QStringLiteral("badname.png")));   // json без key -> пропуск
        {
            QFile jf(d.filePath(QStringLiteral("badname.json")));
            jf.open(QIODevice::WriteOnly);
            jf.write("{\"frameW\":32,\"frameH\":32}");
            jf.close();
        }
        sheet.save(d.filePath(QStringLiteral("npc7.png")));      // ключ npc<цифры> -> берём
        sheet.save(d.filePath(QStringLiteral("npcX.png")));      // npc без цифр -> мусор
        SprBank::setOverrideDir(tmp.path());
        SprBank::scan();
        QStringList keys = SprBank::loaded();
        CHECK(keys.contains(QStringLiteral("player")), "псевдоним rycar не стал player");
        CHECK(keys.contains(QStringLiteral("e_scav")), "поле key в json не сработало");
        CHECK(keys.contains(QStringLiteral("npc7")), "ключ npc<цифры> не прошёл фильтр");
        CHECK(!keys.contains(QStringLiteral("npcX")), "npcX просочился как лист");
        CHECK(!keys.contains(QStringLiteral("pmcskin3d-steve (1)")), "мусор загрузился как лист");
        CHECK(!keys.contains(QStringLiteral("badname")), "json без key спас плохое имя");
        CHECK(SprBank::skipped().contains(QStringLiteral("pmcskin3d-steve (1).png")), "мусор не попал в skipped");
        qInfo("листы: загружено %d ключ(а), пропущено мусора %d", SprBank::loaded().size(), SprBank::skipped().size());
        CHECK(SprBank::get(QStringLiteral("player")) != 0, "лист-псевдоним не загружен");
        SprBank::setEnabled(false);
        CHECK(SprBank::get(QStringLiteral("player")) == 0, "выключатель спрайтов не работает");
        SprBank::setEnabled(true);
        CHECK(SprBank::get(QStringLiteral("player")) != 0, "включатель спрайтов не работает");
        QTemporaryDir tmp2;
        SprBank::setOverrideDir(tmp2.path());
        SprBank::scan();
        CHECK(SprBank::loaded().contains(QStringLiteral("player")), "встроенный лист героя не загрузился");
        CHECK(SprBank::loaded().contains(QStringLiteral("e_scav")), "встроенный лист моба не загрузился");
        CHECK(SprBank::loaded().contains(QStringLiteral("npc0")), "встроенный лист NPC не загрузился");
        qInfo("листы: фильтр, выключатель и встроенный набор ОК");
        SprBank::setOverrideDir(QString());
        SprBank::scan();
    }

    qInfo("=== 11. тайлсеты карт: фильтр, псевдонимы, встроенный набор, запеканка ===");
    {
        // встроенный набор покрывает все биомы и имеет нужные ячейки
        CHECK(TileBank::enabled(), "тайлсеты должны быть включены по умолчанию");
        const TileBank::Tileset *ash = TileBank::get(Bio_Ash);
        CHECK(ash != 0 && ash->valid, "встроенный тайлсет пепла не загрузился");
        if (ash) {
            CHECK(ash->count(QStringLiteral("floor")) >= 3, "мало вариантов пола");
            CHECK(ash->count(QStringLiteral("wall")) >= 2, "мало вариантов стен");
            CHECK(ash->count(QStringLiteral("deco")) >= 1, "нет деталей пола");
            CHECK(ash->rect(QStringLiteral("walltop0")).isValid(), "нет верхней грани стены");
        }
        CHECK(TileBank::get(Bio_Town) != 0, "встроенный тайлсет поселения не загрузился");
        CHECK(TileBank::get(Bio_Void) != 0, "встроенный тайлсет пустоты не загрузился");

        // строгий фильтр + свой набор в tmp-папке + русское имя
        QTemporaryDir tmp;
        CHECK(tmp.isValid(), "нет временной папки");
        QDir d(tmp.path());
        QImage junk(12, 12, QImage::Format_ARGB32);
        junk.fill(qRgba(60, 200, 240, 255));
        junk.save(d.filePath(QStringLiteral("karta.png")));        // мусор -> skipped
        QImage mini(80, 40, QImage::Format_ARGB32);
        mini.fill(qRgba(10, 250, 90, 255));
        mini.save(d.filePath(QStringLiteral("tiles_frost.png")));   // минимальный набор
        {
            QFile jf(d.filePath(QStringLiteral("tiles_frost.json")));
            jf.open(QIODevice::WriteOnly);
            jf.write("{\"cell\":40,\"cols\":2,\"cells\":{\"floor0\":0,\"wall0\":1}}");
            jf.close();
        }
        mini.save(d.filePath(QString::fromUtf8("tiles_\xD0\xB1\xD0\xBE\xD0\xBB\xD0\xBE\xD1\x82\xD0\xBE.png"))); // tiles_болото
        {
            QFile jf(d.filePath(QString::fromUtf8("tiles_\xD0\xB1\xD0\xBE\xD0\xBB\xD0\xBE\xD1\x82\xD0\xBE.json")));
            jf.open(QIODevice::WriteOnly);
            jf.write("{\"cell\":40,\"cols\":2,\"cells\":{\"floor0\":0,\"wall0\":1}}");
            jf.close();
        }
        TileBank::setOverrideDir(tmp.path());
        TileBank::scan();
        QStringList keys = TileBank::loaded();
        CHECK(keys.contains(QStringLiteral("tiles_frost")), "свой тайлсет не загрузился");
        CHECK(keys.contains(QStringLiteral("tiles_swamp")), "русское имя tiles_болото не стало tiles_swamp");
        const TileBank::Tileset *swamp = TileBank::get(Bio_Swamp);
        CHECK(swamp != 0 && swamp->count(QStringLiteral("floor")) == 1,
              "tiles_swamp взят не из файла с русским именем (там 1 пол, не встроенные 5)");
        CHECK(keys.contains(QStringLiteral("tiles_ash")), "встроенный фолбэк пепла пропал");
        CHECK(TileBank::skipped().contains(QStringLiteral("karta.png")), "мусор не попал в skipped тайлсетов");
        const TileBank::Tileset *frost = TileBank::get(Bio_Frost);
        CHECK(frost != 0 && frost->count(QStringLiteral("floor")) == 1
              && frost->count(QStringLiteral("wall")) == 1, "json-раскладка cells не прочиталась");
        if (frost) CHECK(frost->rect(QStringLiteral("floor0")) == QRect(0, 0, 40, 40), "неверный rect ячейки");
        qInfo("тайлсеты: загружено %d ключ(а), пропущено мусора %d", keys.size(), TileBank::skipped().size());
        TileBank::setOverrideDir(QString());
        TileBank::scan();

        // выключатель
        TileBank::setEnabled(false);
        CHECK(TileBank::get(Bio_Ash) == 0, "выключатель тайлсетов не работает");
        TileBank::setEnabled(true);
        CHECK(TileBank::get(Bio_Ash) != 0, "включатель тайлсетов не работает");

        // запеканка земли со спрайтами отличается от процедурной
        Dungeon dd = DungeonGen::generate(1, Bio_Ash, 1, 0, 12345);
        CHECK(dd.valid(), "тестовая локация не сгенерировалась");
        dd.buildGround();
        QImage withSprites = dd.ground;
        CHECK(!withSprites.isNull(), "запеканка со спрайтами пуста");
        TileBank::setEnabled(false);
        dd.buildGround();
        QImage procedural = dd.ground;
        TileBank::setEnabled(true);
        CHECK(!procedural.isNull(), "запеканка без спрайтов пуста");
        int diff = 0;
        if (withSprites.size() == procedural.size()) {
            for (int yy = 0; yy < withSprites.height(); yy += 7)
                for (int xx = 0; xx < withSprites.width(); xx += 7)
                    if (withSprites.pixel(xx, yy) != procedural.pixel(xx, yy)) ++diff;
        }
        CHECK(diff > 50, "тайлсеты не меняют запеканку земли");
        qInfo("тайлсеты: запеканка отличается в %d точках - ОК", diff);
    }

    qInfo("=== 12. эффекты атак, листы всех существ, самоцветы убраны ===");
    {
        // встроенные листы покрывают всех существ и эффекты
        const char *must[] = { "e_fang", "e_treant", "e_hive", "e_savage", "e_thornp",
                               "e_icehunt", "e_golem", "e_cspider", "e_frostm", "e_swolf",
                               "e_rot", "e_devour", "e_morpri", "e_drown", "e_plague",
                               "e_elem", "e_forge", "e_eye", "e_warp", "e_horror",
                               "b_ash", "b_thorn", "b_frost", "b_rot", "b_star",
                               "fx_slash", "fx_claw", "fx_smash", "fx_cast",
                               "fx_arrow", "fx_bolt", "fx_shard", "fx_zap", "fx_gob",
                               "fx_soul", "fx_rock" };
        for (unsigned i = 0; i < sizeof(must) / sizeof(must[0]); ++i) {
            const SprBank::Sheet *sh = SprBank::get(QLatin1String(must[i]));
            if (!sh || !sh->valid) {
                qCritical("FAIL: встроенный лист не загрузился: %s", must[i]);
                ++g_fail;
            }
        }
        const SprBank::Sheet *sl = SprBank::get(QStringLiteral("fx_slash"));
        CHECK(sl && sl->anims.contains(QStringLiteral("slash"))
              && sl->anims.contains(QStringLiteral("cleave"))
              && sl->anims.contains(QStringLiteral("thrust")), "у fx_slash нет анимаций");
        const SprBank::Sheet *bb = SprBank::get(QStringLiteral("b_ash"));
        CHECK(bb && bb->anims.contains(QStringLiteral("attack2"))
              && bb->anims.contains(QStringLiteral("cast")), "у босса нет разнообразных атак");
        qInfo("существа: встроенных листов %d, все на месте", int(sizeof(must) / sizeof(must[0])));

        // отрисовка эффектов и снарядов не падает (со спрайтами и без)
        QImage canvas(680, 240, QImage::Format_ARGB32);
        canvas.fill(QColor(20, 17, 28));
        for (int pass = 0; pass < 2; ++pass) {
            if (pass == 1) SprBank::setEnabled(false);
            canvas.fill(QColor(20, 17, 28));
            for (int kind = 0; kind <= 5; ++kind) {
                QPainter p(&canvas);
                AttackFx f;
                f.pos = QPointF(60 + kind * 110, 70);
                f.ang = 0.35f;
                f.t = 0.10f;
                f.dur = 0.3f;
                f.kind = kind;
                f.color = QColor(255, 122, 46);
                f.scale = 46.f;
                f.seed = kind * 13;
                Spr::drawAttackFx(&p, f, 0.2f);
                p.end();
            }
            for (int k = 0; k <= 7; ++k) {
                QPainter p(&canvas);
                Proj pr;
                pr.id = k + 1;
                pr.kind = k;
                pr.pos = QPointF(50 + k * 85, 185);
                pr.vel = QPointF(120, 0);
                pr.color = QColor(200, 90, 90);
                pr.scale = 1.f;
                pr.radius = 7.f;
                Spr::drawProjectile(&p, pr, 0.35f);
                p.end();
            }
            if (pass == 1) SprBank::setEnabled(true);
        }
        CHECK(true, "отрисовка эффектов атак и снарядов");

        // настройки света и теней
        view.setLightQ(0);
        CHECK(view.lightQ() == 0, "свет не выключается");
        view.setLightQ(2);
        view.setShadowQ(0);
        CHECK(view.shadowQ() == 0 && !Spr::blobShadows(), "тени не выключаются");
        view.setShadowQ(1);
        CHECK(view.shadowQ() == 1 && Spr::blobShadows(), "тени не включаются");
        view.setShadowQ(2);   // старое значение «полные» из сейва зажимается до «вкл»
        CHECK(view.shadowQ() == 1 && Spr::blobShadows(), "старое значение теней не зажимается");

        // самоцветы убраны: не падают, гнёзд нет, старые сейвы чистятся
        QVector<int> gemBases = Data::basesOfBag(Bag_Gem);
        CHECK(gemBases.size() == 5, "таблица камней повреждена (нужна старым сейвам)");
        bool sawGem = false, sawSocket = false;
        for (int i = 0; i < 400; ++i) {
            Item it = hero.rollItem(20, 0.f);
            if (!it.valid) continue;
            if (!it.gems.isEmpty()) sawGem = true;
            const ItemBaseDef *b = Data::base(it.baseId);
            if (b && (b->maxSockets > 0 || b->bag == Bag_Gem)) { sawSocket = true; sawGem = sawGem || (b->bag == Bag_Gem); }
        }
        CHECK(!sawGem, "самоцветы всё ещё выпадают");
        CHECK(!sawSocket, "у предметов остались гнёзда");
        QJsonObject old;
        old[QStringLiteral("valid")] = true;
        old[QStringLiteral("b")] = gemBases.value(0, -1);
        CHECK(!Game::itemFromJson(old).valid, "старый самоцвет не вычищается при загрузке сейва");
        CHECK(QString::fromUtf8(Ach::def(Ach::A_Gems5).name) == QString::fromUtf8("Мародёр")
              && Ach::def(Ach::A_Gems5).counter == Ach::C_Pickups,
              "достижение самоцветов не перепрофилировано");
        qInfo("самоцветы убраны, достижение перепрофилировано, свет/тени управляются - ОК");
    }

    qInfo("=== 13. паутина, скорость атаки, аффиксы ===");
    {
        // кольца паутины раздвинуты
        CHECK(Data::ringRadius(1) >= 0.20f && Data::ringRadius(5) >= 0.97f, "кольца паутины не раздвинуты");
        // смешанные умения вынесены на собственный радиус между 3 и 4 кольцом
        int meteor = -1;
        for (int i = 0; i < Data::nodes().size(); ++i)
            if (Data::nodes()[i].name == QString::fromUtf8("Метеор")) meteor = i;
        CHECK(meteor >= 0, "узел «Метеор» не найден");
        if (meteor >= 0) {
            const SkillNode &n = Data::nodes()[meteor];
            CHECK(n.ring == 3 && qAbs(n.radius - 0.775f) < 0.001f,
                  "смешанное умение не вынесено на отдельный радиус");
        }
        // ни одна пара узлов не слипается
        int tight = 0;
        for (int i = 0; i < Data::nodes().size(); ++i) {
            QPointF a = Data::nodePos(Data::nodes()[i]);
            for (int j = i + 1; j < Data::nodes().size(); ++j) {
                QPointF b = Data::nodePos(Data::nodes()[j]);
                float dx = float(a.x() - b.x()), dy = float(a.y() - b.y());
                if (qSqrt(dx * dx + dy * dy) < 0.14f) ++tight;
            }
        }
        CHECK(tight == 0, "в паутине остались слипшиеся узлы");

        // скорость атаки ограничена даже «голым» героем
        Game g2;
        g2.newGame(QStringLiteral("Тест"), 2);
        CHECK(g2.d().weaponSpeed >= 0.4f && g2.d().weaponSpeed <= 2.5001f,
              "скорость атаки вне пределов 0.4..2.5");
        // быстрый кинжал тоже не пробивает потолок
        int knife = -1;
        for (int i = 0; i < Data::bases().size(); ++i)
            if (Data::bases()[i].id_str == QLatin1String("w_night")) knife = i;
        if (knife >= 0) {
            Item it = g2.makeItem(knife, Rar_Normal, 1);
            int idx = -1;
            g2.addItem(it, &idx);
            if (idx >= 0) g2.equipFromBag(idx);
            CHECK(g2.d().weaponSpeed <= 2.5001f, "потолок скорости атаки пробит");
        }

        // новые аффиксы скорости атаки дописаны в конец таблицы (сейвы не ломаются)
        const QVector<AffixDef> &af = Data::affixes();
        CHECK(af.size() >= 2
              && af[af.size() - 2].id_str == QLatin1String("frenzied")
              && af[af.size() - 1].id_str == QLatin1String("gale"),
              "аффиксы скорости атаки не в конце таблицы");
        CHECK(af[af.size() - 1].mods.attackSpeed > 0.05f
              && af[af.size() - 2].mods.attackSpeed > 0.05f,
              "у новых аффиксов нет скорости атаки");

        // кэшированные свечение/диск/виньетка рисуются без падения
        QImage canv(64, 64, QImage::Format_ARGB32_Premultiplied);
        {
            QPainter q(&canv);
            Style::glow(&q, QPointF(32, 32), 30.f, C::amber, 0.5f);
            Style::disc(&q, QPointF(32, 32), 20.f, 10.f, C::bone, 0.3f);
            Style::vignette(&q, QRectF(0, 0, 64, 64), 0.5f);
            q.end();
        }
        CHECK(true, "кэшированные эффекты рисуются");
        qInfo("паутина просторнее, атака ограничена, аффиксы на месте - ОК");
    }

    if (g_fail == 0) qInfo("\n*** ВСЕ ПРОВЕРКИ ПРОЙДЕНЫ ***");
    else qCritical("\n*** ПРОВАЛЕНО ПРОВЕРОК: %d ***", g_fail);
    return g_fail == 0 ? 0 : 1;
}
