#ifndef AA_GLFX_H
#define AA_GLFX_H

// Опциональный слой постобработки на QOpenGLWidget (Qt 5.9):
// CRT-искривление, bloom, сканлайны, хроматическая аберрация, палитровый дайтеринг, виньетка.
// Сцена рендерится виджетом в QImage, отсюда уходит в текстуру и проходит один шейдер-проход.
// Если модуля opengl или драйвера нет - слой просто не создаётся, игра работает как раньше.

#include <QWidget>

#ifdef HAVE_GL
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

#ifdef __GLEW_H__
#endif

namespace aa {

class GameView;

class GlFxView : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit GlFxView(GameView *src, QWidget *parent = 0);
    bool ok() const { return m_ok; }

signals:
    void failed();

protected:
    void initializeGL();
    void paintGL();

private:
    GameView *m_src;
    bool m_ok;
    QOpenGLShaderProgram *m_prog;
    GLuint m_tex;
    int m_texW, m_texH;
    float m_time;
};

} // namespace aa

#endif // HAVE_GL
#endif // AA_GLFX_H
