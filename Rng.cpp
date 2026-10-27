#include "Core/Rng.h"

#include <QtMath>
#include <QDateTime>
#include <QVector>

namespace aa {

Rng::Rng(quint64 s) { seed(s); }

void Rng::seed(quint64 s)
{
    m_s = s ? s : 0x9E3779B97F4A7C15ull;
    // прогрев
    for (int i = 0; i < 16; ++i) next();
}

quint32 Rng::next()
{
    quint64 x = m_s;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    m_s = x;
    return quint32((x * 2685821657736338717ull) >> 32);
}

int Rng::range(int lo, int hi)
{
    if (hi <= lo) return lo;
    return lo + int(next() % quint32(hi - lo + 1));
}

float Rng::unit() { return float(next() & 0xFFFFFF) / float(0x1000000); }

float Rng::frange(float lo, float hi) { return lo + unit() * (hi - lo); }

bool Rng::chance(float p) { return unit() < p; }

int Rng::pick(int n) { return n <= 0 ? 0 : int(next() % quint32(n)); }

float Rng::gauss()
{
    // сумма 4 равномерных -> колоколообразно
    float s = unit() + unit() + unit() + unit();
    return (s - 2.0f) * 0.7f;
}

int Rng::weighted(const QVector<int> &w)
{
    int total = 0;
    for (int i = 0; i < w.size(); ++i) total += qMax(0, w[i]);
    if (total <= 0) return w.isEmpty() ? -1 : pick(w.size());
    int r = range(0, total - 1);
    for (int i = 0; i < w.size(); ++i) {
        r -= qMax(0, w[i]);
        if (r < 0) return i;
    }
    return w.size() - 1;
}

Rng &rng()
{
    static Rng g(quint64(QDateTime::currentMSecsSinceEpoch()) ^ 0xA53C9E17ull);
    return g;
}

} // namespace aa
