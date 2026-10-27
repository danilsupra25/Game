#include "Core/Audio.h"
#include "Core/Rng.h"

#include <QtMath>
#include <QIODevice>
#include <QDir>
#include <QUrl>
#include <QSettings>
#include <QCoreApplication>
#include <QDateTime>
#ifdef HAVE_AUDIO
#include <QSoundEffect>
#include <QMediaPlayer>
#endif

namespace aa {

// ============================================================
//  Синтезатор игровых эффектов (16 бит, 22050 Гц, моно).
//  Никаких внешних аудиофайлов - всё считается на лету.
// ============================================================
static const int SR = 22050;

Audio &Audio::inst()
{
    static Audio a;
    return a;
}

Audio::Audio()
#ifdef HAVE_AUDIO
    : m_music(0), m_musicOn(false), m_mediaNext(0), m_musicOut(0), m_musicIo(0),
      m_musicWav(false), m_quickStops(0), m_musicStartedMs(0)
#endif
{
}
Audio::~Audio() { shutdown(); }

void Audio::init()
{
    {
        QSettings st(QStringLiteral("AshAmber"), QStringLiteral("PepeL_i_Yantar"));
        m_volume = qBound(0.f, float(st.value(QStringLiteral("volume"), 0.65).toDouble()), 1.f);
    }
#ifdef HAVE_AUDIO
    if (m_inited) return;
    m_format.setSampleRate(SR);
    m_format.setChannelCount(1);
    m_format.setSampleSize(16);
    m_format.setCodec(QStringLiteral("audio/pcm"));
    m_format.setByteOrder(QAudioFormat::LittleEndian);
    m_format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info = QAudioDeviceInfo::defaultOutputDevice();
    if (!info.isFormatSupported(m_format)) m_format = info.nearestFormat(m_format);

    m_voices.clear();
    m_clock.start();
    m_voices.reserve(8);
    for (int i = 0; i < 8; ++i) {
        Voice v;
        v.out = new QAudioOutput(m_format, 0);
        v.out->setVolume(m_volume);
        v.buf = new QBuffer();
        m_voices.append(v);
    }
    m_inited = true;
#endif
}

void Audio::shutdown()
{
#ifdef HAVE_AUDIO
    for (int i = 0; i < m_voices.size(); ++i) {
        if (m_voices[i].out) { m_voices[i].out->stop(); delete m_voices[i].out; m_voices[i].out = 0; }
        if (m_voices[i].buf) { delete m_voices[i].buf; m_voices[i].buf = 0; }
    }
    m_voices.clear();
    for (int i = 0; i < m_mediaPool.size(); ++i) delete m_mediaPool[i];
    m_mediaPool.clear();
    m_mediaOwner.clear();
    if (m_musicOut) { m_musicOut->stop(); delete m_musicOut; m_musicOut = 0; }
    if (m_musicIo) { delete m_musicIo; m_musicIo = 0; }
    m_musicData.clear();
    if (m_music) { m_music->stop(); delete m_music; m_music = 0; }
    m_musicOn = false;
    m_inited = false;
#endif
}

void Audio::setEnabled(bool on)
{
    m_enabled = on;
    if (!on) { stopAll(); stopMusic(); }
}

void Audio::setVolume(float v)
{
    m_volume = qBound(0.f, v, 1.f);
    QSettings st(QStringLiteral("AshAmber"), QStringLiteral("PepeL_i_Yantar"));
    st.setValue(QStringLiteral("volume"), double(m_volume));
#ifdef HAVE_AUDIO
    for (int i = 0; i < m_voices.size(); ++i) if (m_voices[i].out) m_voices[i].out->setVolume(m_volume);
    for (int i = 0; i < m_mediaPool.size(); ++i) if (m_mediaPool[i]) m_mediaPool[i]->setVolume(int(m_volume * 80));
    if (m_music) m_music->setVolume(int(m_volume * 80));
    if (m_musicOut) m_musicOut->setVolume(m_volume);
#endif
}

void Audio::stopAll()
{
#ifdef HAVE_AUDIO
    for (int i = 0; i < m_voices.size(); ++i) {
        if (m_voices[i].out) m_voices[i].out->stop();
        m_voices[i].busy = false;
    }
#endif
}

QString Audio::mappingFor(const QString &slot)
{
    QDir dir(assetsDir());
    QString mp = dir.absoluteFilePath(QStringLiteral("mapping.txt"));
    QFile f(mp);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    while (!f.atEnd()) {
        QString line = QString::fromUtf8(f.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) continue;
        int eq = line.indexOf(QLatin1Char('='));
        if (eq <= 0) continue;
        QString key = line.left(eq).trimmed().toLower();
        QString val = line.mid(eq + 1).trimmed();
        if (key == slot.toLower()) {
            QFileInfo fi(val);
            if (!fi.isAbsolute()) fi = QFileInfo(dir.absoluteFilePath(val));
            if (fi.exists()) return fi.absoluteFilePath();
        }
    }
    return QString();
}

// минимальный интервал между повторами одного звука, сек
static float minInterval(int sfx)
{
    switch (sfx) {
    case Sfx_Hit:   return 0.085f;
    case Sfx_Crit:  return 0.10f;
    case Sfx_Swing: return 0.06f;
    case Sfx_Kill:  return 0.06f;
    case Sfx_Hurt:  return 0.12f;
    case Sfx_Shock: return 0.05f;
    case Sfx_Fire:  return 0.05f;
    default:        return 0.03f;
    }
}

void Audio::play(int sfx)
{
#ifdef HAVE_AUDIO
    if (!m_enabled) return;
    if (!m_inited) init();
    if (!m_inited) return;

    // ленивый рескан: файлы могли положить после запуска (не чаще раза в 2 сек)
    {
        static float lastScan = -10.f;
        float tnow = float(m_clock.elapsed()) / 1000.f;
        if (!m_assetBuf.contains(sfx) && !m_mediaAsset.contains(sfx) && tnow - lastScan > 2.f) {
            lastScan = tnow;
            initAssets();
        }
    }

    // не допускаем наслоения одинаковых звуков в один момент
    float now = float(m_clock.elapsed()) / 1000.f;
    float last = m_lastPlay.value(sfx, -10.f);
    if (now - last < minInterval(sfx)) return;
    m_lastPlay.insert(sfx, now);

    // 1) wav, разобранный в свой конвейер - играет всегда, как синтез
    if (m_assetBuf.contains(sfx)) {
        int freeIdx = -1;
        for (int i = 0; i < m_voices.size(); ++i)
            if (m_voices[i].out && m_voices[i].out->state() != QAudio::ActiveState) { freeIdx = i; break; }
        if (freeIdx >= 0) {
            Voice &v = m_voices[freeIdx];
            v.busy = true;
            v.buf->close();
            v.buf->setData(m_assetBuf.value(sfx));
            if (v.buf->open(QIODevice::ReadOnly)) { v.out->stop(); v.out->start(v.buf); }
        }
        return;
    }
    // 2) сжатые форматы - через пул медиаплееров, с фолбэком на синтез при ошибке
    if (m_mediaAsset.contains(sfx) && !m_broken.contains(sfx)) {
        if (m_mediaPool.isEmpty()) {
            for (int i = 0; i < 4; ++i) {
                QMediaPlayer *mp = new QMediaPlayer();
                mp->setVolume(int(m_volume * 80));
                QObject::connect(mp, static_cast<void (QMediaPlayer::*)(QMediaPlayer::Error)>(&QMediaPlayer::error),
                                 [this, mp](QMediaPlayer::Error) {
                    int owner = m_mediaOwner.value(mp, -1);
                    if (owner >= 0) m_broken.insert(owner);
                });
                m_mediaPool.append(mp);
            }
        }
        QMediaPlayer *mp = 0;
        for (int i = 0; i < m_mediaPool.size(); ++i)
            if (m_mediaOwner.value(m_mediaPool[i], -1) == sfx) { mp = m_mediaPool[i]; break; }
        if (!mp) { mp = m_mediaPool.at(m_mediaNext % m_mediaPool.size()); ++m_mediaNext; }
        m_mediaOwner.insert(mp, sfx);
        mp->stop();
        mp->setMedia(QUrl::fromLocalFile(m_mediaAsset.value(sfx)));
        mp->play();
        return;
    }
    int freeIdx = -1;
    for (int i = 0; i < m_voices.size(); ++i) {
        Voice &v = m_voices[i];
        if (!v.out) continue;
        QAudio::State st = v.out->state();
        if (st != QAudio::ActiveState) { freeIdx = i; break; }
    }
    if (freeIdx < 0) return;
    Voice &v = m_voices[freeIdx];
    QByteArray data = synth(sfx);
    if (data.isEmpty()) return;
    v.busy = true;
    v.buf->close();
    v.buf->setData(data);
    if (!v.buf->open(QIODevice::ReadOnly)) return;
    v.out->stop();
    v.out->start(v.buf);
#else
    Q_UNUSED(sfx);
#endif
}

// ============================================================
//  Генерация PCM
// ============================================================
namespace {

struct Wav
{
    QVector<float> s;
    explicit Wav(float secs) { s.resize(qMax(1, int(float(SR) * secs))); }
    int n() const { return s.size(); }
    float &at(int i) { return s[i]; }
    float t(int i) const { return float(i) / float(SR); }
    float env(int i) const
    {
        float tt = t(i);
        float total = float(n()) / float(SR);
        float a = tt < 0.006f ? tt / 0.006f : 1.f;
        float d = total > 0.02f ? qMax(0.f, 1.f - (tt - 0.006f) / (total - 0.006f)) : 1.f;
        return a * d;
    }

