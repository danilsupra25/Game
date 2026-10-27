#ifndef AA_MAINWINDOW_H
#define AA_MAINWINDOW_H

#include <QMainWindow>
#include "Core/Types.h"

class QStackedWidget;

namespace aa {

class Game;
class GameView;
class Hud;
class OverlayPanel;
class InventoryPanel;
class SkillWebPanel;
class WorldMapPanel;
class ShopPanel;
class PausePanel;
class DeathPanel;
class LogPanel;
class HelpPanel;
class AchPanel;
class TitleScreen;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();
    ~MainWindow();

protected:
    bool eventFilter(QObject *watched, QEvent *event);
    void closeEvent(QCloseEvent *);
    void showEvent(QShowEvent *);

private slots:
    void onStartNewGame(QString heroName, int classId);
    void onContinueGame();
    void onQuitGame();
    void onUiKey(int key);
    void onPanelAction(int id);
    void onTravelRequested(int locId);
    void onEnterRequested(int locId);
    void onPanelClosed();
    void onPlayerDied();
    void onLevelUp(int level);
    void onLog(QString msg, QColor color);
    void onLoot(QString name, QColor color);
    void onBossEngaged(int defId, QString name);
    void onLocationCleared(int locId);
    void onRequestShop();
    void onRequestHeal();
    void onRequestRespec();
    void onRequestShrine(int kind);
    void onExitToMap();
    void onSaveGame();
    void onLoadGame();
    void onQuitToMenu();
    void onRespawn();
    void onToggleFullscreen();
    void onAchUnlocked(int id);
    void onPostFxFailed();
    void onTitleHelp();
    void returnToTownFromScroll();

private:
    void buildUi();
    void openPanel(OverlayPanel *panel);
    void closeAllPanels();
    bool anyPanelOpen() const;
    void enterLocation(int locId);
    void updateMusicForLocation(int locId);
    void loadSettings();
    void saveSettings();
    void applySettings();

    Game *m_game;
    QStackedWidget *m_stack;
    TitleScreen *m_title;
    QWidget *m_shell;
    GameView *m_view;
    Hud *m_hud;

    InventoryPanel *m_inv;
    SkillWebPanel *m_web;
    WorldMapPanel *m_map;
    ShopPanel *m_shop;
    PausePanel *m_pause;
    DeathPanel *m_death;
    LogPanel *m_log;
    HelpPanel *m_help;
    AchPanel *m_ach;

    bool m_sound;
    bool m_shake;
    bool m_playing;
    int m_pixel;
    bool m_scan;
    bool m_gl;
    bool m_sprites;
    int m_lightQ;        // 0 выкл, 1 низкий, 2 высокий (настройка света)
    int m_shadowQ;       // 0 выкл, 1 простые, 2 полные (настройка теней)
    bool m_autoQ = true;   // авто-подстройка качества по FPS
    bool m_minimap = true; // показ миникарты в HUD
    int m_invSort;       // режим сортировки сумки: 0 тип, 1 редкость, 2 уровень
    bool m_musicStarted;
};

} // namespace aa

#endif // AA_MAINWINDOW_H
