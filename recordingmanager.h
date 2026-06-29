#pragma once

#include <QObject>
#include <QMediaRecorder>
#include <QMediaCaptureSession>
#include <QVideoSink>
#include <QVideoFrame>
#include <QTimer>
#include <QString>

class VideoPlayer;

class RecordingManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY isRecordingChanged)
    Q_PROPERTY(QString recordingTime READ recordingTime NOTIFY recordingTimeChanged)

public:
    explicit RecordingManager(QObject *parent = nullptr);
    ~RecordingManager();

    void setVideoSink(QVideoSink *sink);

    bool isRecording() const { return m_isRecording; }
    QString recordingTime() const { return m_recordingTime; }

    Q_INVOKABLE void startRecording();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void takeSnapshot();
    Q_INVOKABLE QString recordingsPath() const;

signals:
    void isRecordingChanged();
    void recordingTimeChanged();
    void snapshotSaved(const QString &path);
    void recordingSaved(const QString &path);
    void error(const QString &message);

private slots:
    void onTimerTick();
    void onFrameChanged(const QVideoFrame &frame);

private:
    QVideoSink *m_sink = nullptr;
    bool m_isRecording = false;
    bool m_captureSnapshot = false;
    QString m_recordingTime = "00:00";
    QTimer *m_timer = nullptr;
    int m_elapsedSeconds = 0;
    QString m_currentRecordingPath;

    // For recording we write raw frames to MP4
    QList<QVideoFrame> m_recordedFrames;
};