    QByteArray toWav() const
    {
        const int bytes = n() * 2;
        QByteArray out;
        out.reserve(44 + bytes);
        struct Put {
            QByteArray &b;
            explicit Put(QByteArray &bb) : b(bb) {}
            void u32(quint32 v)
            {
                b.append(char(v & 0xFF)); b.append(char((v >> 8) & 0xFF));
                b.append(char((v >> 16) & 0xFF)); b.append(char((v >> 24) & 0xFF));
            }
            void u16(quint16 v) { b.append(char(v & 0xFF)); b.append(char((v >> 8) & 0xFF)); }
            void str(const char *txt) { b.append(txt, 4); }
        } put(out);
        put.str("RIFF"); put.u32(quint32(36 + bytes)); put.str("WAVE");
        put.str("fmt "); put.u32(16); put.u16(1); put.u16(1);
        put.u32(quint32(SR)); put.u32(quint32(SR * 2)); put.u16(2); put.u16(16);
        put.str("data"); put.u32(quint32(bytes));
        for (int i = 0; i < n(); ++i) {
            float v = qBound(-1.f, s.at(i), 1.f);
            put.u16(quint16(int(v * 30000.f)));
        }
        return out;
    }
};

inline float nz(Rng &r) { return r.unit() * 2.f - 1.f; }
const float TAU = 6.2831853f;

} // anonymous

QByteArray Audio::synth(int sfx)
{
    Rng r(quint64(sfx) * 7919ull + 13ull);
    Wav w(0.2f);

    switch (sfx) {
    case Sfx_Swing: {
        Wav b(0.16f);
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            b.at(i) = nz(r) * e * 0.34f * qMax(0.f, 1.f - tt * 7.f) + qSin(tt * (700.f - tt * 2600.f) * TAU) * e * 0.07f;
        }
        return b.toWav();
    }
    case Sfx_Hit: {
        Wav b(0.14f);
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            b.at(i) = (nz(r) * 0.45f + qSin(tt * 140.f * TAU) * 0.55f) * e * qMax(0.f, 1.f - tt * 9.f);
        }
        return b.toWav();
    }
    case Sfx_Crit: {
        Wav b(0.26f);
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            float f = 900.f * qExp(-tt * 6.f) + 90.f;
            b.at(i) = (nz(r) * 0.35f + qSin(tt * f * TAU) * 0.6f + qSin(tt * f * 2.01f * TAU) * 0.22f) * e;
        }
        return b.toWav();
    }
    case Sfx_Hurt: {
        Wav b(0.22f);
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            float f = 260.f - tt * 380.f;
            b.at(i) = (qSin(tt * f * TAU) * 0.45f + nz(r) * 0.26f) * e * qMax(0.f, 1.f - tt * 4.f);
        }
        return b.toWav();
    }
    case Sfx_Cast: {
        Wav b(0.30f);
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            float f = 300.f + tt * 1400.f;
            b.at(i) = (qSin(tt * f * TAU) * 0.40f + qSin(tt * f * 1.5f * TAU) * 0.16f) * e;
        }
        return b.toWav();
    }
    case Sfx_Fire:
    case Sfx_Explosion: {
        Wav b(sfx == Sfx_Explosion ? 0.55f : 0.34f);
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            float f = 180.f * qExp(-tt * 5.f) + 40.f;
            b.at(i) = (nz(r) * 0.6f * qMax(0.f, 1.f - tt * 2.4f) + qSin(tt * f * TAU) * 0.45f) * e;
        }
        return b.toWav();
    }
    case Sfx_Ice: {
        Wav b(0.28f);
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            float f = 2400.f - tt * 1500.f;
            b.at(i) = (qSin(tt * f * TAU) * 0.26f + qSin(tt * f * 2.7f * TAU) * 0.16f
                       + nz(r) * 0.16f * qMax(0.f, 1.f - tt * 6.f)) * e;
        }
        return b.toWav();
    }
    case Sfx_Shock: {
        Wav b(0.22f);
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            float sq = qSin(tt * 620.f * TAU) > 0 ? 0.45f : -0.45f;
            b.at(i) = (sq + nz(r) * 0.22f) * e * qMax(0.f, 1.f - tt * 6.f);
        }
        return b.toWav();
    }
    case Sfx_Poison: {
        Wav b(0.32f);
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            b.at(i) = (nz(r) * 0.26f * (0.5f + 0.5f * qSin(tt * 22.f)) + qSin(tt * 90.f * TAU) * 0.16f) * e;
        }
        return b.toWav();
    }
    case Sfx_Heal: {
        Wav b(0.45f);
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            float f = 520.f + qSin(tt * 6.f) * 90.f;
            b.at(i) = (qSin(tt * f * TAU) * 0.26f + qSin(tt * f * 2.f * TAU) * 0.12f) * e;
        }
        return b.toWav();
    }
    case Sfx_Shield: {
        Wav b(0.34f);
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            b.at(i) = (qSin(tt * (330.f + tt * 260.f) * TAU) * 0.28f + qSin(tt * 990.f * TAU) * 0.09f) * e;
        }
        return b.toWav();
    }
    case Sfx_Pickup: {
        Wav b(0.16f);
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            float f = (tt < 0.06f) ? 880.f : 1320.f;
            b.at(i) = qSin(tt * f * TAU) * 0.26f * e;
        }
        return b.toWav();
    }
    case Sfx_Gold: {
        Wav b(0.22f);
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            float f = 1600.f + qSin(tt * 40.f) * 500.f;
            b.at(i) = (qSin(tt * f * TAU) * 0.18f + qSin(tt * f * 1.5f * TAU) * 0.10f) * e;
        }
        return b.toWav();
    }
    case Sfx_LevelUp: {
        Wav b(0.85f);
        const float notes[5] = { 392.f, 523.f, 659.f, 784.f, 1046.f };
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            int k = qBound(0, int(tt / 0.15f), 4);
            float lt = tt - k * 0.15f;
            float eg = qMax(0.f, 1.f - lt * 3.2f);
            b.at(i) = (qSin(lt * notes[k] * TAU) * 0.24f + qSin(lt * notes[k] * 2.f * TAU) * 0.10f) * e * eg;
        }
        return b.toWav();
    }
    case Sfx_Death: {
        Wav b(0.9f);
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            float f = 320.f * qExp(-tt * 2.2f) + 45.f;
            b.at(i) = (qSin(tt * f * TAU) * 0.36f + nz(r) * 0.14f * qMax(0.f, 1.f - tt)) * e;
        }
        return b.toWav();
    }
    case Sfx_Boss: {
        Wav b(1.4f);
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            float f = 70.f + qSin(tt * 3.f) * 16.f;
            b.at(i) = (qSin(tt * f * TAU) * 0.42f + qSin(tt * f * 1.5f * TAU) * 0.22f
                       + nz(r) * 0.18f * qMax(0.f, 1.f - tt * 0.8f)) * e;
        }
        return b.toWav();
    }
    case Sfx_Portal: {
        Wav b(0.7f);
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            float f = 200.f + qSin(tt * 5.f) * 120.f + tt * 260.f;
            b.at(i) = (qSin(tt * f * TAU) * 0.22f + qSin(tt * f * 3.f * TAU) * 0.08f) * e;
        }
        return b.toWav();
    }
    case Sfx_Ui: {
        Wav b(0.10f);
        for (int i = 0; i < b.n(); ++i) { float e = b.env(i); b.at(i) = qSin(b.t(i) * 760.f * TAU) * 0.22f * e; }
        return b.toWav();
    }
    case Sfx_UiBad: {
        Wav b(0.18f);
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            b.at(i) = qSin(tt * (220.f - tt * 400.f) * TAU) * 0.24f * e;
        }
        return b.toWav();
    }
    case Sfx_SkillPoint: {
        Wav b(0.42f);
        const float notes[3] = { 660.f, 880.f, 1320.f };
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            int k = qBound(0, int(tt / 0.12f), 2);
            float lt = tt - k * 0.12f;
            float eg = qMax(0.f, 1.f - lt * 5.f);
            b.at(i) = (qSin(lt * notes[k] * TAU) * 0.22f + qSin(lt * notes[k] * 2.f * TAU) * 0.08f) * e * eg;
        }
        return b.toWav();
    }
    case Sfx_Ach: {
        Wav b(0.9f);
        const float notes[4] = { 523.f, 659.f, 784.f, 1046.f };
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            int k = qBound(0, int(tt / 0.16f), 3);
            float lt = tt - k * 0.16f;
            float eg = qMax(0.f, 1.f - lt * 2.6f);
            b.at(i) = (qSin(lt * notes[k] * TAU) * 0.24f + qSin(lt * notes[k] * 2.f * TAU) * 0.10f
                       + qSin(lt * notes[k] * 3.f * TAU) * 0.05f) * e * eg;
        }
        return b.toWav();
    }
    case Sfx_Roll: {
        Wav b(0.24f);
        for (int i = 0; i < b.n(); ++i) {
            float tt = b.t(i), e = b.env(i);
            b.at(i) = nz(r) * 0.28f * e * qMax(0.f, 1.f - tt * 4.5f);
        }
        return b.toWav();
    }
    default:
        break;
    }
    Q_UNUSED(w);
    return QByteArray();
}

