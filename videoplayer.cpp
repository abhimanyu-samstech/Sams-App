#include "videoplayer.h"
#include <QDebug>
#include <QVideoSink>

VideoPlayer::VideoPlayer(QObject *parent)
    : QObject(parent)
{
    m_player = new QMediaPlayer(this);
    m_audio = new QAudioOutput(this);
    m_player->setAudioOutput(m_audio);

    connect(m_player, &QMediaPlayer::errorOccurred,
            this, [](QMediaPlayer::Error error, const QString &errorString) {
                qWarning() << "Media error:" << error << errorString;
            });

    connect(m_player, &QMediaPlayer::playbackStateChanged,
            this, [](QMediaPlayer::PlaybackState state) {
                qDebug() << "Playback state:" << state;
            });
}

VideoPlayer::~VideoPlayer()
{
    stop();
}

void VideoPlayer::setVideoOutput(QObject *videoOutput)
{
    QVideoSink *sink = videoOutput->property("videoSink").value<QVideoSink*>();
    if (sink) {
        m_player->setVideoSink(sink);
        qDebug() << "Video sink connected";
    } else {
        qWarning() << "Failed to get video sink from QML VideoOutput";
    }
}

void VideoPlayer::start()
{
    // Legacy method - starts with hardcoded URL for testing
    startUrl("rtsp://192.168.0.150/video0");
}

void VideoPlayer::startUrl(const QString &rtspUrl)
{
    qDebug() << "VideoPlayer: starting" << rtspUrl;
    m_player->setSource(QUrl(rtspUrl));
    m_player->play();
}

void VideoPlayer::stop()
{
    if (m_player) m_player->stop();
}