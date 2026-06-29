#pragma once

#include <QObject>
#include <QSettings>
#include <QString>

class StreamManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged)

public:
    explicit StreamManager(QObject *parent = nullptr);

    QString serverUrl() const { return m_serverUrl; }
    void setServerUrl(const QString &url);

    Q_INVOKABLE QString webrtcUrl(const QString &path) const;
    Q_INVOKABLE void saveServerUrl(const QString &url);
    Q_INVOKABLE QString loadServerUrl() const;

signals:
    void serverUrlChanged();

private:
    QString m_serverUrl;
    QSettings m_settings;
};