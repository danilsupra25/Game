#ifndef AA_RNG_H
#define AA_RNG_H

#include <QtGlobal>
#include <QString>

namespace aa {

// Собственный детерминированный ГПСЧ (xorshift64*).
// Не зависит от qrand()/QRandomGenerator (последнего нет в Qt 5.9),
// состояние можно сохранять в сейв -> воспроизводимая генерация мира.
class Rng
{
public:
    explicit Rng(quint64 seed = 0x9E3779B97F4A7C15ull);

    void seed(quint64 s);
    quint64 state() const { return m_s; }
    void setState(quint64 s) { m_s = s ? s : 1ull; }

    quint32 next();                      // 32 бита
    int  range(int lo, int hi);          // включительно
    float frange(float lo, float hi);
    float unit();                        // [0,1)
    bool chance(float p);                // p в долях
    int  pick(int n);                    // 0..n-1
    float gauss();                       // примерно нормальное [-1,1]

    // выбор из взвешенного списка
    int weighted(const QVector<int> &weights);

private:
    quint64 m_s;
};

// Глобальный ГПСЧ игры
Rng &rng();

} // namespace aa

#endif // AA_RNG_H