// ============================================================
//  ВНЕШНИЕ АССЕТЫ И МУЗЫКА
// ============================================================
static QStringList audioExts()
{
    return QStringList() << QStringLiteral("ogg") << QStringLiteral("mp3") << QStringLiteral("wav")
           << QStringLiteral("flac") << QStringLiteral("m4a");
}

static bool isAudioFile(const QFileInfo &fi)
{
    return audioExts().contains(fi.suffix().toLower());
}

// Ищет папку со звуками: assets/sounds, assets/sound, sounds, sound, assets,
// а также просто корень - рядом с exe и в рабочей директории (и на уровень выше).
QString Audio::assetsDir()
{
    static QString cached;
    static bool done = false;
    if (done) return cached;
    done = true;
    QStringList roots;
    roots << QCoreApplication::applicationDirPath()
          << QDir::currentPath()
          << QCoreApplication::applicationDirPath() + QStringLiteral("/..")
          << QDir::currentPath() + QStringLiteral("/..");
    QStringList subs;
    subs << QStringLiteral("/assets/sounds") << QStringLiteral("/assets/sound")
         << QStringLiteral("/sounds") << QStringLiteral("/sound")
         << QStringLiteral("/assets") << QStringLiteral("");
    for (int r = 0; r < roots.size(); ++r) {
        for (int i = 0; i < subs.size(); ++i) {
            QDir d(roots.at(r) + subs.at(i));
            if (!d.exists()) continue;
            QFileInfoList list = d.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
            for (int k = 0; k < list.size(); ++k) {
                if (isAudioFile(list.at(k))) { cached = d.absolutePath(); return cached; }
            }
        }
    }
    cached = QCoreApplication::applicationDirPath() + QStringLiteral("/assets/sounds");
    return cached;
}

