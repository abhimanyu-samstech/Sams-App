#pragma once

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QQuickItem>

class VideoPlayer : public QObject
{
    Q_OBJECT

public:
    explicit VideoPlayer(QObject *parent = nullptr);
    ~VideoPlayer();

    Q_INVOKABLE void start();
    Q_INVOKABLE void startUrl(const QString &rtspUrl);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void setVideoOutput(QObject *videoOutput);

    QMediaPlayer* mediaPlayer() { return m_player; }

private:
    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audio = nullptr;
};