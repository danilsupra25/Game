#include "Game/GlFx.h"

#ifdef HAVE_GL

#include "Game/GameView.h"
#include "World/Dungeon.h"
#include "Core/Style.h"

#include <QImage>
#include <QVector2D>
#include <QOpenGLContext>

namespace aa {

static const char *VERT_SRC =
    "attribute vec2 a_pos;\n"
    "varying vec2 v_uv;\n"
    "void main()\n"
    "{\n"
    "    v_uv = a_pos;\n"
    "    vec2 p = a_pos * 2.0 - 1.0;\n"
    "    gl_Position = vec4(p.x, -p.y, 0.0, 1.0);\n"
    "}\n";

static const char *FRAG_SRC =
    "varying vec2 v_uv;\n"
    "uniform sampler2D u_tex;\n"
    "uniform vec2 u_res;\n"
    "uniform float u_time;\n"
    "uniform float u_curve;\n"
    "uniform float u_scan;\n"
    "uniform float u_bloom;\n"
    "uniform float u_dither;\n"
    "uniform float u_aber;\n"
    "uniform vec3 u_tint;\n"
    "uniform float u_sat;\n"
    "uniform float u_con;\n"
    "uniform float u_grain;\n"
    "vec2 warpUv(vec2 uv)\n"
    "{\n"
    "    vec2 c = uv * 2.0 - 1.0;\n"
    "    float r2 = dot(c, c);\n"
    "    c = c * (1.0 + u_curve * 0.055 * r2);\n"
    "    return c * 0.5 + 0.5;\n"
    "}\n"
    "float bayer2(vec2 p)\n"
    "{\n"
    "    vec2 q = floor(mod(p, 2.0));\n"
    "    return (q.x + 2.0 * q.y) * 0.25;\n"
    "}\n"
    "void main()\n"
    "{\n"
    "    vec2 uv = warpUv(v_uv);\n"
    "    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)\n"
    "    {\n"
    "        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
    "        return;\n"
    "    }\n"
    "    float ca = u_aber / u_res.x;\n"
    "    vec3 col;\n"
    "    col.r = texture2D(u_tex, uv + vec2(ca, 0.0)).r;\n"
    "    col.g = texture2D(u_tex, uv).g;\n"
    "    col.b = texture2D(u_tex, uv - vec2(ca, 0.0)).b;\n"
    "    vec3 bl = vec3(0.0, 0.0, 0.0);\n"
    "    for (int i = 0; i < 16; i++)\n"
    "    {\n"
    "        float ang = float(i) * 0.3926991;\n"
    "        float rad = (1.0 + mod(float(i), 4.0)) * (2.2 / u_res.y);\n"
    "        vec2 off = vec2(cos(ang), sin(ang)) * rad;\n"
    "        vec3 samp = texture2D(u_tex, uv + off).rgb;\n"
    "        float lum = dot(samp, vec3(0.3333, 0.3333, 0.3333));\n"
    "        bl = bl + samp * smoothstep(0.50, 0.92, lum);\n"
    "    }\n"
    "    col = col + (bl / 16.0) * u_bloom;\n"
    "    float sl = 0.5 + 0.5 * sin(uv.y * u_res.y * 3.14159);\n"
    "    col = col * (1.0 - u_scan * 0.30 * (1.0 - sl));\n"
    "    float grille = sin(uv.x * u_res.x * 3.14159);\n"
    "    col.r = col.r * (1.0 + u_scan * 0.05 * grille);\n"
    "    col.b = col.b * (1.0 - u_scan * 0.05 * grille);\n"
    "    if (u_dither > 0.5)\n"
    "    {\n"
    "        float lv = 28.0;\n"
    "        float n = bayer2(gl_FragCoord.xy) - 0.375;\n"
    "        vec3 q = floor(col * lv + n) / lv;\n"
    "        col = mix(col, q, 0.75);\n"
    "    }\n"
    "    vec2 cc = uv * 2.0 - 1.0;\n"
    "    col = col * (1.0 - 0.30 * dot(cc, cc) * 0.5);\n"
    "    col = col * (1.0 + 0.012 * sin(u_time * 60.0));\n"
    "    float lum2 = dot(col, vec3(0.299, 0.587, 0.114));\n"
    "    col = mix(vec3(lum2), col, u_sat);\n"
    "    col = (col - 0.5) * u_con + 0.5;\n"
    "    col = col * u_tint;\n"
    "    float gr = fract(sin(dot(gl_FragCoord.xy + u_time * 37.0, vec2(12.9898, 78.233))) * 43758.5453);\n"
    "    col = col + (gr - 0.5) * u_grain;\n"
    "    gl_FragColor = vec4(col, 1.0);\n"
    "}\n";

GlFxView::GlFxView(GameView *src, QWidget *parent)
    : QOpenGLWidget(parent), m_src(src), m_ok(false), m_prog(0), m_tex(0), m_texW(0), m_texH(0), m_time(0.f)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setFocusPolicy(Qt::NoFocus);
}

