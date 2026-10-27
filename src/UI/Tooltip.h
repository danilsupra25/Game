#ifndef AA_TOOLTIP_H
#define AA_TOOLTIP_H

#include <QWidget>
#include <QString>
#include <QColor>
#include <QPoint>
#include <QVector>
#include "Core/Types.h"

namespace aa {

class Game;

class Tooltip : public QWidget
{
    Q_OBJECT
public:
    static Tooltip *instance(QWidget *parentWidget = 0);

    void showItem(const Item &it, const Game *game, const QPoint &globalPos, bool inShop = false, int price = -1);
    void showSkill(int skillId, int rank, const QPoint &globalPos);
    void showNode(int nodeId, int rank, const Game *game, const QPoint &globalPos);
    void showText(const QString &title, const QString &body, const QPoint &globalPos,
                  const QColor &titleColor = QColor());
    void hideNow();

protected:
    void paintEvent(QPaintEvent *);

private:
    explicit Tooltip(QWidget *parent = 0);
    void place(const QPoint &globalPos);

    struct Line { QString text; QColor color; QFont font; int align; float spaceBefore; };
    QVector<Line> m_lines;
    QColor m_border;
    float m_glow = 0.f;
    int m_glyph = 0;
    QColor m_glyphColor;
};

} // namespace aa

#endif // AA_TOOLTIP_H