// Точное имя -> префикс -> ключевые слова (регистр не важен)
static QString findAssetFile(const QString &base)
{
    QDir dir(Audio::assetsDir());
    if (!dir.exists()) return QString();
    QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    QStringList keys;
    if (base == QLatin1String("music"))
        keys << QStringLiteral("музык") << QStringLiteral("theme")
             << QStringLiteral("ambient") << QStringLiteral("bgm") << QStringLiteral("track");
    else if (base == QLatin1String("music2"))
        keys << QStringLiteral("music2") << QStringLiteral("menu") << QStringLiteral("меню")
             << QStringLiteral("title") << QStringLiteral("mainmenu");
    else if (base == QLatin1String("kill"))
        keys << QStringLiteral("kill") << QStringLiteral("убий") << QStringLiteral("slay")
             << QStringLiteral("kill_") << QStringLiteral("enemy");
    else if (base == QLatin1String("death"))
        keys << QStringLiteral("death") << QStringLiteral("смерт") << QStringLiteral("die")
             << QStringLiteral("dead") << QStringLiteral("gameover");
    else if (base == QLatin1String("levelup"))
        keys << QStringLiteral("levelup") << QStringLiteral("level") << QStringLiteral("уровн")
             << QStringLiteral("lvl") << QStringLiteral("upgrade");
    else if (base == QLatin1String("hit"))
        keys << QStringLiteral("hit") << QStringLiteral("удар") << QStringLiteral("strike")
             << QStringLiteral("impact") << QStringLiteral("swing") << QStringLiteral("attack");
    else if (base == QLatin1String("crit"))
        keys << QStringLiteral("crit") << QStringLiteral("крит") << QStringLiteral("critical")
             << QStringLiteral("burst");
    else if (base == QLatin1String("skill"))
        keys << QStringLiteral("skill") << QStringLiteral("навык") << QStringLiteral("очк")
             << QStringLiteral("node") << QStringLiteral("point") << QStringLiteral("web");
    else if (base == QLatin1String("ach"))
        keys << QStringLiteral("ach") << QStringLiteral("trophy") << QStringLiteral("дост")
             << QStringLiteral("unlock");
    else if (base == QLatin1String("ui"))
        keys << QStringLiteral("click") << QStringLiteral("button") << QStringLiteral("клик")
             << QStringLiteral("кноп") << QStringLiteral("interface");
    else if (base == QLatin1String("music3"))
        keys << QStringLiteral("music3") << QStringLiteral("town") << QStringLiteral("город")
             << QStringLiteral("village") << QStringLiteral("hub") << QStringLiteral("npc");
    int bestScore = 0;
    QString best;
    for (int i = 0; i < list.size(); ++i) {
        const QFileInfo &fi = list.at(i);
        if (!isAudioFile(fi)) continue;
        QString bn = fi.completeBaseName().toLower();
        int score = 0;
        if (bn == base) score = 4;
        else if (bn.startsWith(base)) score = 3;
        else {
            for (int k = 0; k < keys.size(); ++k)
                if (bn.contains(keys.at(k))) { score = 2; break; }
        }
        if (score > bestScore) { bestScore = score; best = fi.absoluteFilePath(); }
    }
    return best;
}

