#include "Core/Style.h"

#include <QHash>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QFontDatabase>
#include <QPolygonF>
#include <QStaticText>
#include <QtMath>
#include "Core/Rng.h"

namespace aa {

// ============================================================
//  ШРИФТЫ
// ============================================================
static QStringList serifFamilies()
{
    return QStringList() << QStringLiteral("PT Serif Caption") << QStringLiteral("Georgia")
                           << QStringLiteral("Palatino Linotype") << QStringLiteral("Times New Roman")
                           << QStringLiteral("DejaVu Serif") << QStringLiteral("Serif");
}
static QStringList sansFamilies()
{
    return QStringList() << QStringLiteral("PT Sans Narrow") << QStringLiteral("DejaVu Sans Condensed")
                           << QStringLiteral("Verdana") << QStringLiteral("DejaVu Sans")
                           << QStringLiteral("Arial") << QStringLiteral("Sans Serif");
}

static QFont makeFont(const QStringList &fams, int pt, bool bold, int spacing = 0, bool caps = false)
{
    QFont f;
    QStringList avail = QFontDatabase().families();
    for (int i = 0; i < fams.size(); ++i) {
        if (avail.contains(fams.at(i))) { f.setFamily(fams.at(i)); break; }
        if (i == fams.size() - 1) f.setFamily(fams.at(i));
    }
    f.setPointSize(pt);
    f.setBold(bold);
    f.setLetterSpacing(QFont::AbsoluteSpacing, spacing);
    if (caps) f.setCapitalization(QFont::AllUppercase);
    f.setStyleStrategy(QFont::PreferAntialias);
    return f;
}

QFont fontTitle(int pt, bool bold) { return makeFont(serifFamilies(), pt, bold, 2); }
QFont fontHead (int pt)            { return makeFont(serifFamilies(), pt, true, 1); }
QFont fontBody (int pt)            { return makeFont(sansFamilies(), pt, false); }
QFont fontSmall(int pt)            { return makeFont(sansFamilies(), pt, false); }
QFont fontNum  (int pt, bool bold) { return makeFont(sansFamilies(), pt, bold, 0); }

// ============================================================
//  УТИЛИТЫ
// ============================================================
QString fmtNum(float v, int prec)
{
    return QString::number(double(v), 'f', prec);
}
QString fmtPct(float v, int prec)
{
    QString s = QString::number(double(v * 100.0f), 'f', prec) + QLatin1Char('%');
    if (v > 0) s.prepend(QLatin1Char('+'));
    return s;
}
QString romanNum(int n)
{
    static const int vals[] = {1000,900,500,400,100,90,50,40,10,9,5,4,1};
    static const char *sym[] = {"M","CM","D","CD","C","XC","L","XL","X","IX","V","IV","I"};
    QString out;
    for (int i = 0; i < 13 && n > 0; ++i) {
        while (n >= vals[i]) { out += QLatin1String(sym[i]); n -= vals[i]; }
    }
    return out.isEmpty() ? QStringLiteral("0") : out;
}

QPolygonF starPolygon(const QPointF &c, float rOut, float rIn, int points, float rot)
{
    QPolygonF poly;
    int n = qMax(2, points) * 2;
    for (int i = 0; i < n; ++i) {
        float r = (i % 2 == 0) ? rOut : rIn;
        float a = rot + float(i) * float(M_PI) / float(points);
        poly << QPointF(c.x() + qCos(a) * r, c.y() + qSin(a) * r);
    }
    return poly;
}

QPainterPath runeRing(const QPointF &c, float r, int teeth, float depth, float rot)
{
    QPainterPath path;
    int n = qMax(3, teeth) * 2;
    for (int i = 0; i <= n; ++i) {
        float rr = (i % 2 == 0) ? r : r - depth;
        float a = rot + float(i) * 2.0f * float(M_PI) / float(n);
        QPointF pt(c.x() + qCos(a) * rr, c.y() + qSin(a) * rr);
        if (i == 0) path.moveTo(pt); else path.lineTo(pt);
    }
    path.closeSubpath();
    return path;
}

// ============================================================
//  ПАНЕЛИ / РАМКИ
// ============================================================
namespace Style {

QImage noiseTile(int seed, const QColor &base, float amount)
{
    static QHash<quint64, QImage> cache;
    quint64 key = (quint64(seed) * 2654435761u) ^ quint64(base.rgba()) ^ quint64(amount * 997.f);
    QHash<quint64, QImage>::const_iterator it = cache.constFind(key);
    if (it != cache.constEnd()) return it.value();
    if (cache.size() > 24) cache.clear();

    const int S = 96;
    QImage img(S, S, QImage::Format_ARGB32);
    img.fill(base);
    Rng r(quint64(seed) * 2246822519u + 3719u);
    // точечный шум кистью - быстро
    QPainter pp(&img);
    pp.setPen(Qt::NoPen);
    for (int i = 0; i < 900; ++i) {
        int d = int(r.gauss() * amount * 60.f);
        QColor c(base.red() + d, base.green() + d, base.blue() + d);
        c.setAlpha(90);
        pp.setBrush(c);
        pp.drawRect(int(r.unit() * S), int(r.unit() * S), 1 + int(r.unit() * 2), 1 + int(r.unit() * 2));
    }
    for (int i = 0; i < 10; ++i) {
        float cx = r.unit() * S, cy = r.unit() * S, rad = 8.f + r.unit() * 26.f;
        QRadialGradient g(QPointF(cx, cy), rad);
        QColor cc = (r.chance(0.5f)) ? base.lighter(135) : base.darker(150);
        cc.setAlpha(int(10 + r.unit() * 16));
        g.setColorAt(0, cc);
        cc.setAlpha(0);
        g.setColorAt(1, cc);
        pp.setBrush(g);
        pp.setPen(Qt::NoPen);
        pp.drawRect(QRectF(cx - rad, cy - rad, rad * 2, rad * 2));
    }
    pp.end();
    cache.insert(key, img);
    return img;
}

void panel(QPainter *p, const QRectF &r, int style, float alpha)
{
    QColor base, hi, lo, ed;
    if (style == 2) { base = C::parchment; hi = C::parchHi; lo = QColor(38, 30, 22); ed = C::edge; }
    else if (style == 1) { base = C::panelLo; hi = C::panel; lo = C::ink; ed = C::edge; }
    else { base = C::panel; hi = C::panelHi; lo = C::panelLo; ed = C::edge; }

    QPainterPath path;
    float cut = qMin(10.0, r.width() * 0.06);
    path.moveTo(r.left() + cut, r.top());
    path.lineTo(r.right() - cut, r.top());
    path.lineTo(r.right(), r.top() + cut);
    path.lineTo(r.right(), r.bottom() - cut);
    path.lineTo(r.right() - cut, r.bottom());
    path.lineTo(r.left() + cut, r.bottom());
    path.lineTo(r.left(), r.bottom() - cut);
    path.lineTo(r.left(), r.top() + cut);
    path.closeSubpath();

    // Тело панели (тень, градиент, шум, блик, рама) не меняется от кадра к кадру -
    // кэшируем готовую картинку по размеру/стилю/прозрачности.
    static QHash<quint64, QImage> cache;
    const int iw = int(qCeil(r.width())), ih = int(qCeil(r.height()));
    const int pad = 6;
    const quint64 key = (quint64(iw) << 44) ^ (quint64(ih) << 26) ^ (quint64(style) << 22)
                      ^ quint64(int(alpha * 100.f));
    QImage body = cache.value(key);
    if (body.isNull()) {
        if (cache.size() > 24) cache.clear();
        body = QImage(iw + pad * 2, ih + pad * 2, QImage::Format_ARGB32_Premultiplied);
        body.fill(Qt::transparent);
        QPainter bp(&body);
        bp.setRenderHint(QPainter::Antialiasing, true);
        bp.translate(pad - r.left(), pad - r.top());
        bp.setOpacity(alpha);

        // тень
        QPainterPath sh = path.translated(0, 3);
        bp.fillPath(sh, QColor(0, 0, 0, 120));

        // основа
        QLinearGradient g(r.topLeft(), r.bottomRight());
        g.setColorAt(0.0, hi);
        g.setColorAt(0.45, base);
        g.setColorAt(1.0, lo);
        bp.fillPath(path, QBrush(g));

        // текстура (тайлится кистью - без генерации под каждый размер)
        QImage tile = noiseTile(style * 37 + 11, base, style == 2 ? 0.20f : 0.10f);
        bp.save();
        bp.setClipPath(path);
        bp.setOpacity(0.30 * alpha);
        bp.setPen(Qt::NoPen);
        bp.setBrush(QBrush(tile));
        bp.drawRect(r);
        bp.restore();
        bp.setOpacity(alpha);

        // верхний блик
        bp.save();
        bp.setClipPath(path);
        QLinearGradient gl(r.topLeft(), QPointF(r.left(), r.top() + 26));
        QColor wh(255, 255, 255, 26);
        gl.setColorAt(0, wh);
        wh.setAlpha(0);
        gl.setColorAt(1, wh);
        bp.fillRect(QRectF(r.left(), r.top(), r.width(), 26), gl);
        bp.restore();

        // рама
        bp.setPen(QPen(ed, 2.0));
        bp.setBrush(Qt::NoBrush);
        bp.drawPath(path);
        bp.setPen(QPen(C::amber.darker(200), 1.0));
        bp.drawRect(r.adjusted(3.5, 3.5, -3.5, -3.5));
        bp.end();
        cache.insert(key, body);
    }
    p->save();
    p->setRenderHint(QPainter::SmoothPixmapTransform, false);
    p->drawImage(QPointF(r.left() - pad, r.top() - pad), body);
    p->restore();
}

void corners(QPainter *p, const QRectF &r, const QColor &c, float s)
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(QPen(c.darker(150), 1));
    p->setBrush(c);
    const QPointF pts[4] = { r.topLeft(), r.topRight(), r.bottomLeft(), r.bottomRight() };
    for (int i = 0; i < 4; ++i) {
        QPainterPath d;
        QPointF o = pts[i];
        d.moveTo(o.x() - s * 0.5f, o.y());
        d.lineTo(o.x(), o.y() - s * 0.5f);
        d.lineTo(o.x() + s * 0.5f, o.y());
        d.lineTo(o.x(), o.y() + s * 0.5f);
        d.closeSubpath();
        p->drawPath(d.translated(0, 0));
    }
    p->restore();
}

void frame(QPainter *p, const QRectF &r, const QColor &c, float w)
{
    p->save();
    p->setPen(QPen(c, w));
    p->setBrush(Qt::NoBrush);
    p->drawRect(r.adjusted(w * 0.5, w * 0.5, -w * 0.5, -w * 0.5));
    p->restore();
}

void button(QPainter *p, const QRectF &r, const QString &text, bool hover, bool down, bool enabled, int glyph,
            float hoverK, bool active)
{
    float hk = hoverK >= 0.f ? clampf(hoverK, 0.f, 1.f) : (hover ? 1.f : 0.f);
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    QColor base = down ? C::panelLo : C::panel;
    if (!enabled) base = QColor(30, 28, 36);
    QPainterPath path;
    float cut = 7.0;
    path.moveTo(r.left() + cut, r.top());
    path.lineTo(r.right() - cut, r.top());
    path.lineTo(r.right(), r.top() + cut);
    path.lineTo(r.right(), r.bottom() - cut);
    path.lineTo(r.right() - cut, r.bottom());
    path.lineTo(r.left() + cut, r.bottom());
    path.lineTo(r.left(), r.bottom() - cut);
    path.lineTo(r.left(), r.top() + cut);
    path.closeSubpath();

    // свечение при наведении
    if (hk > 0.05f && enabled)
        glow(p, r.center(), r.width() * 0.62f, C::amber, 0.16f * hk);

    QLinearGradient g(r.topLeft(), r.bottomLeft());
    g.setColorAt(0, base.lighter(135 + int(30 * hk)));
    g.setColorAt(0.5, base.lighter(100 + int(28 * hk)));
    g.setColorAt(1, base.darker(150 - int(20 * hk)));
    p->fillPath(path, g);

    // янтарная заливка наведения / активного состояния
    if (enabled && (hk > 0.01f || active)) {
        QColor hc = C::amber;
        hc.setAlphaF(0.06f + 0.16f * hk + (active ? 0.14f : 0.f));
        p->fillPath(path, hc);
    }
    // рамка
    QColor edge = active ? C::amber : (hk > 0.02f ? C::edgeHi : C::edge);
    p->setPen(QPen(enabled ? edge : C::edge.darker(170), active ? 2.2f : (1.4f + 0.8f * hk)));
    p->setBrush(Qt::NoBrush);
    p->drawPath(path);
    if (active) {
        QTransform t;
        t.translate(r.center().x(), r.center().y());
        t.scale(0.93, 0.82);
        t.translate(-r.center().x(), -r.center().y());
        p->setPen(QPen(C::amber.darker(150), 1.0));
        p->drawPath(t.map(path));
    }

    QColor tc = !enabled ? C::ash.darker(140)
              : (active ? C::amber.lighter(125) : (hk > 0.4f ? C::amber : C::bone));
    QFont f = fontHead(int(qMin(15.0, r.height() * 0.42)));
    QRectF tr = r;
    if (down) tr.translate(0, 1);
    if (glyph != Gl_None) {
        float sz = r.height() * 0.52f;
        QRectF gr(r.left() + sz * 0.55f, r.center().y() - sz * 0.5f + (down ? 1 : 0), sz, sz);
        drawGlyph(p, glyph, gr, tc, tc, (enabled ? 0.25f : 0.f) + 0.5f * hk + (active ? 0.3f : 0.f));
        tr.setLeft(gr.right() + 2);
    }
    textShadow(p, tr, text, tc, f, Qt::AlignCenter);
    p->restore();
}

void bar(QPainter *p, const QRectF &r, float t, const QColor &fill, const QColor &edgec, bool vertical, float glowAmt)
{
    t = clampf(t, 0.f, 1.f);
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(QPen(edgec.darker(140), 1.4));
    p->setBrush(C::ink);
    p->drawRoundedRect(r, 3, 3);

    QRectF fr = r.adjusted(2, 2, -2, -2);
    QPainterPath clip;
    clip.addRoundedRect(fr, 2, 2);
    p->setClipPath(clip);
    QRectF fillR = vertical ? QRectF(fr.left(), fr.bottom() - fr.height() * t, fr.width(), fr.height() * t)
                            : QRectF(fr.left(), fr.top(), fr.width() * t, fr.height());
    QLinearGradient g(vertical ? fillR.bottomLeft() : fillR.topLeft(),
                      vertical ? fillR.topLeft() : fillR.bottomLeft());
    g.setColorAt(0, fill.lighter(150));
    g.setColorAt(0.45, fill);
    g.setColorAt(1, fill.darker(170));
    p->fillRect(fillR, g);
    // блик
    QColor wh(255, 255, 255, 46);
    QRectF hl = fillR;
    if (vertical) { hl.setWidth(hl.width() * 0.3f); }
    else { hl.setHeight(hl.height() * 0.34f); }
    p->fillRect(hl, wh);
    if (glowAmt > 0.f) {
        QColor gc = fill; gc.setAlpha(int(90 * glowAmt));
        p->fillRect(fillR, gc);
    }
    p->restore();
}

// статичные слои сферы (низ: колба+фон, верх: стекло+рама+заклёпки) - кэш по размеру/цветам
static void orbLayers(int sz, const QColor &c1, const QColor &c2, QImage &bottom, QImage &top)
{
    static QHash<quint64, QPair<QImage, QImage> > cache;
    const quint64 key = (quint64(sz) << 48) ^ quint64(c1.rgba()) ^ (quint64(c2.rgba()) << 1);
    QHash<quint64, QPair<QImage, QImage> >::const_iterator it = cache.constFind(key);
    if (it != cache.constEnd()) { bottom = it.value().first; top = it.value().second; return; }
    if (cache.size() > 8) cache.clear();

    const int pad = 4;
    const int S = sz + pad * 2;
    const QPointF c(S * 0.5, S * 0.5);
    const float R = sz * 0.5f;

    bottom = QImage(S, S, QImage::Format_ARGB32_Premultiplied);
    bottom.fill(Qt::transparent);
    {
        QPainter p(&bottom);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(QPen(C::edge.darker(130), 3.0));
        p.setBrush(C::ink);
        p.drawEllipse(c, R, R);
        QRadialGradient bg(c, R);
        bg.setColorAt(0, QColor(40, 34, 46));
        bg.setColorAt(1, QColor(14, 12, 20));
        QPainterPath clip;
        clip.addEllipse(c, R - 3, R - 3);
        p.setClipPath(clip);
        p.fillRect(QRectF(c.x() - R, c.y() - R, sz, sz), bg);
    }
    top = QImage(S, S, QImage::Format_ARGB32_Premultiplied);
    top.fill(Qt::transparent);
    {
        QPainter p(&top);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPainterPath clip;
        clip.addEllipse(c, R - 3, R - 3);
        p.setClipPath(clip);
        QRadialGradient hl(QPointF(c.x() - R * 0.35f, c.y() - R * 0.45f), R * 0.75f);
        hl.setColorAt(0, QColor(255, 255, 255, 105));
        hl.setColorAt(0.5, QColor(255, 255, 255, 22));
        hl.setColorAt(1, QColor(255, 255, 255, 0));
        p.fillRect(QRectF(c.x() - R, c.y() - R, sz, sz), hl);
        p.setClipping(false);
        p.setPen(QPen(C::edgeHi.darker(150), 4.0));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(c, R - 1, R - 1);
        p.setPen(QPen(C::edge, 1.6));
        p.drawEllipse(c, R - 4, R - 4);
        p.setPen(Qt::NoPen);
        p.setBrush(C::edgeHi.darker(120));
        for (int i = 0; i < 8; ++i) {
            float a = i * float(M_PI) / 4.f + 0.2f;
            p.drawEllipse(QPointF(c.x() + qCos(a) * (R - 2), c.y() + qSin(a) * (R - 2)), 1.9, 1.9);
        }
    }
    cache.insert(key, qMakePair(bottom, top));
}

void orb(QPainter *p, const QRectF &r, float t, const QColor &c1, const QColor &c2, float pulse)
{
    t = clampf(t, 0.f, 1.f);
    QPointF c = r.center();
    float R = qMin(r.width(), r.height()) * 0.5f;
    QImage bottom, top;
    orbLayers(int(R * 2), c1, c2, bottom, top);
    const QPointF off = QPointF(r.left(), r.top()) - QPointF(4, 4);

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->drawImage(off, bottom);

    QPainterPath clip;
    clip.addEllipse(c, R - 3, R - 3);
    p->setClipPath(clip);

    // жидкость
    float lvl = (R - 3) * 2 * (1.f - t);
    QRectF liq(r.left(), r.top() + lvl, r.width(), r.height());
    QLinearGradient g(liq.topLeft(), liq.bottomLeft());
    g.setColorAt(0, c2);
    g.setColorAt(1, c1.darker(180));
    p->fillRect(liq, g);
    // волна
    QPainterPath wave;
    float y0 = r.top() + lvl;
    wave.moveTo(r.left(), y0 + 4);
    for (float x = 0; x <= r.width(); x += 3.f) {
        wave.lineTo(r.left() + x, y0 + qSin(x * 0.075f + pulse * 3.f) * 3.4f);
    }
    wave.lineTo(r.right(), r.bottom());
    wave.lineTo(r.left(), r.bottom());
    wave.closeSubpath();
    p->fillPath(wave, QBrush(g));
    // блик поверхности
    p->setPen(QPen(QColor(255, 255, 255, 70), 1.4));
    p->setBrush(Qt::NoBrush);
    QPainterPath surf;
    surf.moveTo(r.left(), y0);
    for (float x = 0; x <= r.width(); x += 3.f)
        surf.lineTo(r.left() + x, y0 + qSin(x * 0.075f + pulse * 3.f) * 3.4f);
    p->drawPath(surf);

    // свечение при малом значении
    if (t < 0.28f) {
        QColor gc = c2;
        gc.setAlpha(int(120 * (0.5f + 0.5f * qSin(pulse * 6.f))));
        p->fillRect(r, gc);
    }
    p->setClipping(false);
    p->drawImage(off, top);
    p->restore();
}

void text(QPainter *p, const QPointF &pos, const QString &s, const QColor &c, const QFont &f, int flags)
{
    p->save();
    p->setFont(f);
    p->setPen(QColor(0, 0, 0, 170));
    p->drawText(pos + QPointF(1.5, 1.5), s);
    p->setPen(c);
    p->drawText(pos, s);
    p->restore();
    Q_UNUSED(flags);
}

void textShadow(QPainter *p, const QRectF &r, const QString &s, const QColor &c, const QFont &f, int flags)
{
    p->save();
    p->setFont(f);
    p->setPen(QColor(0, 0, 0, 190));
    p->drawText(r.translated(1.5, 1.5), flags, s);
    p->setPen(c);
    p->drawText(r, flags, s);
    p->restore();
}

// Кэшированный мягкий диск заданного цвета: рисовать готовую текстуру заметно
// быстрее, чем затенять QRadialGradient по пикселям.
static QImage softDiscTex(const QColor &col)
{
    static QHash<QRgb, QImage> cache;
    const QRgb key = col.rgb();
    QHash<QRgb, QImage>::const_iterator it = cache.constFind(key);
    if (it != cache.constEnd()) return it.value();
    if (cache.size() > 48) cache.clear();
    const int S = 128;
    QImage img(S, S, QImage::Format_ARGB32_Premultiplied);
    const float h = S * 0.5f;
    for (int y = 0; y < S; ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y));
        for (int x = 0; x < S; ++x) {
            float dx = (x + 0.5f - h) / h, dy = (y + 0.5f - h) / h;
            float d = qSqrt(dx * dx + dy * dy);
            float a = 0.f;
            if (d < 0.45f)      a = 1.f - (d / 0.45f) * 0.65f;        // 1.00 -> 0.35
            else if (d < 1.f)   a = 0.35f * (1.f - (d - 0.45f) / 0.55f); // 0.35 -> 0
            line[x] = qPremultiply(qRgba(col.red(), col.green(), col.blue(), int(a * 255.f)));
        }
    }
    cache.insert(key, img);
    return img;
}

