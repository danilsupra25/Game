#ifndef AA_AUDIO_H
#define AA_AUDIO_H

// Процедурный звук: все эффекты синтезируются в рантайме (без аудиофайлов).
// Работает только если в сборке есть QtMultimedia (см. .pro: HAVE_AUDIO).

#include <QByteArray>
#include <QVector>
#include <QHash>
#include <QSet>
#include <QString>
#include <QElapsedTimer>

#ifdef HAVE_AUDIO
#include <QAudioOutput>
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QBuffer>
class QSoundEffect;
class QMediaPlayer;
#endif

namespace aa {

enum Sfx {
    Sfx_Swing = 0,
    Sfx_Hit,
    Sfx_Crit,
    Sfx_Hurt,
    Sfx_Cast,
    Sfx_Fire,
    Sfx_Ice,
    Sfx_Shock,
    Sfx_Poison,
    Sfx_Heal,
    Sfx_Shield,
    Sfx_Pickup,
    Sfx_Gold,
    Sfx_LevelUp,
    Sfx_Death,
    Sfx_Boss,
    Sfx_Portal,
    Sfx_Ui,
    Sfx_UiBad,
    Sfx_Roll,
    Sfx_Explosion,
    Sfx_Kill,
    Sfx_SkillPoint,
    Sfx_Ach,
    Sfx_Count
};

class Audio
{
public:
    static Audio &inst();

    void init();
    void shutdown();
    void setEnabled(bool on);
    bool enabled() const { return m_enabled; }
    void setVolume(float v);
    float volume() const { return m_volume; }

    void play(int sfx);
    void stopAll();

    // внешние ассеты: assets/sounds/{music,kill,death,levelup}.* рядом с exe или в CWD.
    // Если файла нет - работает встроенный синтез.
    void initAssets();
    bool hasAsset(int sfx) const;      // файл есть И плеер не отказал
    bool assetKnown(int sfx) const;    // файл обнаружен на диске
    QString assetPath(int sfx) const;
    QString report() const;
    static QString assetsDir();          // умный поиск папки со звуками
    static QString mappingFor(const QString &slot);  // mapping.txt
    QString assetsReport() const;        // человекочитаемо: что найдено
    // which: 0 - игровой трек (music), 1 - меню (music2), 2 - поселение с NPC (music3)
    void startMusic(int which = 0);
    void stopMusic();
    bool musicPlaying() const;
    bool musicIsWav() const;
    QString currentMusic() const;
    QString musicFile(int which) const;

private:
    Audio();
    ~Audio();
    QByteArray synth(int sfx);

    bool m_enabled = true;
    bool m_inited = false;
    float m_volume = 0.65f;

#ifdef HAVE_AUDIO
    struct Voice {
        QAudioOutput *out = 0;
        QBuffer *buf = 0;
        bool busy = false;
    };
    QVector<Voice> m_voices;
    QAudioFormat m_format;
    QMediaPlayer *m_music;
    bool m_musicOn;
    QString m_musicFile;
    // WAV-музыка через собственный конвейер (играет там, где медиаплеер не может)
    QAudioOutput *m_musicOut;
    QBuffer *m_musicIo;
    QByteArray m_musicData;
    bool m_musicWav;
    int m_quickStops;
    qint64 m_musicStartedMs;
    QHash<int, QByteArray> m_assetBuf;              // wav, разобранный в свой конвейер
    QHash<int, QString> m_assetName;                // имя файла для отчёта
    QHash<int, QString> m_mediaAsset;               // mp3/ogg/... -> путь, играет QMediaPlayer
    QSet<int> m_broken;                             // ассеты, которые плеер отказался играть
    QVector<QMediaPlayer *> m_mediaPool;
    QHash<QMediaPlayer *, int> m_mediaOwner;
    int m_mediaNext;
    QHash<int, float> m_lastPlay;   // анти-«каша»: минимальный интервал между одинаковыми звуками
    QElapsedTimer m_clock;
    QString m_report;
#endif
};

} // namespace aa

#endif // AA_AUDIO_H