namespace {
// Минимальный WAV-парсер: PCM 8/16 бит, моно/стерео, любая частота -> моно 22050 s16
bool loadWavToWav(const QString &path, Wav &out, int maxSecs = 20)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray all = f.readAll();
    f.close();
    if (all.size() < 44 || all.left(4) != "RIFF" || all.mid(8, 4) != "WAVE") return false;
    int pos = 12;
    int channels = 1, rate = SR, bits = 16;
    int dataPos = -1, dataSize = 0;
    while (pos + 8 <= all.size()) {
        QByteArray id = all.mid(pos, 4);
        quint32 sz = quint32(quint8(all[pos + 4])) | (quint32(quint8(all[pos + 5])) << 8)
                   | (quint32(quint8(all[pos + 6])) << 16) | (quint32(quint8(all[pos + 7])) << 24);
        int body = pos + 8;
        if (id == "fmt " && sz >= 16) {
            quint16 fmt = quint16(quint8(all[body])) | (quint16(quint8(all[body + 1])) << 8);
            channels = quint16(quint8(all[body + 2])) | (quint16(quint8(all[body + 3])) << 8);
            rate = qint32(quint8(all[body + 4])) | (qint32(quint8(all[body + 5])) << 8)
                 | (qint32(quint8(all[body + 6])) << 16) | (qint32(quint8(all[body + 7])) << 24);
            bits = quint16(quint8(all[body + 14])) | (quint16(quint8(all[body + 15])) << 8);
            if (fmt != 1) return false;   // только PCM
        } else if (id == "data") {
            dataPos = body;
            dataSize = int(sz);
        }
        pos = body + int(sz) + (sz & 1);
    }
    if (dataPos < 0 || channels < 1 || rate < 4000 || (bits != 8 && bits != 16)) return false;
    dataSize = qMin(dataSize, all.size() - dataPos);
    int bytesPerSample = bits / 8;
    int srcSamples = dataSize / (bytesPerSample * channels);
    if (srcSamples <= 0) return false;
    int dstSamples = int(qint64(srcSamples) * SR / rate);
    if (dstSamples <= 0 || dstSamples > SR * maxSecs) return false;
    out.s.resize(dstSamples);
    for (int i = 0; i < dstSamples; ++i) {
        qint64 si = qint64(i) * rate / SR;
        if (si >= srcSamples) si = srcSamples - 1;
        float acc = 0.f;
        for (int c = 0; c < channels; ++c) {
            int off = dataPos + int(si) * bytesPerSample * channels + c * bytesPerSample;
            if (bits == 16) {
                qint16 v = qint16(quint16(quint8(all[off])) | (quint16(quint8(all[off + 1])) << 8));
                acc += float(v) / 32768.f;
            } else {
                acc += (float(quint8(all[off])) - 128.f) / 128.f;
            }
        }
        out.s[i] = acc / float(channels);
    }
    return true;
}
} // namespace