void glow(QPainter *p, const QPointF &c, float radius, const QColor &col, float power)
{
    const float a = col.alphaF() * power;
    if (radius <= 0.5f || a <= 0.004f) return;
    p->save();
    p->setCompositionMode(QPainter::CompositionMode_Plus);
    p->setOpacity(qMin(1.f, a));
    p->setRenderHint(QPainter::SmoothPixmapTransform, true);
    p->drawImage(QRectF(c.x() - radius, c.y() - radius, radius * 2.f, radius * 2.f), softDiscTex(col));
    p->restore();
}

void disc(QPainter *p, const QPointF &c, float rx, float ry, const QColor &col, float opacity)
{
    const float a = col.alphaF() * opacity;
    if (rx <= 0.5f || ry <= 0.5f || a <= 0.004f) return;
    p->save();
    p->setOpacity(qMin(1.f, a));
    p->setRenderHint(QPainter::SmoothPixmapTransform, true);
    p->drawImage(QRectF(c.x() - rx, c.y() - ry, rx * 2.f, ry * 2.f), softDiscTex(col));
    p->restore();
}

void vignette(QPainter *p, const QRectF &r, float strength, const QColor &col)
{
    // виньетка статична для данного размера - держим готовую картинку полного
    // разрешения и кладём её 1:1 (гладкое растягивание каждым кадром слишком дорого)
    static QHash<quint64, QImage> cache;
    const int w2 = qMax(2, int(r.width())), h2 = qMax(2, int(r.height()));
    const quint64 key = (quint64(w2) << 44) ^ (quint64(h2) << 24) ^ quint64(col.rgba())
                      ^ quint64(int(strength * 997.f));
    QImage img = cache.value(key);
    if (img.isNull()) {
        if (cache.size() > 3) cache.clear();
        img = QImage(w2, h2, QImage::Format_ARGB32_Premultiplied);
        const float rad = qMax(float(w2), float(h2)) * 0.72f;
        const float cx = w2 * 0.5f, cy = h2 * 0.5f;
        for (int y = 0; y < h2; ++y) {
            QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y));
            for (int x = 0; x < w2; ++x) {
                float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
                float d = qSqrt(dx * dx + dy * dy) / rad;
                float k = d <= 0.45f ? 0.f : (d >= 1.f ? 1.f : (d - 0.45f) / 0.55f);
                int a = int(k * strength * 255.f);
                line[x] = a > 0 ? qPremultiply(qRgba(col.red(), col.green(), col.blue(), a)) : 0;
            }
        }
        cache.insert(key, img);
    }
    p->save();
    p->setRenderHint(QPainter::SmoothPixmapTransform, false);
    p->drawImage(QPoint(int(r.left()), int(r.top())), img);
    p->restore();
}

void divider(QPainter *p, qreal y, qreal x0, qreal x1, const QColor &c)
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    QLinearGradient g(x0, y, x1, y);
    QColor t = c; t.setAlpha(0);
    g.setColorAt(0, t);
    g.setColorAt(0.5, c);
    g.setColorAt(1, t);
    p->setPen(QPen(QBrush(g), 1.4));
    p->drawLine(QPointF(x0, y), QPointF(x1, y));
    qreal cx = (x0 + x1) * 0.5;
    p->setPen(Qt::NoPen);
    p->setBrush(c);
    QPolygonF d;
    d << QPointF(cx, y - 4) << QPointF(cx + 4, y) << QPointF(cx, y + 4) << QPointF(cx - 4, y);
    p->drawPolygon(d);
    p->restore();
}

void slotCell(QPainter *p, const QRectF &r, bool hover, bool selected, bool filled)
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing, false);
    QLinearGradient g(r.topLeft(), r.bottomLeft());
    g.setColorAt(0, filled ? C::panelHi : C::panelLo);
    g.setColorAt(1, C::ink);
    p->fillRect(r, g);
    p->setPen(QPen(selected ? C::amber : C::edge.darker(120), selected ? 2.0 : 1.0));
    p->setBrush(Qt::NoBrush);
    p->drawRect(r.adjusted(0.5, 0.5, -0.5, -0.5));
    if (hover) {
        QColor hc = C::amber; hc.setAlpha(40);
        p->fillRect(r.adjusted(1, 1, -1, -1), hc);
        p->setPen(QPen(C::amber, 1.4));
        p->drawRect(r.adjusted(1, 1, -1, -1));
    }
    p->restore();
}

} // namespace Style

