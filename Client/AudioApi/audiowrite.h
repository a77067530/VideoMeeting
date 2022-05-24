#ifndef AUDIOWRITE_H
#define AUDIOWRITE_H

#include <QObject>
#include"world.h"
class AudioWrite : public QObject
{
    Q_OBJECT
public:
    explicit AudioWrite(QObject *parent = nullptr);
    ~AudioWrite();
signals:

public slots:
    void slot_playAudio(QByteArray ba);
private:
    QAudioOutput * m_audio_out;
    QIODevice * m_buffer_out;
    QAudioFormat format;
    SpeexBits bits_dec;
    void *Dec_State;
};

#endif // AUDIOWRITE_H
