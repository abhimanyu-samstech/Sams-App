#pragma once

#include <QObject>
#include <QString>
#include "cameradiscovery.h"

class CameraManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool cameraAvailable READ cameraAvailable NOTIFY cameraAvailableChanged)

public:
    explicit CameraManager(QObject *parent = nullptr);
    ~CameraManager();

    void start();

    // The UI calls this — never touches raw IPs or RTSP URLs directly
    Q_INVOKABLE QString openCamera(const QString &deviceId);

    bool cameraAvailable() const { return m_cameraAvailable; }

signals:
    void cameraAvailableChanged();
    void cameraReady(const QString &deviceId, const QString &rtspUrl);

private slots:
    void onCameraDiscovered(const QString &deviceId, const CameraInfo &info);
    void onCameraUpdated(const QString &deviceId, const CameraInfo &info);

private:
    CameraDiscovery *m_discovery = nullptr;
    bool m_cameraAvailable = false;
};