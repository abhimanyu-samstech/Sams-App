#include "streammanager.h"
#include <QDebug>

StreamManager::StreamManager(QObject *parent)
    : QObject(parent)
    , m_settings("Innofusion", "CamTest1")
{
    // Load saved server URL on startup
    m_serverUrl = m_settings.value("serverUrl", "http://192.168.0.101:8889").toString();
    qDebug() << "StreamManager: loaded server URL:" << m_serverUrl;
}

void StreamManager::setServerUrl(const QString &url)
{
    if (m_serverUrl == url) return;
    m_serverUrl = url;
    emit serverUrlChanged();
}

void StreamManager::saveServerUrl(const QString &url)
{
    m_serverUrl = url;
    m_settings.setValue("serverUrl", url);
    m_settings.sync();
    emit serverUrlChanged();
    qDebug() << "StreamManager: saved server URL:" << url;
}

QString StreamManager::loadServerUrl() const
{
    return m_settings.value("serverUrl", "http://192.168.0.101:8889").toString();
}

QString StreamManager::webrtcUrl(const QString &path) const
{
    return m_serverUrl + "/" + path;
}