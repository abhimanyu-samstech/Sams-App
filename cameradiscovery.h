#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QJsonObject>
#include <QMap>
#include <QString>

struct CameraInfo {
    QString deviceId;
    QString model;
    QString ip;
    QString rtspPath;

    QString rtspUrl() const {
        return "rtsp://" + ip + rtspPath;
    }
};

class CameraDiscovery : public QObject
{
    Q_OBJECT

public:
    explicit CameraDiscovery(QObject *parent = nullptr);
    ~CameraDiscovery();

    void start();
    void stop();

    QMap<QString, CameraInfo> discoveredCameras() const { return m_cameras; }

signals:
    void cameraDiscovered(const QString &deviceId, const CameraInfo &info);
    void cameraUpdated(const QString &deviceId, const CameraInfo &info);

private slots:
    void onReadyRead();

private:
    QUdpSocket *m_socket = nullptr;
    QMap<QString, CameraInfo> m_cameras;
};