void Audio::initAssets()
{
#ifdef HAVE_AUDIO
    struct Map { const char *base; int sfx; };
    static const Map maps[8] = { { "kill", Sfx_Kill }, { "death", Sfx_Death }, { "levelup", Sfx_LevelUp },
                                 { "hit", Sfx_Hit }, { "crit", Sfx_Crit }, { "skill", Sfx_SkillPoint },
                                 { "ach", Sfx_Ach }, { "ui", Sfx_Ui } };
    for (int i = 0; i < 8; ++i) {
        int sfx = maps[i].sfx;
        if (m_assetBuf.contains(sfx) || m_mediaAsset.contains(sfx)) continue;
        QString f = mappingFor(maps[i].base);
        if (f.isEmpty()) f = findAssetFile(QLatin1String(maps[i].base));
        if (f.isEmpty()) continue;
        m_broken.remove(sfx);
        QString suf = QFileInfo(f).suffix().toLower();
        m_assetName.insert(sfx, QFileInfo(f).fileName());
        if (suf == QLatin1String("wav")) {
            Wav w(0.01f);
            if (loadWavToWav(f, w) && w.n() > 16) {
                m_assetBuf.insert(sfx, w.toWav());
                continue;
            }
            m_assetName.insert(sfx, QFileInfo(f).fileName() + QStringLiteral(" (wav не читается)"));
        }
        m_mediaAsset.insert(sfx, f);
    }
    // отчёт собирается из текущего состояния, а не из одного прохода
    QStringList rep;
    for (int i = 0; i < 8; ++i) {
        int sfx = maps[i].sfx;
        if (!m_assetName.contains(sfx)) continue;
        rep << QStringLiteral("%1=%2%3").arg(QLatin1String(maps[i].base), m_assetName.value(sfx),
              m_assetBuf.contains(sfx) ? QStringLiteral("") : QStringLiteral(" (медиаплеер)"));
    }
    m_report = rep.isEmpty() ? QString() : (QStringLiteral("Свои звуки: ") + rep.join(QStringLiteral(", ")));
#endif
}