void GlFxView::initializeGL()
{
    // в Qt 5.9 initializeOpenGLFunctions() возвращает void, поэтому проверяем контекст руками
    initializeOpenGLFunctions();
    if (!context() || !context()->isValid()) { m_ok = false; emit failed(); return; }
    m_prog = new QOpenGLShaderProgram(this);
    bool ok = m_prog->addShaderFromSourceCode(QOpenGLShader::Vertex, VERT_SRC);
    if (!ok) qWarning("GL vertex shader: %s", qPrintable(m_prog->log()));
    ok = ok && m_prog->addShaderFromSourceCode(QOpenGLShader::Fragment, FRAG_SRC);
    if (!ok) qWarning("GL fragment shader: %s", qPrintable(m_prog->log()));
    ok = ok && m_prog->link();
    if (!ok) qWarning("GL link: %s", qPrintable(m_prog->log()));
    if (!ok) {
        delete m_prog;
        m_prog = 0;
        m_ok = false;
        emit failed();
        return;
    }
    glGenTextures(1, &m_tex);
    glBindTexture(GL_TEXTURE_2D, m_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_ok = true;
}

void GlFxView::paintGL()
{
    glViewport(0, 0, width(), height());
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (!m_ok || !m_prog) return;

    QImage img = m_src->sceneImage();
    if (img.isNull()) return;
    QImage rgba = img.convertToFormat(QImage::Format_RGBA8888);

    glBindTexture(GL_TEXTURE_2D, m_tex);
    if (m_texW != rgba.width() || m_texH != rgba.height()) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba.width(), rgba.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     rgba.constBits());
        m_texW = rgba.width();
        m_texH = rgba.height();
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, rgba.width(), rgba.height(), GL_RGBA, GL_UNSIGNED_BYTE,
                        rgba.constBits());
    }

    m_prog->bind();

    // полноэкранный quad
    static const GLfloat quad[] = { 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f };
    int loc = m_prog->attributeLocation("a_pos");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, quad);

    m_prog->setUniformValue("u_tex", 0);
    m_prog->setUniformValue("u_res", QVector2D(float(width()), float(height())));
    m_time += 0.016f;
    m_prog->setUniformValue("u_time", m_time);
    m_prog->setUniformValue("u_curve", 1.0f);
    m_prog->setUniformValue("u_scan", 0.55f);
    m_prog->setUniformValue("u_bloom", 0.55f);
    m_prog->setUniformValue("u_dither", 1.0f);
    m_prog->setUniformValue("u_aber", 1.1f);
    // цветокор по биому
    Biome bio = m_src->dungeon().biome;
    QVector3D tint(1.f, 1.f, 1.f);
    float sat = 1.06f, con = 1.06f, grain = 0.035f;
    switch (bio) {
    case Bio_Ash:    tint = QVector3D(1.04f, 0.98f, 0.92f); sat = 0.94f; con = 1.08f; grain = 0.05f; break;
    case Bio_Ember:  tint = QVector3D(0.98f, 1.05f, 0.92f); sat = 1.10f; break;
    case Bio_Frost:  tint = QVector3D(0.92f, 1.00f, 1.12f); sat = 0.90f; con = 1.04f; break;
    case Bio_Crystal:tint = QVector3D(1.02f, 0.94f, 1.12f); sat = 1.12f; break;
    case Bio_Swamp:  tint = QVector3D(0.94f, 1.06f, 0.90f); sat = 1.05f; grain = 0.045f; break;
    case Bio_Ruins:  tint = QVector3D(1.06f, 1.00f, 0.88f); sat = 0.92f; break;
    case Bio_Lava:   tint = QVector3D(1.12f, 0.94f, 0.86f); sat = 1.15f; con = 1.10f; break;
    case Bio_Void:   tint = QVector3D(1.00f, 0.90f, 1.14f); sat = 1.08f; grain = 0.06f; break;
    case Bio_Town:   tint = QVector3D(1.06f, 0.99f, 0.90f); sat = 1.02f; con = 1.02f; break;
    default: break;
    }
    m_prog->setUniformValue("u_tint", tint);
    m_prog->setUniformValue("u_sat", sat);
    m_prog->setUniformValue("u_con", con);
    m_prog->setUniformValue("u_grain", grain);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_tex);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(loc);
    m_prog->release();
}

} // namespace aa

#endif // HAVE_GL
