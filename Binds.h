#ifndef AA_BINDS_H
#define AA_BINDS_H

// Привязки клавиш: сохраняются в QSettings, меняются из настроек.
// Учитывает раскладку: на русской раскладке физические клавиши WASD/F/E/Q/G
// порождают кириллические keysym, поэтому для них есть алиасы.

#include <QString>

namespace aa {
namespace Binds {

enum Act {
    A_Up = 0, A_Down, A_Left, A_Right,
    A_Potion, A_Interact, A_Return, A_Shard,
    A_Skill1, A_Skill2, A_Skill3, A_Skill4, A_Skill5, A_Skill6,
    A_Inv, A_Web, A_Map, A_Log, A_Help, A_Pause, A_Save, A_Load, A_Ach,
    A_Count
};

int  key(int act);          // текущая привязка
int  defKey(int act);       // привязка по умолчанию
void setKey(int act, int newKey);
void resetAll();
void load();
void save();

QString actName(int act);
QString keyName(int key);

// нажата ли клавиша, подходящая действию (с учётом алиасов и стрелок для движения)
bool matches(int pressedKey, int act);

// алиас клавиши для русской раскладки (Ц/Ф/Ы/В/А/У/Й/П и т.д.)
int layoutAlias(int key);

} // namespace Binds
} // namespace aa

#endif // AA_BINDS_H
