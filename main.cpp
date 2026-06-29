#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include <QtWebView/QtWebView>
#include "videoplayer.h"
#include "cameramanager.h"
#include "recordingmanager.h"

int main(int argc, char *argv[])
{
    QtWebView::initialize();

    qputenv("QT_FFMPEG_DECODING_HW_DEVICE_TYPES", QByteArray(""));
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

    QGuiApplication app(argc, argv);
    app.setApplicationName("CamTest1");
    app.setOrganizationName("Innofusion");

    VideoPlayer player;
    CameraManager cameraManager;
    RecordingManager recordingManager;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("videoPlayer", &player);
    engine.rootContext()->setContextProperty("cameraManager", &cameraManager);
    engine.rootContext()->setContextProperty("recordingManager", &recordingManager);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() {
            qCritical() << "QML object creation failed!";
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.loadFromModule("CamTest1", "Main");

    qDebug() << "QML loaded, root objects:" << engine.rootObjects().size();

    cameraManager.start();

    // When camera is discovered, start streaming automatically
    QObject::connect(&cameraManager, &CameraManager::cameraReady,
                     &player, [&player](const QString &deviceId,
                               const QString &rtspUrl) {
                         Q_UNUSED(deviceId)
                         static bool started = false;
                         if (!started) {
                             started = true;
                             qDebug() << "Auto-starting stream for" << rtspUrl;
                             player.startUrl(rtspUrl);
                         }
                     });

    // Connect recording manager to video sink once player is ready
    QObject::connect(player.mediaPlayer(), &QMediaPlayer::playbackStateChanged,
                     &recordingManager, [&player, &recordingManager](QMediaPlayer::PlaybackState state) {
                         if (state == QMediaPlayer::PlayingState) {
                             recordingManager.setVideoSink(player.mediaPlayer()->videoSink());
                             qDebug() << "RecordingManager: connected to video sink";
                         }
                     });

    return QGuiApplication::exec();
}