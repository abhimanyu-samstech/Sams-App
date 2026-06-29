#include "recordingmanager.h"
#include <QVideoFrame>
#include <QImage>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QDebug>
#include <QProcess>

RecordingManager::RecordingManager(QObject *parent)
    : QObject(parent)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &RecordingManager::onTimerTick);
}

RecordingManager::~RecordingManager()
{
    if (m_isRecording) stopRecording();
}

void RecordingManager::setVideoSink(QVideoSink *sink)
{
    if (m_sink == sink) return;

    if (m_sink) {
        disconnect(m_sink, &QVideoSink::videoFrameChanged,
                   this, &RecordingManager::onFrameChanged);
    }

    m_sink = sink;

    if (m_sink) {
        connect(m_sink, &QVideoSink::videoFrameChanged,
                this, &RecordingManager::onFrameChanged);
    }
}

QString RecordingManager::recordingsPath() const
{
    QString path = QStandardPaths::writableLocation(
                       QStandardPaths::AppDataLocation) + "/recordings";
    QDir().mkpath(path);
    return path;
}

void RecordingManager::startRecording()
{
    if (m_isRecording) return;

    m_currentRecordingPath = recordingsPath() + "/rec_" +
                             QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QDir().mkpath(m_currentRecordingPath);

    m_recordedFrames.clear();
    m_elapsedSeconds = 0;
    m_recordingTime = "00:00";
    m_isRecording = true;

    m_timer->start();

    emit isRecordingChanged();
    emit recordingTimeChanged();

    qDebug() << "RecordingManager: started recording to" << m_currentRecordingPath;
}

void RecordingManager::stopRecording()
{
    if (!m_isRecording) return;

    m_isRecording = false;
    m_timer->stop();

    emit isRecordingChanged();

    // Save all captured frames as JPEGs in the recording folder
    qDebug() << "RecordingManager: saving" << m_recordedFrames.size() << "frames";

    int frameIndex = 0;
    for (const QVideoFrame &frame : m_recordedFrames) {
        QImage img = frame.toImage();
        if (!img.isNull()) {
            QString framePath = m_currentRecordingPath +
                                QString("/frame_%1.jpg").arg(frameIndex++, 5, 10, QChar('0'));
            img.save(framePath, "JPEG", 85);
        }
    }

    m_recordedFrames.clear();
    emit recordingSaved(m_currentRecordingPath);
    qDebug() << "RecordingManager: recording saved to" << m_currentRecordingPath;
}

void RecordingManager::takeSnapshot()
{
    m_captureSnapshot = true;
    qDebug() << "RecordingManager: snapshot requested";
}

void RecordingManager::onTimerTick()
{
    m_elapsedSeconds++;
    int minutes = m_elapsedSeconds / 60;
    int seconds = m_elapsedSeconds % 60;
    m_recordingTime = QString("%1:%2")
                          .arg(minutes, 2, 10, QChar('0'))
                          .arg(seconds, 2, 10, QChar('0'));
    emit recordingTimeChanged();
}

void RecordingManager::onFrameChanged(const QVideoFrame &frame)
{
    if (m_captureSnapshot) {
        m_captureSnapshot = false;
        QImage img = frame.toImage();
        if (!img.isNull()) {
            QString path = recordingsPath() + "/snap_" +
                           QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".jpg";
            if (img.save(path, "JPEG", 95)) {
                qDebug() << "RecordingManager: snapshot saved to" << path;
                emit snapshotSaved(path);
            } else {
                emit error("Failed to save snapshot");
            }
        }
    }

    if (m_isRecording) {
        // Capture every 3rd frame to keep memory reasonable (~10fps from 30fps stream)
        static int frameCount = 0;
        if (++frameCount % 3 == 0) {
            m_recordedFrames.append(frame);
        }
    }
}