bool Audio::hasAsset(int sfx) const
{
#ifdef HAVE_AUDIO
    return m_assetBuf.contains(sfx) || (m_mediaAsset.contains(sfx) && !m_broken.contains(sfx));
#else
    Q_UNUSED(sfx);
    return false;
#endif
}

bool Audio::assetKnown(int sfx) const
{
#ifdef HAVE_AUDIO
    return m_assetBuf.contains(sfx) || m_mediaAsset.contains(sfx);
#else
    Q_UNUSED(sfx);
    return false;
#endif
}

QString Audio::assetPath(int sfx) const
{
#ifdef HAVE_AUDIO
    return m_mediaAsset.value(sfx, QString());
#else
    Q_UNUSED(sfx);
    return QString();
#endif
}

QString Audio::report() const
{
#ifdef HAVE_AUDIO
    return m_report;
#else
    return QString();
#endif
}

QString Audio::assetsReport() const
{
#ifdef HAVE_AUDIO
    QString base = m_report.isEmpty()
        ? QStringLiteral("Свои звуки не найдены (искал в %1) - работает встроенный синтез").arg(assetsDir())
        : m_report;
    {
        QStringList mus;
        QString gm = musicFile(false), mm = musicFile(true);
        if (!gm.isEmpty()) mus << QStringLiteral("music=%1").arg(QFileInfo(gm).fileName());
        if (!mm.isEmpty() && mm != gm) mus << QStringLiteral("меню=%1").arg(QFileInfo(mm).fileName());
        if (!mus.isEmpty())
            base = (base.startsWith(QStringLiteral("Свои звуки:")) ? base + QStringLiteral(", ") : QStringLiteral("Свои звуки: ")) + mus.join(QStringLiteral(", "));
    }
    QDir dir(assetsDir());
    QStringList files;
    if (dir.exists()) {
        QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
        for (int i = 0; i < list.size(); ++i)
            if (isAudioFile(list.at(i)) || list.at(i).fileName() == QLatin1String("mapping.txt"))
                files << list.at(i).fileName();
    }
    if (!files.isEmpty())
        base += QStringLiteral(" | в папке: %1").arg(files.join(QStringLiteral(", ")));
    if (!m_broken.isEmpty())
        base += QStringLiteral(" | часть файлов плеер отклонил - для них играет синтез");
    return base;
#else
    return QStringLiteral("Звук отключён при сборке (нет QtMultimedia)");
#endif
}

bool Audio::musicPlaying() const
{
#ifdef HAVE_AUDIO
    return m_musicOn;
#else
    return false;
#endif
}

bool Audio::musicIsWav() const
{
#ifdef HAVE_AUDIO
    return m_musicWav;
#else
    return false;
#endif
}

QString Audio::currentMusic() const
{
#ifdef HAVE_AUDIO
    return m_musicFile;
#else
    return QString();
#endif
}

