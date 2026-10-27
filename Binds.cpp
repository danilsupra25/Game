#include "Core/Binds.h"

#include <QSettings>
#include <QKeySequence>
#include <QSet>

namespace aa {
namespace Binds {

static int g_cur[A_Count];
static bool g_loaded = false;

int defKey(int act)
{
    switch (act) {
    case A_Up:      return Qt::Key_W;
    case A_Down:    return Qt::Key_S;
    case A_Left:    return Qt::Key_A;
    case A_Right:   return Qt::Key_D;
    case A_Potion:  return Qt::Key_Space;
    case A_Interact:return Qt::Key_F;
    case A_Return:  return Qt::Key_Q;
    case A_Shard:   return Qt::Key_G;
    case A_Skill1:  return Qt::Key_1;
    case A_Skill2:  return Qt::Key_2;
    case A_Skill3:  return Qt::Key_3;
    case A_Skill4:  return Qt::Key_4;
    case A_Skill5:  return Qt::Key_5;
    case A_Skill6:  return Qt::Key_6;
    case A_Inv:     return Qt::Key_I;
    case A_Web:     return Qt::Key_K;
    case A_Map:     return Qt::Key_M;
    case A_Log:     return Qt::Key_L;
    case A_Help:    return Qt::Key_H;
    case A_Pause:   return Qt::Key_Escape;
    case A_Save:    return Qt::Key_F5;
    case A_Load:    return Qt::Key_F9;
    case A_Ach:     return Qt::Key_J;
    default:        return Qt::Key_unknown;
    }
}

int layoutAlias(int key)
{
    // коды клавиш русской раскладки (Unicode, т.к. отдельных enum'ов у Qt нет)
    // физическая клавиша -> keysym русской раскладки
    switch (key) {
    case Qt::Key_W: return 0x0446;     // Ц
    case Qt::Key_A: return 0x0444;      // Ф
    case Qt::Key_S: return 0x044b;    // Ы
    case Qt::Key_D: return 0x0432;      // В
    case Qt::Key_F: return 0x0430;       // А
    case Qt::Key_E: return 0x0443;       // У
    case Qt::Key_Q: return 0x0439;  // Й
    case Qt::Key_G: return 0x043f;      // П
    case Qt::Key_R: return 0x043a;      // К
    case Qt::Key_C: return 0x0441;      // С
    case Qt::Key_B: return 0x0438;       // И
    // и обратно: если нажата кириллица, а привязана латиница
    case 0x0446:    return Qt::Key_W;
    case 0x0444:     return Qt::Key_A;
    case 0x044b:   return Qt::Key_S;
    case 0x0432:     return Qt::Key_D;
    case 0x0430:      return Qt::Key_F;
    case 0x0443:      return Qt::Key_E;
    case 0x0439: return Qt::Key_Q;
    case 0x043f:     return Qt::Key_G;
    case 0x043a:     return Qt::Key_R;
    case 0x0441:     return Qt::Key_C;
    case 0x0438:      return Qt::Key_B;
    case Qt::Key_I: return 0x0448;    // Ш
    case Qt::Key_K: return 0x043b;    // Л
    case Qt::Key_M: return 0x044c;    // Ь
    case Qt::Key_L: return 0x0434;    // Д
    case Qt::Key_H: return 0x0440;    // Р
    case 0x0448: return Qt::Key_I;
    case 0x043b: return Qt::Key_K;
    case 0x044c: return Qt::Key_M;
    case 0x0434: return Qt::Key_L;
    case 0x0440: return Qt::Key_H;
    case Qt::Key_J: return 0x043e;    // О
    case 0x043e: return Qt::Key_J;
    default: return Qt::Key_unknown;
    }
}

static int arrowFor(int act)
{
    switch (act) {
    case A_Up:    return Qt::Key_Up;
    case A_Down:  return Qt::Key_Down;
    case A_Left:  return Qt::Key_Left;
    case A_Right: return Qt::Key_Right;
    default:      return Qt::Key_unknown;
    }
}

void load()
{
    if (g_loaded) return;
    g_loaded = true;
    for (int i = 0; i < A_Count; ++i) g_cur[i] = defKey(i);
    QSettings s(QStringLiteral("AshAmber"), QStringLiteral("PepeL_i_Yantar"));
    s.beginGroup(QStringLiteral("binds"));
    for (int i = 0; i < A_Count; ++i) {
        int v = s.value(actName(i), defKey(i)).toInt();
        if (v != 0) g_cur[i] = v;
    }
    s.endGroup();
}

void save()
{
    QSettings s(QStringLiteral("AshAmber"), QStringLiteral("PepeL_i_Yantar"));
    s.beginGroup(QStringLiteral("binds"));
    for (int i = 0; i < A_Count; ++i) s.setValue(actName(i), g_cur[i]);
    s.endGroup();
}

void resetAll()
{
    for (int i = 0; i < A_Count; ++i) g_cur[i] = defKey(i);
    save();
}

int key(int act)
{
    load();
    return (act >= 0 && act < A_Count) ? g_cur[act] : Qt::Key_unknown;
}

void setKey(int act, int newKey)
{
    load();
    if (act < 0 || act >= A_Count || newKey == Qt::Key_unknown) return;
    // одна клавиша - одно действие: снимем её с других
    for (int i = 0; i < A_Count; ++i)
        if (i != act && g_cur[i] == newKey) g_cur[i] = Qt::Key_unknown;
    g_cur[act] = newKey;
    save();
}

QString actName(int act)
{
    switch (act) {
    case A_Up:       return QStringLiteral("up");
    case A_Down:     return QStringLiteral("down");
    case A_Left:     return QStringLiteral("left");
    case A_Right:    return QStringLiteral("right");
    case A_Potion:   return QStringLiteral("potion");
    case A_Interact: return QStringLiteral("interact");
    case A_Return:   return QStringLiteral("return");
    case A_Shard:    return QStringLiteral("shard");
    case A_Skill1:   return QStringLiteral("skill1");
    case A_Skill2:   return QStringLiteral("skill2");
    case A_Skill3:   return QStringLiteral("skill3");
    case A_Skill4:   return QStringLiteral("skill4");
    case A_Skill5:   return QStringLiteral("skill5");
    case A_Skill6:   return QStringLiteral("skill6");
    case A_Inv:      return QStringLiteral("inventory");
    case A_Web:      return QStringLiteral("skillweb");
    case A_Map:      return QStringLiteral("worldmap");
    case A_Log:      return QStringLiteral("log");
    case A_Help:     return QStringLiteral("help");
    case A_Pause:    return QStringLiteral("pause");
    case A_Save:     return QStringLiteral("save");
    case A_Load:     return QStringLiteral("load");
    case A_Ach:      return QStringLiteral("achievements");
    default:         return QStringLiteral("?");
    }
}

QString keyName(int key)
{
    if (key == Qt::Key_unknown || key == 0) return QStringLiteral("—");
    QString s = QKeySequence(key).toString();
    if (s.isEmpty()) s = QStringLiteral("?");
    return s;
}

bool matches(int pressedKey, int act)
{
    load();
    if (act < 0 || act >= A_Count) return false;
    if (pressedKey == g_cur[act]) return true;
    if (pressedKey == defKey(act)) return true;
    if (arrowFor(act) != Qt::Key_unknown && pressedKey == arrowFor(act)) return true;
    int al = layoutAlias(pressedKey);
    if (al != Qt::Key_unknown && (al == g_cur[act] || al == defKey(act))) return true;
    return false;
}

} // namespace Binds
} // namespace aa
