#include "cameradiscovery.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkDatagram>
#include <QDebug>

CameraDiscovery::CameraDiscovery(QObject *parent)
    : QObject(parent)
{
    m_socket = new QUdpSocket(this);
    connect(m_socket, &QUdpSocket::readyRead,
            this, &CameraDiscovery::onReadyRead);
}

CameraDiscovery::~CameraDiscovery()
{
    stop();
}

void CameraDiscovery::start()
{
    if (m_socket->state() == QAbstractSocket::BoundState)
        return;

    bool bound = m_socket->bind(QHostAddress::AnyIPv4, 5005,
                                QUdpSocket::ShareAddress |
                                    QUdpSocket::ReuseAddressHint);
    if (bound) {
        qDebug() << "CameraDiscovery: listening on UDP port 5005";
    } else {
        qWarning() << "CameraDiscovery: failed to bind port 5005:"
                   << m_socket->errorString();
    }
}

void CameraDiscovery::stop()
{
    if (m_socket && m_socket->state() == QAbstractSocket::BoundState)
        m_socket->close();
}

void CameraDiscovery::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        QByteArray data = datagram.data();

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError) {
            qWarning() << "CameraDiscovery: invalid JSON:" << err.errorString();
            continue;
        }

        QJsonObject obj = doc.object();
        QString deviceId = obj.value("device_id").toString();
        if (deviceId.isEmpty()) continue;

        CameraInfo info;
        info.deviceId  = deviceId;
        info.model     = obj.value("model").toString();
        info.ip        = obj.value("ip").toString();
        info.rtspPath  = obj.value("rtsp_path").toString();

        bool isNew = !m_cameras.contains(deviceId);
        m_cameras[deviceId] = info;

        qDebug() << "CameraDiscovery: found" << deviceId
                 << "at" << info.rtspUrl();

        if (isNew)
            emit cameraDiscovered(deviceId, info);
        else
            emit cameraUpdated(deviceId, info);
    }
}