QString Audio::musicFile(int which) const
{
    if (which == 1) {   // меню
        QString f = mappingFor(QLatin1String("music2"));
        if (!f.isEmpty()) return f;
        f = findAssetFile(QStringLiteral("music2"));
        if (!f.isEmpty()) return f;
        f = findAssetFile(QStringLiteral("menu"));
        if (!f.isEmpty()) return f;
        return findAssetFile(QStringLiteral("music"));
    }
    if (which == 2) {   // поселение с NPC
        QString f = mappingFor(QLatin1String("music3"));
        if (!f.isEmpty()) return f;
        f = findAssetFile(QStringLiteral("music3"));
        if (!f.isEmpty()) return f;
        f = findAssetFile(QStringLiteral("town"));
        if (!f.isEmpty()) return f;
        return findAssetFile(QStringLiteral("music"));
    }
    QString f = mappingFor(QLatin1String("music"));
    if (!f.isEmpty()) return f;
    f = findAssetFile(QStringLiteral("music"));
    if (!f.isEmpty()) return f;
    return findAssetFile(QStringLiteral("music2"));
}

void Audio::startMusic(int which)
{
#ifdef HAVE_AUDIO
    if (!m_enabled) { stopMusic(); return; }
    QString f = musicFile(which);
    if (f.isEmpty()) { stopMusic(); return; }
    bool sameAndPlaying = (m_musicFile == f) && m_musicOn &&
            ((m_musicWav && m_musicOut && m_musicOut->state() == QAudio::ActiveState) ||
             (!m_musicWav && m_music && m_music->state() == QMediaPlayer::PlayingState));
    if (sameAndPlaying) return;
    // мгновенная и строгая смена: глушим старый источник ДО старта нового
    if (m_musicOut) { m_musicOut->stop(); }
    if (m_music) { m_music->stop(); }
    m_musicOn = false;
    if (QFileInfo(f).suffix().toLower() == QLatin1String("wav")) {
        Wav w(0.05f);
        if (loadWavToWav(f, w, 900)) {          // музыка может быть длинной
            m_musicData = w.toWav();
            if (!m_musicOut) {
                m_musicOut = new QAudioOutput(m_format);
                m_musicIo = new QBuffer(&m_musicData);
                m_musicIo->open(QIODevice::ReadOnly);
                QObject::connect(m_musicOut, &QAudioOutput::stateChanged, [this](QAudio::State st) {
                    if (st != QAudio::StoppedState || !m_musicOn || !m_musicWav) return;
                    if (m_musicOut->error() != QAudio::NoError) { m_musicOn = false; return; }
                    // защита от мгновенных остановов (битый файл/нет устройства)
                    qint64 now = QDateTime::currentMSecsSinceEpoch();
                    if (now - m_musicStartedMs < 250) {
                        if (++m_quickStops >= 2) { m_musicOn = false; return; }
                    } else m_quickStops = 0;
                    m_musicIo->seek(0);
                    m_musicStartedMs = now;
                    m_musicOut->start(m_musicIo);
                });
            }
            m_musicOut->setVolume(m_volume);
            m_musicWav = true;
            m_musicFile = f;
            m_musicOn = true;
            m_quickStops = 0;
            m_musicIo->seek(0);
            m_musicStartedMs = QDateTime::currentMSecsSinceEpoch();
            m_musicOut->start(m_musicIo);
            return;
        }
    }
    m_musicWav = false;
    if (!m_music) {
        m_music = new QMediaPlayer();
        m_music->setVolume(int(m_volume * 80));
        QObject::connect(m_music, static_cast<void (QMediaPlayer::*)(QMediaPlayer::Error)>(&QMediaPlayer::error),
                         [this](QMediaPlayer::Error) {
            m_musicOn = false;
            m_report += QStringLiteral(" | музыку плеер отклонил (формат или кодек)");
        });
        QObject::connect(m_music, static_cast<void (QMediaPlayer::*)(QMediaPlayer::MediaStatus)>(&QMediaPlayer::mediaStatusChanged),
                         [this](QMediaPlayer::MediaStatus st) {
            if (st == QMediaPlayer::EndOfMedia && m_music && m_musicOn) m_music->play();   // зацикливаем
        });
    }
    m_music->stop();
    m_music->setMedia(QUrl::fromLocalFile(f));
    m_music->play();
    m_musicOn = true;
    m_musicFile = f;
#else
    Q_UNUSED(which);
#endif
}

void Audio::stopMusic()
{
#ifdef HAVE_AUDIO
    if (m_musicOut) m_musicOut->stop();
    if (m_music) m_music->stop();
    m_musicOn = false;
    m_musicFile.clear();
    m_musicWav = false;
#endif
}

} // namespace aa
