#include "cameramanager.h"
#include <QDebug>
#include <QTimer>

CameraManager::CameraManager(QObject *parent)
    : QObject(parent)
{
    m_discovery = new CameraDiscovery(this);

    connect(m_discovery, &CameraDiscovery::cameraDiscovered,
            this, &CameraManager::onCameraDiscovered);

    connect(m_discovery, &CameraDiscovery::cameraUpdated,
            this, &CameraManager::onCameraUpdated);
}

CameraManager::~CameraManager()
{
    m_discovery->stop();
}

void CameraManager::start()
{
    m_discovery->start();
    qDebug() << "CameraManager: started, waiting for camera broadcasts...";

    // TEMP: force UI visible for testing without EVB
    m_cameraAvailable = true;
    emit cameraAvailableChanged();
}

QString CameraManager::openCamera(const QString &deviceId)
{
    auto cameras = m_discovery->discoveredCameras();

    if (!cameras.contains(deviceId)) {
        qWarning() << "CameraManager: camera not found:" << deviceId;
        return QString();
    }

    QString url = cameras[deviceId].rtspUrl();
    qDebug() << "CameraManager: opening" << deviceId << "at" << url;
    return url;
}

void CameraManager::onCameraDiscovered(const QString &deviceId,
                                       const CameraInfo &info)
{
    qDebug() << "CameraManager: camera discovered:" << deviceId
             << "model:" << info.model
             << "url:" << info.rtspUrl();

    m_cameraAvailable = true;
    emit cameraAvailableChanged();
    emit cameraReady(deviceId, info.rtspUrl());
}

void CameraManager::onCameraUpdated(const QString &deviceId,
                                    const CameraInfo &info)
{
    qDebug() << "CameraManager: camera updated:" << deviceId
             << "url:" << info.rtspUrl();

    emit cameraReady(deviceId, info.rtspUrl());
}