// ============================================================
//  ГЛИФЫ / ИКОНКИ  (полностью векторные, рисуются в пространстве 0..1)
// ============================================================
namespace {

struct IconCtx {
    QPainter *p;
    QRectF r;
    QColor m;   // основной цвет
    QColor a;   // акцент
    QPointF u(float x, float y) const { return QPointF(r.left() + r.width() * x, r.top() + r.height() * y); }
    QRectF ur(float x, float y, float w, float h) const
    {
        return QRectF(r.left() + r.width() * x, r.top() + r.height() * y, r.width() * w, r.height() * h);
    }
    void poly(const QVector<QPointF> &pts, const QColor &fill, const QColor &line = QColor(), float lw = 1.0f) const
    {
        QPolygonF pg;
        for (int i = 0; i < pts.size(); ++i) pg << u(float(pts[i].x()), float(pts[i].y()));
        p->setBrush(fill);
        p->setPen(line.isValid() ? QPen(line, lw) : QPen(Qt::NoPen));
        p->drawPolygon(pg);
    }
    void ell(float cx, float cy, float rx, float ry, const QColor &fill, const QColor &line = QColor(), float lw = 1.0f) const
    {
        p->setBrush(fill);
        p->setPen(line.isValid() ? QPen(line, lw) : QPen(Qt::NoPen));
        p->drawEllipse(u(cx, cy), r.width() * rx, r.height() * ry);
    }
    void rect(float x, float y, float w, float h, const QColor &fill, const QColor &line = QColor(), float lw = 1.0f) const
    {
        p->setBrush(fill);
        p->setPen(line.isValid() ? QPen(line, lw) : QPen(Qt::NoPen));
        p->drawRect(ur(x, y, w, h));
    }
    void line(float x0, float y0, float x1, float y1, const QColor &c, float w = 1.0f) const
    {
        p->setPen(QPen(c, w, Qt::SolidLine, Qt::RoundCap));
        p->drawLine(u(x0, y0), u(x1, y1));
    }
    void polyLine(const QVector<QPointF> &pts, const QColor &c, float w = 1.0f) const
    {
        QPolygonF pg;
        for (int i = 0; i < pts.size(); ++i) pg << u(float(pts[i].x()), float(pts[i].y()));
        p->setPen(QPen(c, w, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p->setBrush(Qt::NoBrush);
        p->drawPolyline(pg);
    }
    QVector<QPointF> vec() const { return QVector<QPointF>(); }
};

QColor dark(const QColor &c, int k = 170) { return c.darker(k); }
QColor lite(const QColor &c, int k = 140) { return c.lighter(k); }

// --- общие иконки оружия ---
void iconSword(const IconCtx &x, bool great)
{
    float w = great ? 0.13f : 0.09f;
    x.poly(QVector<QPointF>() << QPointF(0.5, 0.04) << QPointF(0.5 + w, 0.18) << QPointF(0.5 + w, 0.66)
             << QPointF(0.5 - w, 0.66) << QPointF(0.5 - w, 0.18), lite(x.m), dark(x.m, 200), 1.0f);
    x.line(0.5, 0.12, 0.5, 0.64, dark(x.m, 240), 1.2f);
    float gw = great ? 0.34f : 0.26f;
    x.poly(QVector<QPointF>() << QPointF(0.5 - gw, 0.66) << QPointF(0.5 + gw, 0.66)
             << QPointF(0.5 + gw - 0.03f, 0.73f) << QPointF(0.5 - gw + 0.03f, 0.73f), x.a, dark(x.a, 200), 1.0f);
    x.rect(0.5 - 0.035f, 0.73f, 0.07f, 0.16f, dark(x.a, 150), dark(x.a, 220), 1.0f);
    x.ell(0.5, 0.93f, 0.055f, 0.055f, x.a, dark(x.a, 220), 1.0f);
}

void iconAxe(const IconCtx &x, bool great)
{
    float w = great ? 0.055f : 0.045f;
    x.rect(0.5 - w * 0.5f, 0.06f, w, 0.86f, dark(x.a, 130), dark(x.a, 220), 1.0f);
    float bw = great ? 0.30f : 0.22f;
    x.poly(QVector<QPointF>() << QPointF(0.5, 0.12f) << QPointF(0.5 + bw, 0.22f) << QPointF(0.5 + bw * 1.05f, 0.42f)
             << QPointF(0.5 + bw * 0.6f, 0.5f) << QPointF(0.5, 0.42f), lite(x.m), dark(x.m, 200), 1.0f);
    x.poly(QVector<QPointF>() << QPointF(0.5, 0.12f) << QPointF(0.5 - bw, 0.22f) << QPointF(0.5 - bw * 1.05f, 0.42f)
             << QPointF(0.5 - bw * 0.6f, 0.5f) << QPointF(0.5, 0.42f), x.m, dark(x.m, 220), 1.0f);
    x.ell(0.5, 0.09f, 0.06f, 0.05f, x.a, dark(x.a, 220), 1.0f);
}

void iconDagger(const IconCtx &x)
{
    x.poly(QVector<QPointF>() << QPointF(0.5, 0.12f) << QPointF(0.60f, 0.30f) << QPointF(0.56f, 0.60f)
             << QPointF(0.44f, 0.60f) << QPointF(0.40f, 0.30f), lite(x.m), dark(x.m, 210), 1.0f);
    x.poly(QVector<QPointF>() << QPointF(0.30f, 0.62f) << QPointF(0.70f, 0.62f) << QPointF(0.64f, 0.69f)
             << QPointF(0.36f, 0.69f), x.a, dark(x.a, 210), 1.0f);
    x.rect(0.465f, 0.69f, 0.07f, 0.20f, dark(x.a, 140), dark(x.a, 220), 1.0f);
}

void iconMace(const IconCtx &x)
{
    x.rect(0.46f, 0.35f, 0.08f, 0.56f, dark(x.a, 130), dark(x.a, 220), 1.0f);
    x.ell(0.5, 0.24f, 0.19f, 0.19f, lite(x.m), dark(x.m, 220), 1.2f);
    for (int i = 0; i < 6; ++i) {
        float ang = i * float(M_PI) / 3.f;
        x.poly(QVector<QPointF>()
               << QPointF(0.5 + qCos(ang) * 0.17f, 0.24f + qSin(ang) * 0.17f)
               << QPointF(0.5 + qCos(ang + 0.35f) * 0.27f, 0.24f + qSin(ang + 0.35f) * 0.27f)
               << QPointF(0.5 + qCos(ang + 0.7f) * 0.17f, 0.24f + qSin(ang + 0.7f) * 0.17f), x.m, dark(x.m, 240), 0.8f);
    }
}

void iconSpear(const IconCtx &x)
{
    x.rect(0.475f, 0.20f, 0.05f, 0.76f, dark(x.a, 120), dark(x.a, 220), 1.0f);
    x.poly(QVector<QPointF>() << QPointF(0.5, 0.03f) << QPointF(0.60f, 0.22f) << QPointF(0.5, 0.28f)
             << QPointF(0.40f, 0.22f), lite(x.m), dark(x.m, 220), 1.0f);
    x.line(0.5, 0.08f, 0.5, 0.24f, dark(x.m, 250), 1.0f);
}

void iconBow(const IconCtx &x)
{
    QPainterPath path;
    path.moveTo(x.u(0.22f, 0.10f));
    path.cubicTo(x.u(0.86f, 0.24f), x.u(0.86f, 0.76f), x.u(0.22f, 0.90f));
    x.p->setPen(QPen(dark(x.a, 140), qMax(2.0f, float(x.r.width()) * 0.055f), Qt::SolidLine, Qt::RoundCap));
    x.p->setBrush(Qt::NoBrush);
    x.p->drawPath(path);
    x.p->setPen(QPen(C::bone, 1.0f));
    x.p->drawLine(x.u(0.22f, 0.10f), x.u(0.22f, 0.90f));
    // стрела
    x.line(0.10f, 0.50f, 0.80f, 0.50f, lite(x.m), 1.6f);
    x.poly(QVector<QPointF>() << QPointF(0.88f, 0.50f) << QPointF(0.74f, 0.44f) << QPointF(0.74f, 0.56f), x.m);
}

void iconStaff(const IconCtx &x, bool wand)
{
    float top = wand ? 0.30f : 0.16f;
    x.rect(0.47f, top, 0.06f, 0.92f - top, dark(x.a, 120), dark(x.a, 230), 1.0f);
    x.ell(0.5, top * 0.62f, wand ? 0.11f : 0.14f, wand ? 0.11f : 0.14f, x.m, lite(x.m), 1.4f);
    x.ell(0.47f, top * 0.55f, 0.045f, 0.045f, lite(x.m, 190));
    // руны на посохе
    x.line(0.5, top + 0.10f, 0.5, top + 0.16f, x.m, 1.4f);
    x.ell(0.5, top + 0.24f, 0.035f, 0.035f, QColor(0,0,0,0), x.m, 1.2f);
}

void iconTome(const IconCtx &x)
{
    x.poly(QVector<QPointF>() << QPointF(0.14f, 0.16f) << QPointF(0.86f, 0.16f) << QPointF(0.86f, 0.86f)
             << QPointF(0.14f, 0.86f), dark(x.a, 130), dark(x.a, 230), 1.4f);
    x.rect(0.20f, 0.22f, 0.60f, 0.58f, C::bone, dark(C::bone, 170), 1.0f);
    x.line(0.5, 0.22f, 0.5, 0.80f, dark(C::bone, 200), 1.0f);
    x.poly(QVector<QPointF>() << QPointF(0.5, 0.32f) << QPointF(0.60f, 0.50f) << QPointF(0.5, 0.68f)
             << QPointF(0.40f, 0.50f), x.m, dark(x.m, 200), 1.0f);
    x.rect(0.44f, 0.10f, 0.12f, 0.10f, x.m, dark(x.m, 220), 1.0f);
}

void iconShield(const IconCtx &x)
{
    x.poly(QVector<QPointF>() << QPointF(0.16f, 0.12f) << QPointF(0.84f, 0.12f) << QPointF(0.80f, 0.56f)
             << QPointF(0.50f, 0.92f) << QPointF(0.20f, 0.56f), lite(x.m), dark(x.m, 230), 1.6f);
    x.poly(QVector<QPointF>() << QPointF(0.28f, 0.22f) << QPointF(0.72f, 0.22f) << QPointF(0.69f, 0.52f)
             << QPointF(0.50f, 0.78f) << QPointF(0.31f, 0.52f), dark(x.m, 160), x.a, 1.2f);
    x.ell(0.5, 0.42f, 0.10f, 0.10f, x.a, lite(x.a), 1.0f);
}

void iconTorch(const IconCtx &x)
{
    x.rect(0.45f, 0.44f, 0.10f, 0.50f, dark(x.a, 120), dark(x.a, 220), 1.0f);
    x.poly(QVector<QPointF>() << QPointF(0.5, 0.06f) << QPointF(0.68f, 0.30f) << QPointF(0.60f, 0.46f)
             << QPointF(0.40f, 0.46f) << QPointF(0.32f, 0.30f), x.m, lite(x.m), 1.0f);
    x.poly(QVector<QPointF>() << QPointF(0.5, 0.18f) << QPointF(0.58f, 0.32f) << QPointF(0.5, 0.44f)
             << QPointF(0.42f, 0.32f), C::lightning, QColor(), 0.8f);
}

void iconTotem(const IconCtx &x)
{
    x.poly(QVector<QPointF>() << QPointF(0.30f, 0.90f) << QPointF(0.34f, 0.20f) << QPointF(0.50f, 0.10f)
             << QPointF(0.66f, 0.20f) << QPointF(0.70f, 0.90f), dark(x.a, 130), dark(x.a, 230), 1.2f);
    x.ell(0.5, 0.34f, 0.10f, 0.10f, x.m, lite(x.m), 1.0f);
    x.rect(0.36f, 0.52f, 0.28f, 0.06f, x.m);
    x.rect(0.38f, 0.66f, 0.24f, 0.05f, dark(x.m, 150));
}

// --- броня ---
void iconHelm(const IconCtx &x, int variant)
{
    QPainterPath d;
    d.moveTo(x.u(0.18f, 0.60f));
    d.cubicTo(x.u(0.18f, 0.16f), x.u(0.82f, 0.16f), x.u(0.82f, 0.60f));
    d.lineTo(x.u(0.70f, 0.88f));
    d.lineTo(x.u(0.30f, 0.88f));
    d.closeSubpath();
    x.p->setBrush(lite(x.m));
    x.p->setPen(QPen(dark(x.m, 230), 1.6f));
    x.p->drawPath(d);
    // забрало
    x.p->setBrush(dark(x.m, 260));
    x.p->setPen(QPen(C::voidBlack, 1.0f));
    x.p->drawRect(x.ur(0.26f, 0.50f, 0.48f, 0.13f));
    x.p->setPen(QPen(x.a, 1.4f));
    x.p->drawLine(x.u(0.5, 0.16f), x.u(0.5, 0.63f));
    if (variant == 2) { // корона
        x.p->setBrush(x.a); x.p->setPen(QPen(dark(x.a, 200), 1.2f));
        QPolygonF cr;
        cr << x.u(0.18f, 0.24f) << x.u(0.30f, 0.06f) << x.u(0.40f, 0.22f) << x.u(0.50f, 0.02f)
           << x.u(0.60f, 0.22f) << x.u(0.70f, 0.06f) << x.u(0.82f, 0.24f);
        x.p->drawPolygon(cr);
        x.ell(0.5, 0.14f, 0.035f, 0.035f, C::bloodHi);
    } else if (variant == 1) { // капюшон
        QPainterPath h;
        h.moveTo(x.u(0.10f, 0.70f));
        h.cubicTo(x.u(0.16f, 0.06f), x.u(0.84f, 0.06f), x.u(0.90f, 0.70f));
        h.cubicTo(x.u(0.70f, 0.50f), x.u(0.30f, 0.50f), x.u(0.10f, 0.70f));
        x.p->setBrush(dark(x.m, 170));
        x.p->setPen(QPen(dark(x.m, 240), 1.4f));
        x.p->drawPath(h);
        x.ell(0.5, 0.48f, 0.13f, 0.10f, C::voidBlack);
        x.ell(0.45f, 0.47f, 0.025f, 0.02f, x.a);
        x.ell(0.55f, 0.47f, 0.025f, 0.02f, x.a);
    }
}

void iconChest(const IconCtx &x, int variant)
{
    QPainterPath d;
    d.moveTo(x.u(0.28f, 0.12f));
    d.lineTo(x.u(0.72f, 0.12f));
    d.lineTo(x.u(0.90f, 0.24f));
    d.lineTo(x.u(0.82f, 0.42f));
    d.lineTo(x.u(0.78f, 0.90f));
    d.lineTo(x.u(0.22f, 0.90f));
    d.lineTo(x.u(0.18f, 0.42f));
    d.lineTo(x.u(0.10f, 0.24f));
    d.closeSubpath();
    x.p->setBrush(variant == 1 ? lite(x.m) : x.m);
    x.p->setPen(QPen(dark(x.m, 240), 1.6f));
    x.p->drawPath(d);
    if (variant == 2) { // латы - пластины
        for (int i = 0; i < 4; ++i)
            x.rect(0.30f, 0.30f + i * 0.14f, 0.40f, 0.08f, lite(x.m, 130), dark(x.m, 200), 0.8f);
    } else { // роба - подол и пояс
        x.rect(0.22f, 0.52f, 0.56f, 0.07f, x.a, dark(x.a, 200), 0.8f);
        x.poly(QVector<QPointF>() << QPointF(0.5, 0.20f) << QPointF(0.58f, 0.34f) << QPointF(0.5, 0.48f)
                 << QPointF(0.42f, 0.34f), lite(x.m, 150), x.a, 0.8f);
        x.line(0.30f, 0.70f, 0.70f, 0.70f, dark(x.m, 200), 1.0f);
        x.line(0.28f, 0.82f, 0.72f, 0.82f, dark(x.m, 200), 1.0f);
    }
    x.p->setPen(QPen(dark(x.m, 260), 1.0f));
    x.p->drawLine(x.u(0.28f, 0.12f), x.u(0.40f, 0.06f));
    x.p->drawLine(x.u(0.72f, 0.12f), x.u(0.60f, 0.06f));
}

void iconGloves(const IconCtx &x)
{
    x.poly(QVector<QPointF>() << QPointF(0.30f, 0.30f) << QPointF(0.72f, 0.30f) << QPointF(0.76f, 0.66f)
             << QPointF(0.66f, 0.88f) << QPointF(0.34f, 0.88f) << QPointF(0.24f, 0.62f), lite(x.m), dark(x.m, 230), 1.4f);
    for (int i = 0; i < 3; ++i)
        x.rect(0.36f + i * 0.11f, 0.10f, 0.08f, 0.22f, x.m, dark(x.m, 230), 1.0f);
    x.rect(0.24f, 0.56f, 0.52f, 0.09f, x.a, dark(x.a, 210), 0.9f);
}

void iconBelt(const IconCtx &x)
{
    x.rect(0.06f, 0.36f, 0.88f, 0.26f, x.m, dark(x.m, 230), 1.4f);
    x.rect(0.36f, 0.28f, 0.28f, 0.42f, x.a, dark(x.a, 220), 1.4f);
    x.rect(0.42f, 0.36f, 0.16f, 0.26f, dark(x.a, 200));
    x.ell(0.5, 0.49f, 0.045f, 0.06f, lite(x.a, 150));
    for (int i = 0; i < 3; ++i) {
        x.ell(0.16f + i * 0.06f, 0.49f, 0.02f, 0.02f, dark(x.m, 220));
        x.ell(0.78f + i * 0.06f, 0.49f, 0.02f, 0.02f, dark(x.m, 220));
    }
}

void iconBoots(const IconCtx &x)
{
    x.poly(QVector<QPointF>() << QPointF(0.26f, 0.10f) << QPointF(0.54f, 0.10f) << QPointF(0.56f, 0.56f)
             << QPointF(0.88f, 0.70f) << QPointF(0.88f, 0.88f) << QPointF(0.24f, 0.88f), lite(x.m), dark(x.m, 240), 1.4f);
    x.rect(0.24f, 0.30f, 0.32f, 0.08f, x.a, dark(x.a, 210), 0.8f);
    x.poly(QVector<QPointF>() << QPointF(0.24f, 0.78f) << QPointF(0.88f, 0.78f) << QPointF(0.88f, 0.88f)
             << QPointF(0.24f, 0.88f), dark(x.m, 180), dark(x.m, 250), 0.8f);
}

void iconAmulet(const IconCtx &x)
{
    QPainterPath c;
    c.moveTo(x.u(0.16f, 0.10f));
    c.cubicTo(x.u(0.24f, 0.46f), x.u(0.76f, 0.46f), x.u(0.84f, 0.10f));
    x.p->setPen(QPen(x.a, qMax(1.6f, float(x.r.width()) * 0.045f)));
    x.p->setBrush(Qt::NoBrush);
    x.p->drawPath(c);
    x.poly(QVector<QPointF>() << QPointF(0.5, 0.44f) << QPointF(0.66f, 0.60f) << QPointF(0.5, 0.90f)
             << QPointF(0.34f, 0.60f), x.m, lite(x.m), 1.4f);
    x.ell(0.5, 0.62f, 0.05f, 0.06f, lite(x.m, 200));
}

void iconRing(const IconCtx &x)
{
    x.ell(0.5, 0.62f, 0.26f, 0.26f, QColor(0,0,0,0), x.a, qMax(2.4f, float(x.r.width()) * 0.075f));
    x.poly(QVector<QPointF>() << QPointF(0.5, 0.08f) << QPointF(0.63f, 0.24f) << QPointF(0.5, 0.40f)
             << QPointF(0.37f, 0.24f), x.m, lite(x.m), 1.2f);
    x.poly(QVector<QPointF>() << QPointF(0.42f, 0.36f) << QPointF(0.58f, 0.36f) << QPointF(0.54f, 0.44f)
             << QPointF(0.46f, 0.44f), x.a, dark(x.a, 200), 0.8f);
}

// --- камни и зелья ---
void iconGem(const IconCtx &x, int shape)
{
    if (shape == 0) {
        x.poly(QVector<QPointF>() << QPointF(0.5, 0.06f) << QPointF(0.86f, 0.38f) << QPointF(0.5, 0.94f)
                 << QPointF(0.14f, 0.38f), x.m, lite(x.m), 1.4f);
        x.polyLine(QVector<QPointF>() << QPointF(0.14f, 0.38f) << QPointF(0.5, 0.52f) << QPointF(0.86f, 0.38f),
                   lite(x.m, 170), 1.0f);
        x.polyLine(QVector<QPointF>() << QPointF(0.5, 0.06f) << QPointF(0.5, 0.94f), dark(x.m, 170), 0.9f);
    } else {
        x.poly(QVector<QPointF>() << QPointF(0.30f, 0.12f) << QPointF(0.70f, 0.12f) << QPointF(0.88f, 0.50f)
                 << QPointF(0.50f, 0.92f) << QPointF(0.12f, 0.50f), x.m, lite(x.m), 1.4f);
        x.polyLine(QVector<QPointF>() << QPointF(0.30f, 0.12f) << QPointF(0.5, 0.5f) << QPointF(0.70f, 0.12f),
                   lite(x.m, 170), 0.9f);
        x.polyLine(QVector<QPointF>() << QPointF(0.12f, 0.50f) << QPointF(0.5, 0.5f) << QPointF(0.88f, 0.50f),
                   lite(x.m, 150), 0.9f);
        x.polyLine(QVector<QPointF>() << QPointF(0.5, 0.5f) << QPointF(0.5, 0.92f), dark(x.m, 160), 0.9f);
    }
    x.ell(0.40f, 0.30f, 0.05f, 0.04f, QColor(255, 255, 255, 160));
}

void iconPotion(const IconCtx &x, const QColor &liq)
{
    x.rect(0.42f, 0.08f, 0.16f, 0.14f, dark(x.a, 120), dark(x.a, 230), 1.0f);
    x.rect(0.40f, 0.04f, 0.20f, 0.07f, x.a, dark(x.a, 210), 0.9f);
    QPainterPath b;
    b.moveTo(x.u(0.42f, 0.22f));
    b.lineTo(x.u(0.58f, 0.22f));
    b.cubicTo(x.u(0.88f, 0.42f), x.u(0.86f, 0.94f), x.u(0.5, 0.94f));
    b.cubicTo(x.u(0.14f, 0.94f), x.u(0.12f, 0.42f), x.u(0.42f, 0.22f));
    x.p->setBrush(QColor(220, 235, 245, 60));
    x.p->setPen(QPen(C::boneDim, 1.4f));
    x.p->drawPath(b);
    x.p->save();
    x.p->setClipPath(b);
    x.p->fillRect(x.ur(0.10f, 0.48f, 0.80f, 0.50f), liq);
    QColor w(255, 255, 255, 60);
    x.p->fillRect(x.ur(0.10f, 0.48f, 0.80f, 0.05f), w);
    x.p->restore();
    x.ell(0.38f, 0.60f, 0.05f, 0.07f, QColor(255, 255, 255, 90));
    x.ell(0.60f, 0.72f, 0.03f, 0.03f, QColor(255, 255, 255, 70));
}

void iconScroll(const IconCtx &x)
{
    x.rect(0.16f, 0.18f, 0.68f, 0.64f, C::bone, dark(C::bone, 170), 1.2f);
    x.ell(0.16f, 0.50f, 0.07f, 0.34f, x.a, dark(x.a, 200), 1.2f);
    x.ell(0.84f, 0.50f, 0.07f, 0.34f, x.a, dark(x.a, 200), 1.2f);
    for (int i = 0; i < 4; ++i)
        x.line(0.28f, 0.32f + i * 0.11f, 0.72f, 0.32f + i * 0.11f, dark(C::boneDim, 150), 1.0f);
    x.poly(QVector<QPointF>() << QPointF(0.5, 0.30f) << QPointF(0.57f, 0.45f) << QPointF(0.5, 0.60f)
             << QPointF(0.43f, 0.45f), x.m);
}

void iconCoin(const IconCtx &x, bool many)
{
    if (many) {
        x.ell(0.32f, 0.70f, 0.22f, 0.16f, dark(x.m, 130), dark(x.m, 220), 1.0f);
        x.ell(0.66f, 0.74f, 0.22f, 0.16f, x.m, dark(x.m, 220), 1.0f);
        x.ell(0.50f, 0.44f, 0.24f, 0.18f, lite(x.m), dark(x.m, 200), 1.2f);
        x.ell(0.50f, 0.42f, 0.10f, 0.07f, dark(x.m, 160));
        return;
    }
    x.ell(0.5, 0.52f, 0.34f, 0.34f, lite(x.m), dark(x.m, 230), 1.6f);
    x.ell(0.5, 0.52f, 0.24f, 0.24f, x.m, dark(x.m, 180), 1.0f);
    x.poly(QVector<QPointF>() << QPointF(0.5, 0.34f) << QPointF(0.60f, 0.52f) << QPointF(0.5, 0.70f)
             << QPointF(0.40f, 0.52f), lite(x.m, 160));
}

// --- умения ---
void iconCleave(const IconCtx &x)
{
    QPainterPath arc;
    arc.moveTo(x.u(0.12f, 0.80f));
    arc.cubicTo(x.u(0.20f, 0.20f), x.u(0.80f, 0.20f), x.u(0.88f, 0.80f));
    arc.cubicTo(x.u(0.72f, 0.42f), x.u(0.28f, 0.42f), x.u(0.12f, 0.80f));
    x.p->setBrush(x.m);
    x.p->setPen(QPen(lite(x.m), 1.4f));
    x.p->drawPath(arc);
    x.line(0.5, 0.95f, 0.5, 0.55f, x.a, 3.0f);
}

void iconSpin(const IconCtx &x)
{
    for (int i = 0; i < 3; ++i) {
        float a = i * 2.094f;
        QPainterPath arc;
        arc.moveTo(x.u(0.5f + qCos(a) * 0.14f, 0.5f + qSin(a) * 0.14f));
        arc.cubicTo(x.u(0.5f + qCos(a) * 0.52f, 0.5f + qSin(a) * 0.52f),
                    x.u(0.5f + qCos(a + 1.1f) * 0.55f, 0.5f + qSin(a + 1.1f) * 0.55f),
                    x.u(0.5f + qCos(a + 1.7f) * 0.22f, 0.5f + qSin(a + 1.7f) * 0.22f));
        x.p->setPen(QPen(x.m, qMax(2.2f, float(x.r.width()) * 0.09f), Qt::SolidLine, Qt::RoundCap));
        x.p->drawPath(arc);
    }
    x.ell(0.5, 0.5, 0.11f, 0.11f, x.a, lite(x.a), 1.2f);
}

void iconDash(const IconCtx &x)
{
    for (int i = 0; i < 3; ++i) {
        float o = i * 0.10f;
        QColor c = x.m; c.setAlphaF(1.0f - i * 0.28f);
        x.poly(QVector<QPointF>() << QPointF(0.20f + o, 0.28f) << QPointF(0.44f + o, 0.50f)
                 << QPointF(0.20f + o, 0.72f) << QPointF(0.30f + o, 0.50f), c);
    }
    x.poly(QVector<QPointF>() << QPointF(0.56f, 0.16f) << QPointF(0.94f, 0.50f) << QPointF(0.56f, 0.84f)
             << QPointF(0.68f, 0.50f), x.a, lite(x.a), 1.2f);
}

void iconFireball(const IconCtx &x)
{
    x.ell(0.5, 0.56f, 0.24f, 0.24f, x.m, lite(x.m), 1.0f);
    x.ell(0.5, 0.56f, 0.13f, 0.13f, C::lightning);
    x.poly(QVector<QPointF>() << QPointF(0.34f, 0.40f) << QPointF(0.26f, 0.10f) << QPointF(0.44f, 0.30f), x.m);
    x.poly(QVector<QPointF>() << QPointF(0.66f, 0.40f) << QPointF(0.76f, 0.12f) << QPointF(0.58f, 0.30f), x.m);
    x.poly(QVector<QPointF>() << QPointF(0.50f, 0.32f) << QPointF(0.52f, 0.04f) << QPointF(0.60f, 0.26f), lite(x.m, 130));
}

void iconMeteor(const IconCtx &x)
{
    x.line(0.10f, 0.06f, 0.44f, 0.44f, x.m, 3.0f);
    x.line(0.24f, 0.02f, 0.52f, 0.34f, x.m, 2.0f);
    x.ell(0.52f, 0.50f, 0.16f, 0.16f, lite(x.m), x.m, 1.4f);
    x.ell(0.52f, 0.50f, 0.08f, 0.08f, C::lightning);
    x.p->setPen(QPen(x.a, 2.0f));
    x.p->setBrush(Qt::NoBrush);
    x.p->drawEllipse(x.u(0.52f, 0.86f), float(x.r.width()) * 0.34f, float(x.r.height()) * 0.11f);
}

void iconIceShard(const IconCtx &x)
{
    x.poly(QVector<QPointF>() << QPointF(0.5, 0.04f) << QPointF(0.72f, 0.44f) << QPointF(0.5, 0.96f)
             << QPointF(0.28f, 0.44f), x.m, lite(x.m), 1.4f);
    x.polyLine(QVector<QPointF>() << QPointF(0.5, 0.04f) << QPointF(0.5, 0.96f), lite(x.m, 180), 1.0f);
    x.polyLine(QVector<QPointF>() << QPointF(0.28f, 0.44f) << QPointF(0.72f, 0.44f), lite(x.m, 150), 0.9f);
    x.poly(QVector<QPointF>() << QPointF(0.14f, 0.30f) << QPointF(0.24f, 0.42f) << QPointF(0.10f, 0.48f), lite(x.m, 140));
    x.poly(QVector<QPointF>() << QPointF(0.86f, 0.30f) << QPointF(0.76f, 0.42f) << QPointF(0.90f, 0.48f), lite(x.m, 140));
}

void iconChain(const IconCtx &x)
{
    const float px[4] = {0.16f, 0.40f, 0.62f, 0.84f};
    const float py[4] = {0.62f, 0.28f, 0.70f, 0.34f};
    for (int i = 0; i < 3; ++i) {
        x.p->setPen(QPen(x.m, 2.2f, Qt::SolidLine, Qt::RoundCap));
        QPointF a = x.u(px[i], py[i]), b = x.u(px[i + 1], py[i + 1]);
        QPointF m = (a + b) * 0.5f + QPointF(0, -6);
        QPainterPath pp; pp.moveTo(a); pp.quadTo(m, b);
        x.p->drawPath(pp);
    }
    for (int i = 0; i < 4; ++i)
        x.ell(px[i], py[i], 0.09f, 0.09f, lite(x.m), x.a, 1.2f);
}

void iconNova(const IconCtx &x)
{
    x.p->setPen(QPen(x.a, 1.6f));
    x.p->setBrush(Qt::NoBrush);
    x.p->drawEllipse(x.u(0.5, 0.5), float(x.r.width()) * 0.40f, float(x.r.height()) * 0.40f);
    x.p->setPen(QPen(x.m, 1.2f));
    x.p->drawEllipse(x.u(0.5, 0.5), float(x.r.width()) * 0.26f, float(x.r.height()) * 0.26f);
    for (int i = 0; i < 8; ++i) {
        float a = i * float(M_PI) / 4.f;
        x.poly(QVector<QPointF>()
               << QPointF(0.5f + qCos(a) * 0.40f, 0.5f + qSin(a) * 0.40f)
               << QPointF(0.5f + qCos(a - 0.2f) * 0.52f, 0.5f + qSin(a - 0.2f) * 0.52f)
               << QPointF(0.5f + qCos(a + 0.2f) * 0.52f, 0.5f + qSin(a + 0.2f) * 0.52f), lite(x.m));
    }
    x.ell(0.5, 0.5, 0.11f, 0.11f, x.m, lite(x.m), 1.0f);
}

void iconPoison(const IconCtx &x)
{
    for (int i = 0; i < 4; ++i) {
        float cx = 0.26f + i * 0.17f, cy = 0.62f - (i % 2) * 0.14f;
        QColor c = x.m; c.setAlphaF(0.55f + (i % 2) * 0.35f);
        x.ell(cx, cy, 0.15f, 0.13f, c);
    }
    x.ell(0.5, 0.30f, 0.10f, 0.10f, lite(x.m, 140));
    x.ell(0.36f, 0.80f, 0.07f, 0.06f, dark(x.m, 140));
    x.poly(QVector<QPointF>() << QPointF(0.72f, 0.20f) << QPointF(0.78f, 0.34f) << QPointF(0.66f, 0.32f), lite(x.m, 160));
}

void iconSummon(const IconCtx &x)
{
    x.p->setPen(QPen(x.a, 1.4f));
    x.p->setBrush(Qt::NoBrush);
    x.p->drawEllipse(x.u(0.5, 0.86f), float(x.r.width()) * 0.34f, float(x.r.height()) * 0.10f);
    QPainterPath g;
    g.moveTo(x.u(0.30f, 0.80f));
    g.cubicTo(x.u(0.26f, 0.34f), x.u(0.40f, 0.10f), x.u(0.50f, 0.10f));
    g.cubicTo(x.u(0.60f, 0.10f), x.u(0.74f, 0.34f), x.u(0.70f, 0.80f));
    g.closeSubpath();
    x.p->setBrush(x.m);
    x.p->setPen(QPen(lite(x.m), 1.2f));
    x.p->drawPath(g);
    x.ell(0.42f, 0.32f, 0.045f, 0.035f, x.a);
    x.ell(0.58f, 0.32f, 0.045f, 0.035f, x.a);
    x.poly(QVector<QPointF>() << QPointF(0.40f, 0.44f) << QPointF(0.60f, 0.44f) << QPointF(0.50f, 0.52f), dark(x.m, 200));
}

void iconWard(const IconCtx &x)
{
    x.p->setPen(QPen(x.m, 2.2f));
    x.p->setBrush(QColor(x.m.red(), x.m.green(), x.m.blue(), 60));
    QPainterPath h;
    h.moveTo(x.u(0.5, 0.06f));
    for (int i = 0; i < 6; ++i) {
        float a = -float(M_PI) / 2 + i * float(M_PI) / 3.f;
        h.lineTo(x.u(0.5f + qCos(a) * 0.42f, 0.5f + qSin(a) * 0.44f));
    }
    h.closeSubpath();
    x.p->drawPath(h);
    x.poly(QVector<QPointF>() << QPointF(0.5, 0.24f) << QPointF(0.62f, 0.50f) << QPointF(0.5, 0.76f)
             << QPointF(0.38f, 0.50f), x.a, lite(x.a), 1.0f);
}

void iconHeal(const IconCtx &x)
{
    x.rect(0.40f, 0.16f, 0.20f, 0.68f, x.m, lite(x.m), 1.2f);
    x.rect(0.16f, 0.40f, 0.68f, 0.20f, x.m, lite(x.m), 1.2f);
    x.ell(0.5, 0.50f, 0.08f, 0.08f, lite(x.m, 190));
}

void iconMultiArrow(const IconCtx &x)
{
    for (int i = -1; i <= 1; ++i) {
        float a = i * 0.28f;
        float dx = qSin(a), dy = -qCos(a);
        float x0 = 0.5f + dx * 0.05f - dx * 0.34f, y0 = 0.86f - dy * 0.34f * -1;
        float x1 = 0.5f + dx * 0.42f, y1 = 0.86f - 0.72f + i * 0.06f;
        Q_UNUSED(x0); Q_UNUSED(y0);
        x.line(0.5f + dx * 0.10f, 0.80f, x1, y1, x.m, 1.8f);
        x.poly(QVector<QPointF>() << QPointF(x1, y1 - 0.06f) << QPointF(x1 - 0.05f, y1 + 0.04f)
                 << QPointF(x1 + 0.05f, y1 + 0.04f), lite(x.m));
    }
}

void iconQuake(const IconCtx &x)
{
    x.p->setPen(QPen(x.a, 2.0f));
    x.p->setBrush(Qt::NoBrush);
    for (int i = 1; i <= 3; ++i)
        x.p->drawEllipse(x.u(0.5, 0.72f), float(x.r.width()) * 0.13f * i, float(x.r.height()) * 0.045f * i);
    for (int i = 0; i < 5; ++i) {
        float px = 0.14f + i * 0.18f;
        x.poly(QVector<QPointF>() << QPointF(px, 0.66f) << QPointF(px + 0.08f, 0.44f - (i % 2) * 0.12f)
                 << QPointF(px + 0.14f, 0.66f), x.m, dark(x.m, 200), 0.9f);
    }
}

void iconMark(const IconCtx &x)
{
    x.p->setPen(QPen(x.a, 1.8f));
    x.p->setBrush(Qt::NoBrush);
    x.p->drawEllipse(x.u(0.5, 0.5), float(x.r.width()) * 0.36f, float(x.r.height()) * 0.36f);
    x.p->drawEllipse(x.u(0.5, 0.5), float(x.r.width()) * 0.18f, float(x.r.height()) * 0.18f);
    x.line(0.5, 0.02f, 0.5, 0.22f, x.m, 1.8f);
    x.line(0.5, 0.78f, 0.5, 0.98f, x.m, 1.8f);
    x.line(0.02f, 0.5, 0.22f, 0.5, x.m, 1.8f);
    x.line(0.78f, 0.5, 0.98f, 0.5, x.m, 1.8f);
    x.ell(0.5, 0.5, 0.05f, 0.05f, x.m);
}

void iconBlood(const IconCtx &x)
{
    x.poly(QVector<QPointF>() << QPointF(0.5, 0.06f) << QPointF(0.80f, 0.52f) << QPointF(0.5, 0.94f)
             << QPointF(0.20f, 0.52f), x.m, lite(x.m), 1.2f);
    x.ell(0.42f, 0.46f, 0.07f, 0.09f, QColor(255, 255, 255, 120));
    x.ell(0.76f, 0.24f, 0.06f, 0.06f, lite(x.m, 130));
}

void iconCurse(const IconCtx &x)
{
    x.ell(0.5, 0.44f, 0.28f, 0.30f, x.m, dark(x.m, 220), 1.4f);
    x.ell(0.38f, 0.40f, 0.08f, 0.09f, C::voidBlack);
    x.ell(0.62f, 0.40f, 0.08f, 0.09f, C::voidBlack);
    x.ell(0.38f, 0.40f, 0.03f, 0.035f, x.a);
    x.ell(0.62f, 0.40f, 0.03f, 0.035f, x.a);
    x.poly(QVector<QPointF>() << QPointF(0.38f, 0.62f) << QPointF(0.44f, 0.56f) << QPointF(0.50f, 0.62f)
             << QPointF(0.56f, 0.56f) << QPointF(0.62f, 0.62f) << QPointF(0.62f, 0.70f)
             << QPointF(0.38f, 0.70f), C::voidBlack);
    x.line(0.30f, 0.16f, 0.36f, 0.26f, x.a, 1.4f);
    x.line(0.70f, 0.16f, 0.64f, 0.26f, x.a, 1.4f);
}

void iconBlink(const IconCtx &x)
{
    for (int i = 0; i < 3; ++i) {
        QColor c = x.m; c.setAlphaF(0.35f + i * 0.3f);
        float o = i * 0.14f;
        x.poly(QVector<QPointF>() << QPointF(0.22f + o, 0.18f) << QPointF(0.42f + o, 0.50f)
                 << QPointF(0.22f + o, 0.82f) << QPointF(0.30f + o, 0.50f), c);
    }
    x.p->setPen(QPen(x.a, 1.4f));
    x.p->setBrush(Qt::NoBrush);
    x.p->drawEllipse(x.u(0.76f, 0.5f), float(x.r.width()) * 0.16f, float(x.r.height()) * 0.20f);
    x.ell(0.76f, 0.5f, 0.06f, 0.08f, x.m);
}

void iconStatus(const IconCtx &x, int kind)
{
    switch (kind) {
    case 0: // горение
        x.poly(QVector<QPointF>() << QPointF(0.5, 0.06f) << QPointF(0.74f, 0.52f) << QPointF(0.5, 0.94f)
                 << QPointF(0.26f, 0.52f), x.m, lite(x.m), 1.0f);
        x.poly(QVector<QPointF>() << QPointF(0.5, 0.36f) << QPointF(0.62f, 0.60f) << QPointF(0.5, 0.86f)
                 << QPointF(0.38f, 0.60f), C::lightning);
        break;
    case 1: // заморозка
        for (int i = 0; i < 6; ++i) {
            float a = i * float(M_PI) / 3.f;
            x.line(0.5f, 0.5f, 0.5f + qCos(a) * 0.42f, 0.5f + qSin(a) * 0.42f, x.m, 1.8f);
            x.line(0.5f + qCos(a) * 0.26f, 0.5f + qSin(a) * 0.26f,
                   0.5f + qCos(a + 0.5f) * 0.36f, 0.5f + qSin(a + 0.5f) * 0.36f, lite(x.m), 1.2f);
            x.line(0.5f + qCos(a) * 0.26f, 0.5f + qSin(a) * 0.26f,
                   0.5f + qCos(a - 0.5f) * 0.36f, 0.5f + qSin(a - 0.5f) * 0.36f, lite(x.m), 1.2f);
        }
        break;
    case 2: // шок
        x.poly(QVector<QPointF>() << QPointF(0.62f, 0.04f) << QPointF(0.30f, 0.50f) << QPointF(0.48f, 0.50f)
                 << QPointF(0.34f, 0.96f) << QPointF(0.74f, 0.42f) << QPointF(0.54f, 0.42f), x.m, lite(x.m), 1.0f);
        break;
    case 3: // яд
        x.ell(0.5, 0.62f, 0.26f, 0.24f, x.m, dark(x.m, 200), 1.0f);
        x.ell(0.40f, 0.58f, 0.06f, 0.07f, lite(x.m, 160));
        x.poly(QVector<QPointF>() << QPointF(0.5, 0.10f) << QPointF(0.60f, 0.40f) << QPointF(0.40f, 0.40f), lite(x.m, 130));
        break;
    case 4: // щит
        x.poly(QVector<QPointF>() << QPointF(0.18f, 0.12f) << QPointF(0.82f, 0.12f) << QPointF(0.74f, 0.58f)
                 << QPointF(0.50f, 0.92f) << QPointF(0.26f, 0.58f), x.m, lite(x.m), 1.4f);
        break;
    case 5: // haste
        x.poly(QVector<QPointF>() << QPointF(0.14f, 0.30f) << QPointF(0.60f, 0.30f) << QPointF(0.44f, 0.46f)
                 << QPointF(0.86f, 0.46f) << QPointF(0.34f, 0.86f) << QPointF(0.46f, 0.58f)
                 << QPointF(0.14f, 0.58f), x.m, lite(x.m), 1.0f);
        break;
    default:
        x.ell(0.5, 0.5, 0.3f, 0.3f, x.m);
        break;
    }
}

void iconSkull(const IconCtx &x)
{
    QPainterPath d;
    d.moveTo(x.u(0.22f, 0.44f));
    d.cubicTo(x.u(0.22f, 0.10f), x.u(0.78f, 0.10f), x.u(0.78f, 0.44f));
    d.lineTo(x.u(0.70f, 0.66f));
    d.lineTo(x.u(0.66f, 0.84f));
    d.lineTo(x.u(0.34f, 0.84f));
    d.lineTo(x.u(0.30f, 0.66f));
    d.closeSubpath();
    x.p->setBrush(x.m); x.p->setPen(QPen(dark(x.m, 200), 1.2f)); x.p->drawPath(d);
    x.ell(0.37f, 0.42f, 0.10f, 0.11f, C::voidBlack);
    x.ell(0.63f, 0.42f, 0.10f, 0.11f, C::voidBlack);
    x.ell(0.37f, 0.42f, 0.035f, 0.04f, x.a);
    x.ell(0.63f, 0.42f, 0.035f, 0.04f, x.a);
    x.poly(QVector<QPointF>() << QPointF(0.5, 0.52f) << QPointF(0.55f, 0.64f) << QPointF(0.45f, 0.64f), dark(x.m, 220));
    for (int i = 0; i < 4; ++i)
        x.rect(0.36f + i * 0.075f, 0.72f, 0.045f, 0.12f, dark(x.m, 170), dark(x.m, 240), 0.7f);
}

void iconHeart(const IconCtx &x)
{
    QPainterPath d;
    d.moveTo(x.u(0.5, 0.90f));
    d.cubicTo(x.u(0.06f, 0.56f), x.u(0.20f, 0.10f), x.u(0.5, 0.32f));
    d.cubicTo(x.u(0.80f, 0.10f), x.u(0.94f, 0.56f), x.u(0.5, 0.90f));
    x.p->setBrush(x.m); x.p->setPen(QPen(lite(x.m), 1.2f)); x.p->drawPath(d);
    x.ell(0.38f, 0.38f, 0.07f, 0.05f, QColor(255, 255, 255, 140));
}

void iconDrop(const IconCtx &x)
{
    QPainterPath d;
    d.moveTo(x.u(0.5, 0.06f));
    d.cubicTo(x.u(0.86f, 0.52f), x.u(0.78f, 0.94f), x.u(0.5, 0.94f));
    d.cubicTo(x.u(0.22f, 0.94f), x.u(0.14f, 0.52f), x.u(0.5, 0.06f));
    x.p->setBrush(x.m); x.p->setPen(QPen(lite(x.m), 1.2f)); x.p->drawPath(d);
    x.ell(0.40f, 0.62f, 0.06f, 0.09f, QColor(255, 255, 255, 120));
}

void iconEye(const IconCtx &x)
{
    QPainterPath d;
    d.moveTo(x.u(0.06f, 0.50f));
    d.cubicTo(x.u(0.28f, 0.16f), x.u(0.72f, 0.16f), x.u(0.94f, 0.50f));
    d.cubicTo(x.u(0.72f, 0.84f), x.u(0.28f, 0.84f), x.u(0.06f, 0.50f));
    x.p->setBrush(C::bone); x.p->setPen(QPen(dark(x.m, 200), 1.4f)); x.p->drawPath(d);
    x.ell(0.5, 0.50f, 0.16f, 0.19f, x.m, dark(x.m, 200), 1.0f);
    x.ell(0.5, 0.50f, 0.07f, 0.09f, C::voidBlack);
    x.ell(0.45f, 0.44f, 0.035f, 0.035f, QColor(255, 255, 255, 200));
}

void iconTown(const IconCtx &x)
{
    x.rect(0.10f, 0.52f, 0.34f, 0.40f, dark(x.m, 130), dark(x.m, 220), 1.0f);
    x.rect(0.56f, 0.44f, 0.34f, 0.48f, x.m, dark(x.m, 220), 1.0f);
    x.poly(QVector<QPointF>() << QPointF(0.06f, 0.52f) << QPointF(0.27f, 0.30f) << QPointF(0.48f, 0.52f), x.a, dark(x.a, 200), 1.0f);
    x.poly(QVector<QPointF>() << QPointF(0.52f, 0.44f) << QPointF(0.73f, 0.18f) << QPointF(0.94f, 0.44f), lite(x.a), dark(x.a, 200), 1.0f);
    x.rect(0.22f, 0.66f, 0.10f, 0.12f, C::lightning);
    x.rect(0.68f, 0.60f, 0.10f, 0.12f, C::lightning);
    x.rect(0.68f, 0.78f, 0.10f, 0.14f, dark(x.m, 250));
}

void iconCamp(const IconCtx &x)
{
    x.poly(QVector<QPointF>() << QPointF(0.5, 0.14f) << QPointF(0.90f, 0.80f) << QPointF(0.10f, 0.80f), dark(x.m, 120), dark(x.m, 230), 1.2f);
    x.poly(QVector<QPointF>() << QPointF(0.5, 0.14f) << QPointF(0.5, 0.80f) << QPointF(0.24f, 0.80f), x.m, dark(x.m, 230), 1.0f);
    x.poly(QVector<QPointF>() << QPointF(0.36f, 0.94f) << QPointF(0.50f, 0.68f) << QPointF(0.64f, 0.94f), C::ember);
    x.poly(QVector<QPointF>() << QPointF(0.43f, 0.94f) << QPointF(0.50f, 0.78f) << QPointF(0.57f, 0.94f), C::lightning);
}

void iconRuins(const IconCtx &x)
{
    x.rect(0.10f, 0.40f, 0.14f, 0.52f, x.m, dark(x.m, 220), 1.0f);
    x.rect(0.32f, 0.26f, 0.14f, 0.66f, lite(x.m), dark(x.m, 220), 1.0f);
    x.rect(0.56f, 0.52f, 0.14f, 0.40f, x.m, dark(x.m, 220), 1.0f);
    x.rect(0.78f, 0.34f, 0.12f, 0.58f, dark(x.m, 130), dark(x.m, 220), 1.0f);
    x.rect(0.06f, 0.86f, 0.88f, 0.08f, dark(x.m, 180), dark(x.m, 240), 1.0f);
    x.poly(QVector<QPointF>() << QPointF(0.28f, 0.26f) << QPointF(0.50f, 0.14f) << QPointF(0.50f, 0.26f), x.a);
}

void iconCave(const IconCtx &x)
{
    QPainterPath d;
    d.moveTo(x.u(0.04f, 0.92f));
    d.lineTo(x.u(0.10f, 0.40f));
    d.lineTo(x.u(0.30f, 0.16f));
    d.lineTo(x.u(0.58f, 0.10f));
    d.lineTo(x.u(0.86f, 0.30f));
    d.lineTo(x.u(0.96f, 0.92f));
    d.closeSubpath();
    x.p->setBrush(dark(x.m, 150)); x.p->setPen(QPen(dark(x.m, 240), 1.4f)); x.p->drawPath(d);
    QPainterPath m;
    m.moveTo(x.u(0.32f, 0.92f));
    m.cubicTo(x.u(0.32f, 0.46f), x.u(0.70f, 0.46f), x.u(0.70f, 0.92f));
    m.closeSubpath();
    x.p->setBrush(C::voidBlack); x.p->setPen(QPen(x.a, 1.2f)); x.p->drawPath(m);
    x.poly(QVector<QPointF>() << QPointF(0.40f, 0.52f) << QPointF(0.44f, 0.66f) << QPointF(0.36f, 0.66f), x.a);
    x.poly(QVector<QPointF>() << QPointF(0.60f, 0.52f) << QPointF(0.64f, 0.66f) << QPointF(0.56f, 0.66f), x.a);
}

void iconTree(const IconCtx &x)
{
    x.rect(0.44f, 0.58f, 0.12f, 0.36f, dark(x.a, 130), dark(x.a, 230), 1.0f);
    x.ell(0.50f, 0.34f, 0.30f, 0.26f, x.m, dark(x.m, 200), 1.2f);
    x.ell(0.32f, 0.46f, 0.17f, 0.15f, dark(x.m, 130));
    x.ell(0.68f, 0.46f, 0.17f, 0.15f, lite(x.m, 115));
    x.ell(0.50f, 0.18f, 0.16f, 0.14f, lite(x.m, 125));
}

void iconCrystal(const IconCtx &x)
{
    x.poly(QVector<QPointF>() << QPointF(0.50f, 0.04f) << QPointF(0.70f, 0.34f) << QPointF(0.60f, 0.94f)
             << QPointF(0.40f, 0.94f) << QPointF(0.30f, 0.34f), x.m, lite(x.m), 1.4f);
    x.poly(QVector<QPointF>() << QPointF(0.22f, 0.42f) << QPointF(0.34f, 0.56f) << QPointF(0.28f, 0.94f)
             << QPointF(0.16f, 0.94f), dark(x.m, 130), lite(x.m, 120), 1.0f);
    x.poly(QVector<QPointF>() << QPointF(0.80f, 0.38f) << QPointF(0.68f, 0.54f) << QPointF(0.74f, 0.94f)
             << QPointF(0.88f, 0.94f), lite(x.m, 120), lite(x.m, 160), 1.0f);
    x.line(0.50f, 0.10f, 0.50f, 0.90f, lite(x.m, 170), 1.0f);
}

void iconFortress(const IconCtx &x)
{
    x.rect(0.08f, 0.34f, 0.84f, 0.58f, x.m, dark(x.m, 230), 1.2f);
    for (int i = 0; i < 5; ++i)
        x.rect(0.08f + i * 0.18f, 0.24f, 0.10f, 0.12f, x.m, dark(x.m, 230), 1.0f);
    x.rect(0.42f, 0.56f, 0.16f, 0.36f, C::voidBlack, dark(x.m, 250), 1.0f);
    x.rect(0.20f, 0.46f, 0.10f, 0.12f, C::lightning);
    x.rect(0.70f, 0.46f, 0.10f, 0.12f, C::lightning);
    x.poly(QVector<QPointF>() << QPointF(0.50f, 0.02f) << QPointF(0.50f, 0.24f), dark(x.m, 230), 1.2f);
    x.poly(QVector<QPointF>() << QPointF(0.50f, 0.04f) << QPointF(0.72f, 0.10f) << QPointF(0.50f, 0.16f), x.a);
}

void iconShrine(const IconCtx &x)
{
    x.rect(0.22f, 0.72f, 0.56f, 0.20f, dark(x.m, 130), dark(x.m, 230), 1.0f);
    x.rect(0.30f, 0.56f, 0.40f, 0.16f, x.m, dark(x.m, 230), 1.0f);
    x.poly(QVector<QPointF>() << QPointF(0.50f, 0.06f) << QPointF(0.62f, 0.36f) << QPointF(0.50f, 0.56f)
             << QPointF(0.38f, 0.36f), x.a, lite(x.a), 1.2f);
    x.p->setPen(QPen(x.a, 1.0f));
    x.p->setBrush(Qt::NoBrush);
    x.p->drawEllipse(x.u(0.5, 0.32f), float(x.r.width()) * 0.30f, float(x.r.height()) * 0.30f);
}

void iconPortal(const IconCtx &x)
{
    QPainterPath d;
    d.moveTo(x.u(0.24f, 0.92f));
    d.cubicTo(x.u(0.10f, 0.30f), x.u(0.90f, 0.30f), x.u(0.76f, 0.92f));
    d.closeSubpath();
    x.p->setBrush(dark(x.m, 220)); x.p->setPen(QPen(x.m, 2.2f)); x.p->drawPath(d);
    x.p->setPen(QPen(x.a, 1.4f));
    x.p->drawEllipse(x.u(0.5, 0.62f), float(x.r.width()) * 0.16f, float(x.r.height()) * 0.20f);
    x.p->drawEllipse(x.u(0.5, 0.62f), float(x.r.width()) * 0.07f, float(x.r.height()) * 0.09f);
    for (int i = 0; i < 5; ++i)
        x.ell(0.22f + i * 0.14f, 0.94f, 0.03f, 0.03f, x.a);
}

void iconAnvil(const IconCtx &x)
{
    x.poly(QVector<QPointF>() << QPointF(0.08f, 0.30f) << QPointF(0.70f, 0.30f) << QPointF(0.82f, 0.42f)
             << QPointF(0.94f, 0.40f) << QPointF(0.86f, 0.54f) << QPointF(0.56f, 0.54f)
             << QPointF(0.50f, 0.42f) << QPointF(0.08f, 0.42f), x.m, dark(x.m, 230), 1.2f);
    x.poly(QVector<QPointF>() << QPointF(0.40f, 0.54f) << QPointF(0.62f, 0.54f) << QPointF(0.70f, 0.82f)
             << QPointF(0.32f, 0.82f), dark(x.m, 140), dark(x.m, 230), 1.0f);
    x.rect(0.24f, 0.82f, 0.54f, 0.10f, dark(x.m, 170), dark(x.m, 250), 1.0f);
}

void iconKey(const IconCtx &x)
{
    x.p->setPen(QPen(x.m, qMax(2.4f, float(x.r.width()) * 0.08f)));
    x.p->setBrush(Qt::NoBrush);
    x.p->drawEllipse(x.u(0.32f, 0.34f), float(x.r.width()) * 0.18f, float(x.r.height()) * 0.18f);
    x.p->setPen(QPen(x.m, qMax(2.2f, float(x.r.width()) * 0.07f), Qt::SolidLine, Qt::RoundCap));
    x.p->drawLine(x.u(0.44f, 0.46f), x.u(0.82f, 0.84f));
    x.p->drawLine(x.u(0.66f, 0.68f), x.u(0.78f, 0.56f));
    x.p->drawLine(x.u(0.76f, 0.78f), x.u(0.88f, 0.66f));
}

void iconStar(const IconCtx &x)
{
    QPolygonF pg;
    for (int i = 0; i < 10; ++i) {
        float rr = (i % 2 == 0) ? 0.46f : 0.19f;
        float a = -float(M_PI) / 2 + i * float(M_PI) / 5.f;
        pg << x.u(0.5f + qCos(a) * rr, 0.5f + qSin(a) * rr);
    }
    x.p->setBrush(x.m); x.p->setPen(QPen(lite(x.m), 1.2f)); x.p->drawPolygon(pg);
}

void iconLock(const IconCtx &x, bool open)
{
    x.rect(0.20f, 0.46f, 0.60f, 0.44f, x.m, dark(x.m, 230), 1.4f);
    x.p->setPen(QPen(lite(x.m, 130), qMax(2.2f, float(x.r.width()) * 0.07f)));
    x.p->setBrush(Qt::NoBrush);
    QPainterPath a;
    if (open) { a.moveTo(x.u(0.32f, 0.46f)); a.lineTo(x.u(0.32f, 0.28f));
                a.cubicTo(x.u(0.32f, 0.06f), x.u(0.78f, 0.06f), x.u(0.80f, 0.30f)); }
    else      { a.moveTo(x.u(0.32f, 0.46f)); a.lineTo(x.u(0.32f, 0.28f));
                a.cubicTo(x.u(0.32f, 0.06f), x.u(0.68f, 0.06f), x.u(0.68f, 0.28f));
                a.lineTo(x.u(0.68f, 0.46f)); }
    x.p->drawPath(a);
    x.ell(0.5, 0.64f, 0.06f, 0.07f, C::voidBlack);
    x.rect(0.48f, 0.68f, 0.04f, 0.12f, C::voidBlack);
}

void iconSave(const IconCtx &x)
{
    x.poly(QVector<QPointF>() << QPointF(0.14f, 0.14f) << QPointF(0.72f, 0.14f) << QPointF(0.86f, 0.28f)
             << QPointF(0.86f, 0.86f) << QPointF(0.14f, 0.86f), x.m, dark(x.m, 230), 1.3f);
    x.rect(0.28f, 0.14f, 0.36f, 0.24f, dark(x.m, 180));
    x.rect(0.26f, 0.52f, 0.48f, 0.34f, C::bone, dark(C::bone, 170), 0.9f);
    for (int i = 0; i < 3; ++i)
        x.line(0.32f, 0.60f + i * 0.08f, 0.68f, 0.60f + i * 0.08f, dark(C::boneDim, 160), 0.9f);
}

void iconSettings(const IconCtx &x)
{
    x.p->setBrush(x.m);
    x.p->setPen(QPen(dark(x.m, 230), 1.0f));
    x.p->drawPath(runeRing(x.u(0.5f, 0.5f), float(x.r.width()) * 0.44f, 8, float(x.r.width()) * 0.13f, 0.f));
    x.ell(0.5, 0.5, 0.15f, 0.15f, dark(x.m, 200), lite(x.m), 1.2f);
}

void iconPlay(const IconCtx &x)
{
    x.poly(QVector<QPointF>() << QPointF(0.24f, 0.12f) << QPointF(0.88f, 0.50f) << QPointF(0.24f, 0.88f), x.m, lite(x.m), 1.4f);
}

void iconInfo(const IconCtx &x)
{
    x.p->setPen(QPen(x.m, qMax(2.0f, float(x.r.width()) * 0.06f)));
    x.p->setBrush(Qt::NoBrush);
    x.p->drawEllipse(x.u(0.5, 0.5), float(x.r.width()) * 0.42f, float(x.r.height()) * 0.42f);
    x.rect(0.46f, 0.42f, 0.08f, 0.32f, x.m);
    x.ell(0.5, 0.30f, 0.05f, 0.05f, x.m);
}

void iconQuit(const IconCtx &x)
{
    x.p->setPen(QPen(x.m, qMax(2.0f, float(x.r.width()) * 0.06f)));
    x.p->setBrush(Qt::NoBrush);
    x.line(0.20f, 0.20f, 0.80f, 0.80f, x.m, qMax(2.4f, float(x.r.width()) * 0.09f));
    x.line(0.80f, 0.20f, 0.20f, 0.80f, x.m, qMax(2.4f, float(x.r.width()) * 0.09f));
}

void iconRespec(const IconCtx &x)
{
    x.p->setPen(QPen(x.m, qMax(2.0f, float(x.r.width()) * 0.07f), Qt::SolidLine, Qt::RoundCap));
    x.p->setBrush(Qt::NoBrush);
    QPainterPath a;
    a.moveTo(x.u(0.50f + 0.38f * 0.866f, 0.50f - 0.38f * 0.5f));
    a.arcTo(x.ur(0.12f, 0.12f, 0.76f, 0.76f), 30, 280);
    x.p->drawPath(a);
    x.poly(QVector<QPointF>() << QPointF(0.80f, 0.18f) << QPointF(0.98f, 0.34f) << QPointF(0.72f, 0.42f), x.a);
}

void iconBag(const IconCtx &x)
{
    QPainterPath d;
    d.moveTo(x.u(0.20f, 0.36f));
    d.lineTo(x.u(0.80f, 0.36f));
    d.lineTo(x.u(0.90f, 0.90f));
    d.lineTo(x.u(0.10f, 0.90f));
    d.closeSubpath();
    x.p->setBrush(x.m); x.p->setPen(QPen(dark(x.m, 230), 1.4f)); x.p->drawPath(d);
    x.p->setPen(QPen(x.a, qMax(1.8f, float(x.r.width()) * 0.06f)));
    x.p->setBrush(Qt::NoBrush);
    QPainterPath h; h.moveTo(x.u(0.70f, 0.30f)); h.arcTo(x.ur(0.30f, 0.08f, 0.40f, 0.44f), 0, 180); x.p->drawPath(h);
    x.rect(0.18f, 0.34f, 0.64f, 0.08f, x.a, dark(x.a, 200), 0.8f);
}

void iconWeb(const IconCtx &x)
{
    x.p->setPen(QPen(x.m, 1.2f));
    x.p->setBrush(Qt::NoBrush);
    for (int i = 0; i < 8; ++i) {
        float a = i * float(M_PI) / 4.f;
        x.p->drawLine(x.u(0.5f, 0.5f), x.u(0.5f + qCos(a) * 0.48f, 0.5f + qSin(a) * 0.48f));
    }
    for (int r = 1; r <= 3; ++r) {
        QPainterPath pp;
        float rad = 0.16f * r;
        for (int i = 0; i <= 8; ++i) {
            float a = i * float(M_PI) / 4.f;
            QPointF pt = x.u(0.5f + qCos(a) * rad, 0.5f + qSin(a) * rad);
            if (i == 0) pp.moveTo(pt); else pp.lineTo(pt);
        }
        x.p->drawPath(pp);
    }
    x.ell(0.5, 0.5, 0.07f, 0.07f, x.a);
}

void iconMap(const IconCtx &x)
{
    x.poly(QVector<QPointF>() << QPointF(0.06f, 0.24f) << QPointF(0.36f, 0.14f) << QPointF(0.64f, 0.24f)
             << QPointF(0.94f, 0.14f) << QPointF(0.94f, 0.78f) << QPointF(0.64f, 0.88f)
             << QPointF(0.36f, 0.78f) << QPointF(0.06f, 0.88f), x.m, dark(x.m, 230), 1.3f);
    x.line(0.36f, 0.14f, 0.36f, 0.78f, dark(x.m, 200), 1.0f);
    x.line(0.64f, 0.24f, 0.64f, 0.88f, dark(x.m, 200), 1.0f);
    x.polyLine(QVector<QPointF>() << QPointF(0.16f, 0.62f) << QPointF(0.30f, 0.48f) << QPointF(0.46f, 0.56f)
                   << QPointF(0.60f, 0.40f) << QPointF(0.80f, 0.48f), x.a, 1.4f);
    x.ell(0.80f, 0.48f, 0.045f, 0.045f, C::bloodHi);
}

void iconSwords(const IconCtx &x)
{
    x.line(0.16f, 0.84f, 0.72f, 0.16f, lite(x.m), qMax(2.4f, float(x.r.width()) * 0.09f));
    x.line(0.84f, 0.84f, 0.28f, 0.16f, x.m, qMax(2.4f, float(x.r.width()) * 0.09f));
    x.line(0.08f, 0.66f, 0.28f, 0.86f, x.a, qMax(2.0f, float(x.r.width()) * 0.07f));
    x.line(0.92f, 0.66f, 0.72f, 0.86f, x.a, qMax(2.0f, float(x.r.width()) * 0.07f));
    x.poly(QVector<QPointF>() << QPointF(0.76f, 0.10f) << QPointF(0.86f, 0.20f) << QPointF(0.72f, 0.24f), lite(x.m));
    x.poly(QVector<QPointF>() << QPointF(0.24f, 0.10f) << QPointF(0.14f, 0.20f) << QPointF(0.28f, 0.24f), x.m);
}

void iconFlask(const IconCtx &x)
{
    x.rect(0.44f, 0.08f, 0.12f, 0.20f, dark(x.a, 140), dark(x.a, 230), 1.0f);
    QPainterPath b;
    b.moveTo(x.u(0.44f, 0.28f));
    b.lineTo(x.u(0.20f, 0.70f));
    b.cubicTo(x.u(0.16f, 0.92f), x.u(0.84f, 0.92f), x.u(0.80f, 0.70f));
    b.lineTo(x.u(0.56f, 0.28f));
    b.closeSubpath();
    x.p->setBrush(QColor(220, 235, 245, 50)); x.p->setPen(QPen(C::boneDim, 1.4f)); x.p->drawPath(b);
    x.p->save(); x.p->setClipPath(b);
    x.p->fillRect(x.ur(0.16f, 0.56f, 0.70f, 0.40f), x.m);
    x.p->restore();
    x.ell(0.40f, 0.72f, 0.04f, 0.04f, QColor(255,255,255,110));
    x.ell(0.58f, 0.64f, 0.03f, 0.03f, QColor(255,255,255,90));
}

void iconHammer(const IconCtx &x)
{
    x.rect(0.45f, 0.40f, 0.10f, 0.56f, dark(x.a, 120), dark(x.a, 230), 1.0f);
    x.poly(QVector<QPointF>() << QPointF(0.18f, 0.16f) << QPointF(0.82f, 0.16f) << QPointF(0.86f, 0.40f)
             << QPointF(0.14f, 0.40f), x.m, dark(x.m, 230), 1.3f);
    x.rect(0.24f, 0.20f, 0.52f, 0.06f, lite(x.m, 140));
}

void iconCrown2(const IconCtx &x)
{
    x.poly(QVector<QPointF>() << QPointF(0.08f, 0.76f) << QPointF(0.12f, 0.30f) << QPointF(0.30f, 0.52f)
             << QPointF(0.50f, 0.18f) << QPointF(0.70f, 0.52f) << QPointF(0.88f, 0.30f)
             << QPointF(0.92f, 0.76f), x.m, dark(x.m, 230), 1.3f);
    x.rect(0.08f, 0.76f, 0.84f, 0.12f, lite(x.m, 120), dark(x.m, 230), 1.0f);
    x.ell(0.50f, 0.30f, 0.05f, 0.05f, C::bloodHi);
    x.ell(0.24f, 0.60f, 0.04f, 0.04f, C::frost);
    x.ell(0.76f, 0.60f, 0.04f, 0.04f, C::poison);
}

void iconSpawner(const IconCtx &x, int kind)
{
    if (kind == 0) { // паук
        x.ell(0.5, 0.56f, 0.20f, 0.18f, x.m, dark(x.m, 230), 1.2f);
        x.ell(0.5, 0.34f, 0.11f, 0.10f, lite(x.m, 120), dark(x.m, 230), 1.0f);
        x.ell(0.45f, 0.32f, 0.03f, 0.03f, x.a);
        x.ell(0.55f, 0.32f, 0.03f, 0.03f, x.a);
        for (int i = 0; i < 4; ++i) {
            float y = 0.44f + i * 0.09f;
            x.polyLine(QVector<QPointF>() << QPointF(0.32f, y) << QPointF(0.12f, y - 0.10f) << QPointF(0.06f, y + 0.10f), dark(x.m, 200), 1.4f);
            x.polyLine(QVector<QPointF>() << QPointF(0.68f, y) << QPointF(0.88f, y - 0.10f) << QPointF(0.94f, y + 0.10f), dark(x.m, 200), 1.4f);
        }
    } else if (kind == 1) { // волк
        x.poly(QVector<QPointF>() << QPointF(0.16f, 0.62f) << QPointF(0.30f, 0.38f) << QPointF(0.36f, 0.20f)
                 << QPointF(0.46f, 0.36f) << QPointF(0.60f, 0.34f) << QPointF(0.66f, 0.18f)
                 << QPointF(0.74f, 0.40f) << QPointF(0.90f, 0.62f) << QPointF(0.72f, 0.78f)
                 << QPointF(0.30f, 0.78f), x.m, dark(x.m, 230), 1.2f);
        x.ell(0.42f, 0.52f, 0.035f, 0.035f, x.a);
        x.ell(0.58f, 0.52f, 0.035f, 0.035f, x.a);
        x.poly(QVector<QPointF>() << QPointF(0.44f, 0.64f) << QPointF(0.56f, 0.64f) << QPointF(0.50f, 0.70f), C::bone);
    } else if (kind == 2) { // кость
        x.p->setPen(QPen(x.m, qMax(3.0f, float(x.r.width()) * 0.10f), Qt::SolidLine, Qt::RoundCap));
        x.p->drawLine(x.u(0.24f, 0.72f), x.u(0.76f, 0.28f));
        x.ell(0.20f, 0.76f, 0.09f, 0.09f, x.m);
        x.ell(0.30f, 0.82f, 0.07f, 0.07f, x.m);
        x.ell(0.80f, 0.24f, 0.09f, 0.09f, x.m);
        x.ell(0.70f, 0.18f, 0.07f, 0.07f, x.m);
    } else { // вулкан
        x.poly(QVector<QPointF>() << QPointF(0.06f, 0.92f) << QPointF(0.34f, 0.40f) << QPointF(0.66f, 0.40f)
                 << QPointF(0.94f, 0.92f), dark(x.m, 140), dark(x.m, 240), 1.2f);
        x.poly(QVector<QPointF>() << QPointF(0.34f, 0.40f) << QPointF(0.66f, 0.40f) << QPointF(0.60f, 0.48f)
                 << QPointF(0.40f, 0.48f), C::ember);
        x.poly(QVector<QPointF>() << QPointF(0.44f, 0.36f) << QPointF(0.50f, 0.08f) << QPointF(0.56f, 0.36f), C::ember);
        x.ell(0.50f, 0.14f, 0.05f, 0.05f, C::lightning);
    }
}

void iconArrowDir(const IconCtx &x, int dir)
{
    if (dir == 0) { // вверх
        x.poly(QVector<QPointF>() << QPointF(0.50f, 0.10f) << QPointF(0.86f, 0.56f) << QPointF(0.62f, 0.56f)
                 << QPointF(0.62f, 0.90f) << QPointF(0.38f, 0.90f) << QPointF(0.38f, 0.56f)
                 << QPointF(0.14f, 0.56f), x.m, lite(x.m), 1.2f);
    } else {
        x.poly(QVector<QPointF>() << QPointF(0.10f, 0.50f) << QPointF(0.56f, 0.14f) << QPointF(0.56f, 0.38f)
                 << QPointF(0.90f, 0.38f) << QPointF(0.90f, 0.62f) << QPointF(0.56f, 0.62f)
                 << QPointF(0.56f, 0.86f), x.m, lite(x.m), 1.2f);
    }
}

void iconCheck(const IconCtx &x, bool ok)
{
    if (ok) {
        x.polyLine(QVector<QPointF>() << QPointF(0.14f, 0.54f) << QPointF(0.40f, 0.80f) << QPointF(0.88f, 0.20f), x.m, qMax(3.0f, float(x.r.width()) * 0.12f));
    } else {
        x.line(0.18f, 0.18f, 0.82f, 0.82f, x.m, qMax(3.0f, float(x.r.width()) * 0.12f));
        x.line(0.82f, 0.18f, 0.18f, 0.82f, x.m, qMax(3.0f, float(x.r.width()) * 0.12f));
    }
}

void iconLevelUp(const IconCtx &x)
{
    x.poly(QVector<QPointF>() << QPointF(0.50f, 0.06f) << QPointF(0.90f, 0.50f) << QPointF(0.66f, 0.50f)
             << QPointF(0.66f, 0.68f) << QPointF(0.34f, 0.68f) << QPointF(0.34f, 0.50f)
             << QPointF(0.10f, 0.50f), x.m, lite(x.m), 1.2f);
    x.poly(QVector<QPointF>() << QPointF(0.20f, 0.92f) << QPointF(0.50f, 0.74f) << QPointF(0.80f, 0.92f), x.a, lite(x.a), 1.0f);
}

void iconLoad(const IconCtx &x)
{
    x.rect(0.14f, 0.44f, 0.72f, 0.44f, x.m, dark(x.m, 230), 1.2f);
    x.poly(QVector<QPointF>() << QPointF(0.14f, 0.44f) << QPointF(0.34f, 0.44f) << QPointF(0.42f, 0.56f)
             << QPointF(0.86f, 0.56f), lite(x.m, 130), dark(x.m, 230), 1.0f);
    x.poly(QVector<QPointF>() << QPointF(0.50f, 0.08f) << QPointF(0.70f, 0.30f) << QPointF(0.58f, 0.30f)
             << QPointF(0.58f, 0.44f) << QPointF(0.42f, 0.44f) << QPointF(0.42f, 0.30f)
             << QPointF(0.30f, 0.30f), x.a, lite(x.a), 1.0f);
}

} // anonymous namespace

// ============================================================
//  Отрисовка глифа по id
// ============================================================
void drawGlyph(QPainter *p, int glyph, const QRectF &rect, const QColor &main, const QColor &accent, float glowAmt)
{
    if (glyph == Gl_None || rect.width() <= 1 || rect.height() <= 1) return;
    QColor acc = accent.isValid() ? accent : C::edgeHi;

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setRenderHint(QPainter::SmoothPixmapTransform, true);
    if (glowAmt > 0.01f) Style::glow(p, rect.center(), qMax(rect.width(), rect.height()) * 0.62f, main, glowAmt);

    IconCtx x; x.p = p; x.r = rect; x.m = main; x.a = acc;

    switch (glyph) {
    case Gl_Sword:       iconSword(x, false); break;
    case Gl_GreatSword:  iconSword(x, true); break;
    case Gl_Axe:         iconAxe(x, false); break;
    case Gl_GreatAxe:    iconAxe(x, true); break;
    case Gl_Dagger:      iconDagger(x); break;
    case Gl_Mace:        iconMace(x); break;
    case Gl_Spear:       iconSpear(x); break;
    case Gl_Bow:         iconBow(x); break;
    case Gl_Staff:       iconStaff(x, false); break;
    case Gl_Wand:        iconStaff(x, true); break;
    case Gl_Tome:        iconTome(x); break;
    case Gl_Shield:      iconShield(x); break;
    case Gl_Torch:       iconTorch(x); break;
    case Gl_Totem:       iconTotem(x); break;
    case Gl_Helm:        iconHelm(x, 0); break;
    case Gl_Hood:        iconHelm(x, 1); break;
    case Gl_Crown:       iconHelm(x, 2); break;
    case Gl_Chest:       iconChest(x, 0); break;
    case Gl_Plate:       iconChest(x, 2); break;
    case Gl_Robe:        iconChest(x, 1); break;
    case Gl_Gloves:      iconGloves(x); break;
    case Gl_Belt:        iconBelt(x); break;
    case Gl_Boots:       iconBoots(x); break;
    case Gl_Amulet:      iconAmulet(x); break;
    case Gl_Ring:        iconRing(x); break;
    case Gl_GemRed:      iconGem(x, 0); break;
    case Gl_GemBlue:     iconGem(x, 1); break;
    case Gl_GemYellow:   iconGem(x, 0); break;
    case Gl_GemGreen:    iconGem(x, 1); break;
    case Gl_GemViolet:   iconGem(x, 0); break;
    case Gl_PotionHp:    iconPotion(x, C::hp); break;
    case Gl_PotionMp:    iconPotion(x, C::mana); break;
    case Gl_PotionXp:    iconPotion(x, C::xp); break;
    case Gl_Scroll:      iconScroll(x); break;
    case Gl_Coin:        iconCoin(x, false); break;
    case Gl_Coins:       iconCoin(x, true); break;
    case Gl_Cleave:      iconCleave(x); break;
    case Gl_Spin:        iconSpin(x); break;
    case Gl_Dash:        iconDash(x); break;
    case Gl_Fireball:    iconFireball(x); break;
    case Gl_Meteor:      iconMeteor(x); break;
    case Gl_IceShard:    iconIceShard(x); break;
    case Gl_Chain:       iconChain(x); break;
    case Gl_Nova:        iconNova(x); break;
    case Gl_Poison:      iconPoison(x); break;
    case Gl_Summon:      iconSummon(x); break;
    case Gl_Ward:        iconWard(x); break;
    case Gl_Heal:        iconHeal(x); break;
    case Gl_MultiArrow:  iconMultiArrow(x); break;
    case Gl_Quake:       iconQuake(x); break;
    case Gl_Mark:        iconMark(x); break;
    case Gl_Blood:       iconBlood(x); break;
    case Gl_Curse:       iconCurse(x); break;
    case Gl_Blink:       iconBlink(x); break;
    case Gl_Burn:        iconStatus(x, 0); break;
    case Gl_Chill:       iconStatus(x, 1); break;
    case Gl_Shock:       iconStatus(x, 2); break;
    case Gl_PoisonDot:   iconStatus(x, 3); break;
    case Gl_ShieldDot:   iconStatus(x, 4); break;
    case Gl_Haste:       iconStatus(x, 5); break;
    case Gl_MarkDot:     iconMark(x); break;
    case Gl_Skull:       iconSkull(x); break;
    case Gl_Heart:       iconHeart(x); break;
    case Gl_Drop:        iconDrop(x); break;
    case Gl_Eye:         iconEye(x); break;
    case Gl_Town:        iconTown(x); break;
    case Gl_Camp:        iconCamp(x); break;
    case Gl_Ruins:       iconRuins(x); break;
    case Gl_Cave:        iconCave(x); break;
    case Gl_Forest:      iconTree(x); break;
    case Gl_Swamp:       iconPoison(x); break;
    case Gl_Fortress:    iconFortress(x); break;
    case Gl_Shrine:      iconShrine(x); break;
    case Gl_Elite:       iconSwords(x); break;
    case Gl_Boss:        iconSkull(x); break;
    case Gl_Portal:      iconPortal(x); break;
    case Gl_Anvil:       iconAnvil(x); break;
    case Gl_Key:         iconKey(x); break;
    case Gl_Tree:        iconTree(x); break;
    case Gl_Crystal:     iconCrystal(x); break;
    case Gl_Bone:        iconSpawner(x, 2); break;
    case Gl_Spider:      iconSpawner(x, 0); break;
    case Gl_Wolf:        iconSpawner(x, 1); break;
    case Gl_Volcano:     iconSpawner(x, 3); break;
    case Gl_Star:        iconStar(x); break;
    case Gl_Lock:        iconLock(x, false); break;
    case Gl_Unlock:      iconLock(x, true); break;
    case Gl_Save:        iconSave(x); break;
    case Gl_Load:        iconLoad(x); break;
    case Gl_Settings:    iconSettings(x); break;
    case Gl_Play:        iconPlay(x); break;
    case Gl_Info:        iconInfo(x); break;
    case Gl_Quit:        iconQuit(x); break;
    case Gl_Respec:    iconRespec(x); break;
    case Gl_LevelUp:     iconLevelUp(x); break;
    case Gl_ArrowUp:     iconArrowDir(x, 0); break;
    case Gl_ArrowRight:  iconArrowDir(x, 1); break;
    case Gl_Check:       iconCheck(x, true); break;
    case Gl_Cross:       iconCheck(x, false); break;
    case Gl_Bag:         iconBag(x); break;
    case Gl_Web:         iconWeb(x); break;
    case Gl_Map:         iconMap(x); break;
    case Gl_Swords:      iconSwords(x); break;
    case Gl_Crown2:      iconCrown2(x); break;
    case Gl_Flask:       iconFlask(x); break;
    case Gl_Hammer:      iconHammer(x); break;
    default:             x.ell(0.5f, 0.5f, 0.3f, 0.3f, main); break;
    }
    p->restore();
}

QColor glyphTint(int glyph)
{
    switch (glyph) {
    case Gl_Fireball: case Gl_Meteor: case Gl_Burn: return C::ember;
    case Gl_IceShard: case Gl_Chill: return C::frost;
    case Gl_Chain: case Gl_Shock: return C::lightning;
    case Gl_Poison: case Gl_PoisonDot: return C::poison;
    case Gl_Curse: case Gl_Blink: case Gl_Mark: return C::arcane;
    case Gl_Heal: case Gl_Heart: return C::good;
    case Gl_Coin: case Gl_Coins: return C::gold;
    default: return C::bone;
    }
}

} // namespace aa
