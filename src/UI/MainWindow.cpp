#include "UI/MainWindow.h"
#include "UI/Panels.h"
#include "UI/Hud.h"
#include "UI/Tooltip.h"
#include "Core/Game.h"
#include "Core/Data.h"
#include "Core/Style.h"
#include "Core/Audio.h"
#include "Core/Binds.h"
#include "Core/Ach.h"
#include "Game/SpriteBank.h"
#include "Game/TileBank.h"
#include "Game/GameView.h"

#include <QStackedWidget>
#include <QGridLayout>
#include <QSettings>
#include <QMouseEvent>
#include <QCloseEvent>
#include <QMessageBox>
#include <QApplication>
#include <QTimer>
#include <QShowEvent>
#include <QDesktopWidget>
#include <algorithm>

namespace aa {

MainWindow::MainWindow()
    : m_sound(true), m_shake(true), m_playing(false), m_musicStarted(false), m_pixel(1), m_scan(false), m_gl(false),
      m_sprites(true), m_lightQ(2), m_shadowQ(1), m_invSort(0)
{
    Data::ensureLoaded();
    m_game = new Game(this);
    buildUi();
    loadSettings();
    applySettings();

    connect(m_game, SIGNAL(logMessage(QString, QColor)), this, SLOT(onLog(QString, QColor)));
    connect(m_game, SIGNAL(leveledUp(int)), this, SLOT(onLevelUp(int)));

    QRect avail = QApplication::desktop()->availableGeometry(this);
    int w = qMin(1360, avail.width() - 60);
    int h = qMin(820, avail.height() - 60);
    resize(w, h);
    setWindowTitle(QStringLiteral("Пепел и Янтарь"));
}

MainWindow::~MainWindow() {}

void MainWindow::buildUi()
{
    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

    // ---- титульный экран ----
    m_title = new TitleScreen(m_stack);
    m_stack->addWidget(m_title);
    connect(m_title, SIGNAL(startNewGame(QString, int)), this, SLOT(onStartNewGame(QString, int)));
    connect(m_title, SIGNAL(continueGame()), this, SLOT(onContinueGame()));
    connect(m_title, SIGNAL(quitGame()), this, SLOT(onQuitGame()));
    connect(m_title, SIGNAL(showHelp()), this, SLOT(onTitleHelp()));

    // ---- игровая оболочка ----
    m_shell = new QWidget(m_stack);
    m_stack->addWidget(m_shell);
    QGridLayout *gl = new QGridLayout(m_shell);
    gl->setContentsMargins(0, 0, 0, 0);
    gl->setSpacing(0);

    m_view = new GameView(m_game, m_shell);
    gl->addWidget(m_view, 0, 0);
    m_view->installEventFilter(this);

    m_hud = new Hud(m_game, m_view, m_shell);
    gl->addWidget(m_hud, 0, 0);

    m_inv = new InventoryPanel(m_shell);
    m_web = new SkillWebPanel(m_shell);
    m_map = new WorldMapPanel(m_shell);
    m_shop = new ShopPanel(m_shell);
    m_pause = new PausePanel(m_shell);
    m_death = new DeathPanel(m_shell);
    m_log = new LogPanel(m_shell);
    m_help = new HelpPanel(m_shell);
    m_ach = new AchPanel(m_shell);

    OverlayPanel *all[9] = { m_inv, m_web, m_map, m_shop, m_pause, m_death, m_log, m_help, m_ach };
    for (int i = 0; i < 9; ++i) {
        all[i]->setGame(m_game);
        all[i]->setView(m_view);
        gl->addWidget(all[i], 0, 0);
        connect(all[i], SIGNAL(closed()), this, SLOT(onPanelClosed()));
        connect(all[i], SIGNAL(actionTriggered(int)), this, SLOT(onPanelAction(int)));
    }
    m_shop->hide();
    connect(m_map, SIGNAL(travelRequested(int)), this, SLOT(onTravelRequested(int)));
    connect(m_map, SIGNAL(enterRequested(int)), this, SLOT(onEnterRequested(int)));
    connect(m_pause, SIGNAL(resumeRequested()), this, SLOT(onPanelClosed()));
    connect(m_pause, SIGNAL(saveRequested()), this, SLOT(onSaveGame()));
    connect(m_pause, SIGNAL(loadRequested()), this, SLOT(onLoadGame()));
    connect(m_pause, SIGNAL(quitToMenuRequested()), this, SLOT(onQuitToMenu()));
    connect(m_pause, SIGNAL(toggleFullscreenRequested()), this, SLOT(onToggleFullscreen()));
    connect(m_pause, SIGNAL(soundToggled(bool)), this, SLOT(onPanelClosed()));
    connect(m_pause, SIGNAL(shakeToggled(bool)), this, SLOT(onPanelClosed()));
    connect(m_death, SIGNAL(respawnRequested()), this, SLOT(onRespawn()));
    connect(m_death, SIGNAL(quitToMenuRequested()), this, SLOT(onQuitToMenu()));

    // ---- сигналы сцены ----
    connect(m_view, SIGNAL(uiKeyPressed(int)), this, SLOT(onUiKey(int)));
    connect(m_view, SIGNAL(playerDiedSignal()), this, SLOT(onPlayerDied()));
    connect(m_view, SIGNAL(logMsg(QString, QColor)), this, SLOT(onLog(QString, QColor)));
    connect(m_view, SIGNAL(lootPicked(QString, QColor)), this, SLOT(onLoot(QString, QColor)));
    connect(m_view, SIGNAL(bossEngaged(int, QString)), this, SLOT(onBossEngaged(int, QString)));
    connect(m_view, SIGNAL(locationCleared(int)), this, SLOT(onLocationCleared(int)));
    connect(m_view, SIGNAL(requestExitToMap()), this, SLOT(onExitToMap()));
    connect(m_view, SIGNAL(requestShop()), this, SLOT(onRequestShop()));
    connect(m_view, SIGNAL(requestHeal()), this, SLOT(onRequestHeal()));
    connect(m_view, SIGNAL(requestRespec()), this, SLOT(onRequestRespec()));
    connect(m_view, SIGNAL(requestShrine(int)), this, SLOT(onRequestShrine(int)));
    connect(m_view, SIGNAL(postFxFailed()), this, SLOT(onPostFxFailed()));

    m_stack->setCurrentWidget(m_title);
    connect(&AchTracker::inst(), SIGNAL(unlockedSignal(int)), this, SLOT(onAchUnlocked(int)));
    AchTracker::inst().load();
    Tooltip::instance(this);
}

// ============================================================
//  НАСТРОЙКИ
// ============================================================
void MainWindow::loadSettings()
{
    QSettings s(QStringLiteral("AshAmber"), QStringLiteral("PepeL_i_Yantar"));
    m_sound = s.value(QStringLiteral("sound"), true).toBool();
    m_shake = s.value(QStringLiteral("shake"), true).toBool();
    // по умолчанию игра стартует во весь экран; в настройках можно выключить
    bool full = s.value(QStringLiteral("fullscreen"), true).toBool();
    QRect g = s.value(QStringLiteral("geometry")).toRect();
    if (g.isValid() && !full) setGeometry(g);
    m_pause->setShake(m_shake);
    m_pause->setFullscreen(full);
    m_pixel = qBound(1, s.value(QStringLiteral("pixel"), 1).toInt(), 4);
    m_scan = s.value(QStringLiteral("scanlines"), false).toBool();
    m_gl = s.value(QStringLiteral("glfx"), false).toBool();
    m_sprites = s.value(QStringLiteral("sprites"), true).toBool();
    SprBank::setEnabled(m_sprites);
    TileBank::setEnabled(m_sprites);
    m_lightQ = qBound(0, s.value(QStringLiteral("lightq"), 2).toInt(), 2);
    m_shadowQ = qBound(0, s.value(QStringLiteral("shadowq"), 1).toInt(), 1);
    m_autoQ = s.value(QStringLiteral("autoquality"), true).toBool();
    m_minimap = s.value(QStringLiteral("minimap"), true).toBool();
    m_invSort = qBound(0, s.value(QStringLiteral("invsort"), 0).toInt(), 2);
    m_pause->setPixel(m_pixel);
    m_pause->setScan(m_scan);
    m_pause->setGl(m_gl);
    m_pause->setLightQ(m_lightQ);
    m_pause->setShadowQ(m_shadowQ);
    m_pause->setAutoQ(m_autoQ);
    m_pause->setMinimap(m_minimap);
    m_view->setAutoQuality(m_autoQ);
    m_view->setMinimapVisible(m_minimap);
    if (full) showFullScreen();
}

void MainWindow::saveSettings()
{
    QSettings s(QStringLiteral("AshAmber"), QStringLiteral("PepeL_i_Yantar"));
    s.setValue(QStringLiteral("sound"), m_sound);
    s.setValue(QStringLiteral("shake"), m_shake);
    s.setValue(QStringLiteral("fullscreen"), isFullScreen());
    s.setValue(QStringLiteral("pixel"), m_pixel);
    s.setValue(QStringLiteral("scanlines"), m_scan);
    s.setValue(QStringLiteral("glfx"), m_gl);
    s.setValue(QStringLiteral("sprites"), m_sprites);
    s.setValue(QStringLiteral("lightq"), m_lightQ);
    s.setValue(QStringLiteral("shadowq"), m_shadowQ);
    s.setValue(QStringLiteral("autoquality"), m_autoQ);
    s.setValue(QStringLiteral("minimap"), m_minimap);
    s.setValue(QStringLiteral("invsort"), m_invSort);
    if (!isFullScreen()) s.setValue(QStringLiteral("geometry"), geometry());
}

void MainWindow::applySettings()
{
    Audio::inst().setEnabled(m_sound);
    m_view->setShakeEnabled(m_shake);
    m_view->setPixelMode(m_pixel);
    m_view->setScanlines(m_scan);
    m_view->setPostFx(m_gl);
    m_view->setLightQ(m_lightQ);
    m_view->setShadowQ(m_shadowQ);
    SprBank::setEnabled(m_sprites);
    TileBank::setEnabled(m_sprites);
}

// ============================================================
//  ПАНЕЛИ
// ============================================================
bool MainWindow::anyPanelOpen() const
{
    return m_inv->isVisible() || m_web->isVisible() || m_map->isVisible() || m_shop->isVisible()
            || m_pause->isVisible() || m_death->isVisible() || m_log->isVisible() || m_help->isVisible()
            || m_ach->isVisible();
}

void MainWindow::openPanel(OverlayPanel *panel)
{
    closeAllPanels();
    panel->refresh();
    panel->openPanel();
    m_view->setInteractive(false);
    m_view->setPaused(true);
}

void MainWindow::closeAllPanels()
{
    Tooltip::instance(this)->hideNow();
    OverlayPanel *all[9] = { m_inv, m_web, m_map, m_shop, m_pause, m_death, m_log, m_help, m_ach };
    for (int i = 0; i < 9; ++i) all[i]->hide();
    if (m_playing && !m_view->playerDead()) {
        m_view->setInteractive(true);
        m_view->setPaused(false);
        m_view->setFocus(Qt::OtherFocusReason);   // иначе клавиатура остаётся у скрытой панели
    }
}

void MainWindow::onPanelClosed()
{
    // настройки из панели паузы
    m_sound = Audio::inst().enabled();
    m_shake = m_pause->shakeOn();
    m_view->setShakeEnabled(m_shake);
    m_pixel = m_pause->pixelNow();
    m_scan = m_pause->scanNow();
    m_gl = m_pause->glNow();
    m_view->setPixelMode(m_pixel);
    m_view->setScanlines(m_scan);
    m_view->setPostFx(m_gl);
    closeAllPanels();
    if (m_playing) m_view->setFocus(Qt::OtherFocusReason);
    saveSettings();
}

void MainWindow::onUiKey(int act)
{
    if (!m_playing) {
        if (act == Binds::A_Help) onTitleHelp();
        return;
    }
    switch (act) {
    case Binds::A_Pause:
        if (anyPanelOpen()) { if (m_death->isVisible()) return; closeAllPanels(); }
        else openPanel(m_pause);
        break;
    case Binds::A_Inv:
        if (m_inv->isVisible()) closeAllPanels(); else openPanel(m_inv);
        break;
    case Binds::A_Web:
        if (m_web->isVisible()) closeAllPanels(); else openPanel(m_web);
        break;
    case Binds::A_Map:
        if (m_map->isVisible()) closeAllPanels();
        else { m_map->setSelected(m_game->currentLocation()); openPanel(m_map); }
        break;
    case Binds::A_Log:
        if (m_log->isVisible()) closeAllPanels(); else openPanel(m_log);
        break;
    case Binds::A_Help:
        if (m_help->isVisible()) closeAllPanels(); else openPanel(m_help);
        break;
    case Binds::A_Ach:
        if (m_ach->isVisible()) closeAllPanels(); else openPanel(m_ach);
        break;
    case Binds::A_Save: onSaveGame(); break;
    case Binds::A_Load: onLoadGame(); break;
    default: break;
    }
}

static QString lightQName(int q)
{
    return q == 2 ? QStringLiteral("высокое") : (q == 1 ? QStringLiteral("низкое") : QStringLiteral("выкл"));
}
static QString shadowQName(int q)
{
    return q > 0 ? QStringLiteral("вкл") : QStringLiteral("выкл");
}

void MainWindow::onPanelAction(int id)
{
    switch (id) {
    case 900: // свиток возвращения
        returnToTownFromScroll();
        break;
    case 100: case 101: break;
    case 102: { // снять всё
        for (int i = 0; i < Slot_EquipCount; ++i) m_game->unequipToBag(Slot(i));
        break;
    }
    case 103: { // сортировка сумки - переключает режим по кругу
        m_invSort = (m_invSort + 1) % 3;
        QVector<Item> items;
        for (int i = 0; i < m_game->bagRef().size(); ++i)
            if (m_game->bagRef().at(i).valid) items.append(m_game->bagRef().at(i));
        int mode = m_invSort;
        std::sort(items.begin(), items.end(), [&](const Item &a, const Item &b) {
            const ItemBaseDef *ba = Data::base(a.baseId), *bb = Data::base(b.baseId);
            if (mode == 1) {           // по редкости
                if (a.rarity != b.rarity) return a.rarity > b.rarity;
                return m_game->itemScore(a) > m_game->itemScore(b);
            }
            if (mode == 2) {           // по уровню предмета
                if (a.ilvl != b.ilvl) return a.ilvl > b.ilvl;
                return m_game->itemScore(a) > m_game->itemScore(b);
            }
            int ka = ba ? int(ba->bag) * 100 + ba->slot : 999;   // по типу (умолч.)
            int kb = bb ? int(bb->bag) * 100 + bb->slot : 999;
            if (ka != kb) return ka < kb;
            if (a.rarity != b.rarity) return a.rarity > b.rarity;
            return m_game->itemScore(a) > m_game->itemScore(b);
        });
        QVector<Item> &bag = m_game->bagRef();
        for (int i = 0; i < bag.size(); ++i) bag[i] = (i < items.size()) ? items.at(i) : Item();
        static const char *mn[3] = { "\xD0\xBF\xD0\xBE \xD1\x82\xD0\xB8\xD0\xBF\xD1\x83",
                                     "\xD0\xBF\xD0\xBE \xD1\x80\xD0\xB5\xD0\xB4\xD0\xBA\xD0\xBE\xD1\x81\xD1\x82\xD0\xB8",
                                     "\xD0\xBF\xD0\xBE \xD1\x83\xD1\x80\xD0\xBE\xD0\xB2\xD0\xBD\xD1\x8E" };
        onLog(QStringLiteral("Сумка упорядочена %1.").arg(QString::fromUtf8(mn[mode])), C::boneDim);
        m_inv->refresh();
        saveSettings();
        break;
    }
    case 110: case 111: case 112: case 113:   // фильтры сумки
        m_inv->setFilter(id - 110);
        break;
    case 200: { // переобучение в паутине
        int cost = m_game->respecCost();
        if (m_game->gold() >= cost) {
            m_game->addGold(-cost);
            m_game->respecAll();
            onLog(QStringLiteral("Паутина навыков очищена за %1 золота.").arg(cost), C::arcane);
        } else onLog(QStringLiteral("Не хватает золота на переобучение."), C::bad);
        break;
    }
    case 201: case 202: case 203: case 204: case 205:
        m_web->setFilter(id - 201 - 1);
        break;
    case 206:
        m_web->resetView();
        break;
    case 300: { // отправиться
        int sel = m_map->selected();
        if (sel >= 0) onEnterRequested(sel);
        break;
    }
    case 400: m_shop->setTab(0); break;
    case 401: m_shop->setTab(1); break;
    case 404: {
        int cost = 20 + m_game->level() * 4;
        if (m_game->gold() >= cost) {
            m_game->addGold(-cost);
            m_shop->restock();
            onLog(QStringLiteral("Торговец выставил новый товар (-%1 золота).").arg(cost), C::gold);
        } else onLog(QStringLiteral("Не хватает золота."), C::bad);
        break;
    }
    case 500: closeAllPanels(); break;
    case 501: onSaveGame(); break;
    case 502: onLoadGame(); break;
    case 504: openPanel(m_help); break;
    case 505: onQuitToMenu(); break;
    case 506: close(); break;
    case 507: m_sound = !m_sound; Audio::inst().setEnabled(m_sound); m_pause->refresh(); saveSettings(); break;
    case 508: m_shake = !m_shake; m_view->setShakeEnabled(m_shake); m_pause->refresh(); saveSettings(); break;
    case 517: m_pixel = (m_pixel >= 4) ? 1 : m_pixel + 1; m_view->setPixelMode(m_pixel);
              m_pause->setPixel(m_pixel); m_pause->refresh(); saveSettings(); break;
    case 518: m_scan = !m_scan; m_view->setScanlines(m_scan);
              m_pause->setScan(m_scan); m_pause->refresh(); saveSettings(); break;
    case 519: m_gl = !m_gl; m_view->setPostFx(m_gl);
              m_pause->setGl(m_gl); m_pause->refresh(); saveSettings(); break;
    case 520: m_sprites = !m_sprites; SprBank::setEnabled(m_sprites); TileBank::setEnabled(m_sprites);
              m_view->rebakeGround(); m_pause->refresh(); saveSettings();
              onLog(m_sprites ? QStringLiteral("Спрайт-листы и тайлсеты включены.")
                              : QStringLiteral("Спрайт-листы и тайлсеты выключены - всё рисуется процедурно."), C::boneDim);
              break;
    case 521: m_lightQ = (m_lightQ + 2) % 3;   // высокий -> низкий -> выкл -> высокий
              m_view->setLightQ(m_lightQ); m_pause->setLightQ(m_lightQ); m_pause->refresh(); saveSettings();
              onLog(QStringLiteral("Качество света: %1.").arg(lightQName(m_lightQ)), C::boneDim);
              break;
    case 522: m_shadowQ = m_shadowQ ? 0 : 1;   // вкл <-> выкл
              m_view->setShadowQ(m_shadowQ); m_pause->setShadowQ(m_shadowQ); m_pause->refresh(); saveSettings();
              onLog(QStringLiteral("Тени: %1.").arg(shadowQName(m_shadowQ)), C::boneDim);
              break;
    case 523: m_autoQ = !m_autoQ;              // авто-подстройка качества по FPS
              m_view->setAutoQuality(m_autoQ); m_pause->setAutoQ(m_autoQ); m_pause->refresh(); saveSettings();
              onLog(m_autoQ ? QStringLiteral("Авто-качество включено: при просадке FPS картинка упрощается, при восстановлении - улучшается.")
                            : QStringLiteral("Авто-качество выключено: качество света и теней всегда как в настройках."), C::boneDim);
              break;
    case 524: m_minimap = !m_minimap;          // миникарта в HUD
              m_view->setMinimapVisible(m_minimap); m_pause->setMinimap(m_minimap); m_pause->refresh(); saveSettings();
              onLog(m_minimap ? QStringLiteral("Миникарта показывается.") : QStringLiteral("Миникарта скрыта."), C::boneDim);
              break;
    case 509: onToggleFullscreen(); break;
    case 503: m_pause->setPage(1); m_pause->setSetTab(0); break;
    case 510: m_pause->setPage(1); m_pause->setSetTab(2); break;
    case 513: m_pause->setSetTab(0); break;
    case 514: m_pause->setSetTab(1); break;
    case 515: m_pause->setSetTab(2); break;
    case 516: m_pause->setPage(0); break;
    case 511: Binds::resetAll(); m_pause->setPage(1); onLog(QStringLiteral("Клавиши сброшены на стандартные."), C::boneDim); break;
    case 512: m_pause->setPage(0); break;
    case 600: onRespawn(); break;
    case 601: onQuitToMenu(); break;
    case 700: m_log->clear(); break;
    case 710: m_help->nextPage(); break;
    case 711: m_help->prevPage(); break;
    default: break;
    }
}

// ============================================================
//  ИГРОВОЙ ПОТОК
// ============================================================
void MainWindow::onStartNewGame(QString heroName, int classId)
{
    Audio::inst().initAssets();      // перескан: файлы могли положить после запуска
    m_game->newGame(heroName, classId);
    m_playing = true;
    m_stack->setCurrentWidget(m_shell);
    m_log->clear();
    int start = Data::firstLocation();
    m_game->travelTo(start);
    m_view->loadLocation(start);
    updateMusicForLocation(start);   // меню-трек глушится сразу при старте партии
    m_view->setInteractive(true);
    m_view->setPaused(false);
    m_view->setFocus(Qt::OtherFocusReason);
    onLog(Audio::inst().assetsReport(), C::boneDim);
    {
        QStringList lines = SprBank::reportLines();
        for (int i = 0; i < lines.size(); ++i) onLog(lines.at(i), C::boneDim);
    }
    {
        QStringList lines = TileBank::reportLines();
        for (int i = 0; i < lines.size(); ++i) onLog(lines.at(i), C::boneDim);
    }
    onLog(QStringLiteral("Новая игра начата. Класс: %1").arg(
              classId == 0 ? QStringLiteral("Воин") : classId == 1 ? QStringLiteral("Маг")
              : classId == 2 ? QStringLiteral("Странник") : QStringLiteral("Жрец пепла")), C::amber);
    openPanel(m_help);
}

void MainWindow::onContinueGame()
{
    Audio::inst().initAssets();
    if (!m_game->loadGame()) {
        QMessageBox::warning(this, QStringLiteral("Пепел и Янтарь"), QStringLiteral("Сохранение не найдено или повреждено."));
        return;
    }
    m_playing = true;
    m_stack->setCurrentWidget(m_shell);
    m_log->clear();
    int loc = m_game->currentLocation();
    const LocationDef *L = Data::loc(loc);
    // всегда стартуем в последнем поселении - честнее
    if (!L || L->type != Loc_Town) loc = m_game->lastTown();
    m_view->loadLocation(loc);
    updateMusicForLocation(loc);
    m_view->setInteractive(true);
    m_view->setPaused(false);
    m_view->setFocus(Qt::OtherFocusReason);
}

void MainWindow::onTitleHelp()
{
    m_stack->setCurrentWidget(m_shell);
    m_playing = false;
    m_view->setPaused(true);
    m_view->setInteractive(false);
    openPanel(m_help);
}

void MainWindow::onQuitGame()
{
    saveSettings();
    qApp->quit();
}

void MainWindow::onQuitToMenu()
{
    saveSettings();
    closeAllPanels();
    m_playing = false;
    m_view->setPaused(true);
    m_view->setInteractive(false);
    m_view->clearScene();
    m_title->resetToMain();          // меню, а не экран выбора класса
    Audio::inst().startMusic(1);     // и трек меню
    m_stack->setCurrentWidget(m_title);
}

void MainWindow::updateMusicForLocation(int locId)
{
    const LocationDef *L = Data::loc(locId);
    Audio::inst().startMusic(L && L->type == Loc_Town ? 2 : 0);   // поселение с NPC или боевая тема
}

void MainWindow::enterLocation(int locId)
{
    closeAllPanels();
    m_game->travelTo(locId);
    m_view->loadLocation(locId);
    m_view->setInteractive(true);
    m_view->setPaused(false);
    m_view->setFocus(Qt::OtherFocusReason);
    const LocationDef *L = Data::loc(locId);
    if (L && L->type == Loc_Town) {
        m_shop->setShopTier(L->tier);
        onLog(QStringLiteral("Ты в поселении: %1. F - говорить с жителями.").arg(L->name), C::gold);
    }
    updateMusicForLocation(locId);
}

void MainWindow::onTravelRequested(int locId) { enterLocation(locId); }
void MainWindow::onEnterRequested(int locId)
{
    if (!m_game->isUnlocked(locId)) { onLog(QStringLiteral("Путь ещё не открыт."), C::bad); return; }
    enterLocation(locId);
}

void MainWindow::onExitToMap()
{
    // сохранить прогресс при выходе из локации
    m_game->saveGame();
    m_map->setSelected(m_game->currentLocation());
    openPanel(m_map);
}

void MainWindow::returnToTownFromScroll()
{
    int town = m_game->lastTown();
    m_game->saveGame();
    enterLocation(town);
    onLog(QStringLiteral("Свиток перенёс тебя в %1.").arg(Data::loc(town) ? Data::loc(town)->name : QString()), C::arcane);
}

void MainWindow::onRequestShop()
{
    const LocationDef *L = Data::loc(m_game->currentLocation());
    m_shop->setShopTier(L ? L->tier : 1);
    m_shop->setTab(0);
    openPanel(m_shop);
}

void MainWindow::onRequestHeal()
{
    int cost = m_game->level() * 6;
    if (m_game->gold() >= cost) {
        m_game->addGold(-cost);
        onLog(QStringLiteral("Целительница забрала %1 золота и вернула тебе силы.").arg(cost), C::good);
    } else {
        onLog(QStringLiteral("«Денег не надо. Только не умирай снова». Тебя вылечили."), C::good);
    }
    m_game->addHp(m_game->maxHp());
    m_game->addMana(m_game->maxMana());
    Audio::inst().play(Sfx_Heal);
}

void MainWindow::onRequestRespec()
{
    int cost = m_game->respecCost();
    if (m_game->gold() >= cost) {
        m_game->addGold(-cost);
        m_game->respecAll();
        onLog(QStringLiteral("Кузнец выжег старые узоры паутины за %1 золота. Очков: %2.")
              .arg(cost).arg(m_game->skillPoints()), C::arcane);
        Audio::inst().play(Sfx_Cast);
    } else {
        onLog(QStringLiteral("Переобучение стоит %1 золота. У тебя %2.").arg(cost).arg(m_game->gold()), C::bad);
        Audio::inst().play(Sfx_UiBad);
    }
}

void MainWindow::onRequestShrine(int kind)
{
    ShrineBuff b = ShrineBuff(qBound(1, kind, 5));
    m_game->applyShrine(b, 240.f);
}

void MainWindow::onPlayerDied()
{
    m_death->refresh();
    openPanel(m_death);
    m_view->setInteractive(false);
}

void MainWindow::onRespawn()
{
    m_death->hide();
    Audio::inst().startMusic(2);   // возрождаемся в поселении
    m_view->respawn();
    closeAllPanels();
    m_view->setInteractive(true);
    m_view->setPaused(false);
}

void MainWindow::onLevelUp(int level)
{
    m_hud->notifyLevelUp(level);
    Audio::inst().play(Sfx_LevelUp);
    if (level % 5 == 0) m_game->saveGame();
}

void MainWindow::onLog(QString msg, QColor color)
{
    m_hud->pushLog(msg, color);
    m_log->addLine(msg, color);
}

void MainWindow::onLoot(QString name, QColor color)
{
    m_hud->notifyLoot(name, color);
}

void MainWindow::onBossEngaged(int defId, QString name)
{
    Q_UNUSED(defId);
    onLog(QStringLiteral("Появился %1!").arg(name), C::bad);
    Audio::inst().play(Sfx_Boss);
}

void MainWindow::onLocationCleared(int locId)
{
    Q_UNUSED(locId);
    m_game->saveGame();
}

void MainWindow::onSaveGame()
{
    if (m_game->saveGame()) Audio::inst().play(Sfx_Ui);
}

void MainWindow::onLoadGame()
{
    if (!Game::saveExists()) { onLog(QStringLiteral("Сохранений нет."), C::bad); return; }
    if (m_game->loadGame()) {
        m_playing = true;
        m_stack->setCurrentWidget(m_shell);
        enterLocation(m_game->lastTown());
        onLog(QStringLiteral("Игра загружена."), C::good);
    } else onLog(QStringLiteral("Не удалось загрузить сохранение."), C::bad);
}

void MainWindow::onAchUnlocked(int id)
{
    const Ach::Def &d = Ach::def(id);
    m_hud->notifyAch(QString::fromUtf8(d.name), d.glyph);
    onLog(QStringLiteral("Достижение: %1 - %2").arg(QString::fromUtf8(d.name), QString::fromUtf8(d.desc)), C::amber);
    Audio::inst().play(Sfx_Ach);
}

void MainWindow::onPostFxFailed()
{
    m_gl = false;
    m_pause->setGl(false);
    m_pause->refresh();
    saveSettings();
}

void MainWindow::onToggleFullscreen()
{
    if (isFullScreen()) showNormal();
    else showFullScreen();
    m_pause->setFullscreen(isFullScreen());
    saveSettings();
}

// ============================================================
//  ФИЛЬТР СОБЫТИЙ (тултипы панели умений)
// ============================================================
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_view && event->type() == QEvent::MouseMove) {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        if (!anyPanelOpen()) {
            QPointF pos = me->localPos();
            int slot = m_hud->hotbarSlotAt(pos);
            if (slot >= 0) {
                int sk = m_game->hotbar(slot);
                if (sk >= 0) {
                    Tooltip::instance(this)->showSkill(sk, m_game->skillRank(sk),
                                                       m_view->mapToGlobal(pos.toPoint()));
                    return false;
                }
            } else if (m_hud->orbHpContains(pos) || m_hud->orbMpContains(pos)) {
                const Derived &d = m_game->d();
                bool hp = m_hud->orbHpContains(pos);
                QString title = hp ? QStringLiteral("Здоровье") : QStringLiteral("Мана");
                QStringList body;
                if (hp) {
                    body << QStringLiteral("%1 из %2").arg(int(m_game->hp())).arg(int(d.maxHp));
                    body << QStringLiteral("Восстановление: %1/сек").arg(QString::number(double(d.hpRegen), 'f', 1));
                    body << QStringLiteral("Броня: %1 (%2% снижения)").arg(int(d.armor)).arg(int(d.res[Dmg_Physical] * 100));
                    body << QStringLiteral("Уклонение: %1%").arg(QString::number(double(d.dodge * 100), 'f', 1));
                } else {
                    body << QStringLiteral("%1 из %2").arg(int(m_game->mana())).arg(int(d.maxMana));
                    body << QStringLiteral("Восстановление: %1/сек").arg(QString::number(double(d.manaRegen), 'f', 1));
                    body << QStringLiteral("Скорость заклинаний: %1%").arg(int(d.castSpeed * 100));
                    body << QStringLiteral("Сокращение отката: %1%").arg(int(d.cdReduction * 100));
                }
                Tooltip::instance(this)->showText(title, body.join(QLatin1Char('\n')),
                                                  m_view->mapToGlobal(pos.toPoint()),
                                                  hp ? C::hpHi : C::manaHi);
                return false;
            }
            Tooltip::instance(this)->hideNow();
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::showEvent(QShowEvent *e)
{
    QMainWindow::showEvent(e);
    if (!m_musicStarted) {
        m_musicStarted = true;
        // стартуем уже внутри event loop - иначе медиаплеер может не подхватиться
        QTimer::singleShot(120, this, [this]() {
            if (!m_playing) Audio::inst().startMusic(1);   // только если всё ещё в меню
        });
    }
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if (m_playing) m_game->saveGame();
    saveSettings();
    Audio::inst().shutdown();
    QMainWindow::closeEvent(e);
}

} // namespace aa
