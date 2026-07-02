import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import QtWebView

ApplicationWindow {
    id: root
    visible: true
    width: 640
    height: 480
    title: "CamView"
    color: "#0a0a0a"

    property bool isWebRTCMode: false
    property bool showSettings: false

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        visible: !isWebRTCMode && !showSettings
    }

    // ── WebRTC View ───────────────────────────────────────────────
    Rectangle {
        id: webrtcScreen
        anchors.fill: parent
        color: "#0a0a0a"
        visible: isWebRTCMode && !showSettings
        z: 1

        WebView {
            id: webView
            anchors.fill: parent
            settings.javaScriptEnabled: true
            settings.localStorageEnabled: true
        }
    }

    // ── Loading Screen ────────────────────────────────────────────
    Rectangle {
        id: loadingScreen
        anchors.fill: parent
        color: "#0a0a0a"
        visible: !cameraManager.cameraAvailable && !showSettings
        z: 10

        Column {
            anchors.centerIn: parent
            spacing: 24

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 80
                height: 80
                radius: 40
                color: "transparent"
                border.color: "#00d4ff"
                border.width: 2

                Rectangle {
                    anchors.centerIn: parent
                    width: 20
                    height: 20
                    radius: 10
                    color: "#00d4ff"

                    SequentialAnimation on opacity {
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.2; duration: 800; easing.type: Easing.InOutSine }
                        NumberAnimation { to: 1.0; duration: 800; easing.type: Easing.InOutSine }
                    }

                    SequentialAnimation on scale {
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.6; duration: 800; easing.type: Easing.InOutSine }
                        NumberAnimation { to: 1.0; duration: 800; easing.type: Easing.InOutSine }
                    }
                }

                RotationAnimation on rotation {
                    loops: Animation.Infinite
                    from: 0; to: 360
                    duration: 2000
                }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Searching for camera..."
                color: "#ffffff"
                font.pixelSize: 18
                font.weight: Font.Light
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "CAM001"
                color: "#00d4ff"
                font.pixelSize: 13
                font.weight: Font.Medium
            }
        }
    }

    // ── Settings Screen ───────────────────────────────────────────
    Rectangle {
        id: settingsScreen
        anchors.fill: parent
        color: "#0a0a0a"
        visible: showSettings
        z: 20

        // Flickable wrapper is the key fix for Android keyboard
        Flickable {
            anchors.fill: parent
            contentHeight: settingsColumn.height + 100
            interactive: true

            Column {
                id: settingsColumn
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 80
                spacing: 24
                width: parent.width * 0.85

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Settings"
                    color: "#ffffff"
                    font.pixelSize: 22
                    font.weight: Font.Medium
                }

                Column {
                    width: parent.width
                    spacing: 8

                    Text {
                        text: "MediaMTX Server URL"
                        color: Qt.rgba(1, 1, 1, 0.6)
                        font.pixelSize: 12
                        font.weight: Font.Medium
                    }

                    Rectangle {
                        width: parent.width
                        height: 48
                        radius: 10
                        color: Qt.rgba(1, 1, 1, 0.08)
                        border.color: urlField.activeFocus ? "#00d4ff" : Qt.rgba(1, 1, 1, 0.2)
                        border.width: 1

                        Behavior on border.color { ColorAnimation { duration: 150 } }

                        TextEdit {
                            id: urlField
                            anchors.fill: parent
                            anchors.margins: 12
                            text: streamManager.serverUrl
                            color: "#ffffff"
                            font.pixelSize: 14
                            verticalAlignment: TextEdit.AlignVCenter
                            wrapMode: TextEdit.NoWrap
                            inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText | Qt.ImhUrlCharactersOnly
                            clip: true
                        }
                    }

                    Text {
                        text: "Example: http://192.168.0.101:8889"
                        color: Qt.rgba(1, 1, 1, 0.3)
                        font.pixelSize: 11
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 50
                    radius: 12
                    color: "#00d4ff"

                    Text {
                        anchors.centerIn: parent
                        text: "Save"
                        color: "#000000"
                        font.pixelSize: 16
                        font.weight: Font.Bold
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            streamManager.saveServerUrl(urlField.text)
                            showSettings = false
                            toast.show("✅ Server URL saved")
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 50
                    radius: 12
                    color: Qt.rgba(1, 1, 1, 0.08)
                    border.color: Qt.rgba(1, 1, 1, 0.2)
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "Cancel"
                        color: Qt.rgba(1, 1, 1, 0.7)
                        font.pixelSize: 16
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: showSettings = false
                    }
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Current: " + streamManager.serverUrl
                    color: Qt.rgba(1, 1, 1, 0.3)
                    font.pixelSize: 11
                    wrapMode: Text.WrapAnywhere
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }

    // ── Top Bar ───────────────────────────────────────────────────
    Rectangle {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 56
        color: Qt.rgba(0, 0, 0, 0.6)
        visible: cameraManager.cameraAvailable || showSettings
        z: 5
        layer.enabled: true

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            Row {
                spacing: 8
                Layout.alignment: Qt.AlignVCenter
                visible: !showSettings

                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: "#00ff88"
                    anchors.verticalCenter: parent.verticalCenter

                    SequentialAnimation on opacity {
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.3; duration: 1000 }
                        NumberAnimation { to: 1.0; duration: 1000 }
                    }
                }

                Text {
                    text: "CAM001"
                    color: "#ffffff"
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: "LIVE"
                    color: "#00ff88"
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    anchors.verticalCenter: parent.verticalCenter
                }

                Rectangle {
                    width: 60
                    height: 24
                    radius: 12
                    color: isWebRTCMode ? Qt.rgba(0, 212, 255, 0.2) : Qt.rgba(1, 1, 1, 0.1)
                    border.color: isWebRTCMode ? "#00d4ff" : Qt.rgba(1, 1, 1, 0.3)
                    border.width: 1
                    anchors.verticalCenter: parent.verticalCenter

                    Behavior on color { ColorAnimation { duration: 150 } }

                    Text {
                        anchors.centerIn: parent
                        text: isWebRTCMode ? "WEB" : "RTSP"
                        color: isWebRTCMode ? "#00d4ff" : Qt.rgba(1, 1, 1, 0.6)
                        font.pixelSize: 10
                        font.weight: Font.Bold
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            isWebRTCMode = !isWebRTCMode
                            if (isWebRTCMode) {
                                var url = streamManager.webrtcUrl("live/CAM001")
                                webView.loadHtml('<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><style>*{margin:0;padding:0;background:#000;}video{width:100vw;height:100vh;object-fit:cover;}</style></head><body><video id="v" autoplay playsinline muted></video><script>async function start(){const pc=new RTCPeerConnection();pc.ontrack=e=>{document.getElementById("v").srcObject=e.streams[0];};const offer=await pc.createOffer({offerToReceiveVideo:true,offerToReceiveAudio:false});await pc.setLocalDescription(offer);const r=await fetch("' + url + '/whep",{method:"POST",headers:{"Content-Type":"application/sdp"},body:offer.sdp});const ans=await r.text();await pc.setRemoteDescription({type:"answer",sdp:ans});}start();</script></body></html>', "http://localhost")
                                toast.show("🌐 Switched to WebRTC mode")
                            } else {
                                webView.url = ""
                                toast.show("📡 Switched to RTSP mode")
                            }
                        }
                    }
                }
            }

            Text {
                visible: showSettings
                text: "Settings"
                color: "#ffffff"
                font.pixelSize: 16
                font.weight: Font.Medium
                Layout.alignment: Qt.AlignVCenter
            }

            Item { Layout.fillWidth: true }

            Row {
                spacing: 6
                visible: recordingManager.isRecording && !showSettings
                Layout.alignment: Qt.AlignVCenter

                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: "#ff3b3b"
                    anchors.verticalCenter: parent.verticalCenter

                    SequentialAnimation on opacity {
                        loops: Animation.Infinite
                        running: recordingManager.isRecording
                        NumberAnimation { to: 0.2; duration: 500 }
                        NumberAnimation { to: 1.0; duration: 500 }
                    }
                }

                Text {
                    text: "REC " + recordingManager.recordingTime
                    color: "#ff3b3b"
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Text {
                text: Qt.formatTime(new Date(), "HH:mm")
                color: Qt.rgba(1, 1, 1, 0.6)
                font.pixelSize: 13
                Layout.alignment: Qt.AlignVCenter
                visible: !showSettings

                Timer {
                    interval: 1000
                    running: true
                    repeat: true
                    onTriggered: parent.text = Qt.formatTime(new Date(), "HH:mm")
                }
            }
        }
    }

    // ── Bottom Control Bar ────────────────────────────────────────
    Rectangle {
        id: bottomBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 100
        color: Qt.rgba(0, 0, 0, 0.7)
        visible: cameraManager.cameraAvailable && !showSettings
        z: 5

        RowLayout {
            anchors.centerIn: parent
            spacing: 40

            Column {
                spacing: 6
                Layout.alignment: Qt.AlignVCenter

                Rectangle {
                    width: 52
                    height: 52
                    radius: 12
                    color: snapshotMouse.pressed ? Qt.rgba(0, 212, 255, 0.3) : Qt.rgba(1, 1, 1, 0.1)
                    border.color: "#00d4ff"
                    border.width: 1.5
                    anchors.horizontalCenter: parent.horizontalCenter

                    Behavior on color { ColorAnimation { duration: 100 } }

                    Text {
                        anchors.centerIn: parent
                        text: "📷"
                        font.pixelSize: 22
                    }

                    MouseArea {
                        id: snapshotMouse
                        anchors.fill: parent
                        onClicked: {
                            recordingManager.takeSnapshot()
                            snapshotFlash.opacity = 0.6
                            snapshotFlash.opacity = 0
                        }
                    }
                }

                Text {
                    text: "SNAP"
                    color: Qt.rgba(1, 1, 1, 0.6)
                    font.pixelSize: 10
                    font.weight: Font.Medium
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            Column {
                spacing: 6
                Layout.alignment: Qt.AlignVCenter

                Rectangle {
                    width: 68
                    height: 68
                    radius: 34
                    color: "transparent"
                    border.color: recordingManager.isRecording ? "#ff3b3b" : "#ffffff"
                    border.width: 2.5
                    anchors.horizontalCenter: parent.horizontalCenter

                    Rectangle {
                        anchors.centerIn: parent
                        width: recordingManager.isRecording ? 24 : 50
                        height: recordingManager.isRecording ? 24 : 50
                        radius: recordingManager.isRecording ? 4 : 25
                        color: recordingManager.isRecording ? "#ff3b3b" : "#ffffff"

                        Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.InOutQuad } }
                        Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.InOutQuad } }
                        Behavior on radius { NumberAnimation { duration: 200; easing.type: Easing.InOutQuad } }
                        Behavior on color { ColorAnimation { duration: 200 } }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (recordingManager.isRecording)
                                recordingManager.stopRecording()
                            else
                                recordingManager.startRecording()
                        }
                    }
                }

                Text {
                    text: recordingManager.isRecording ? "STOP" : "REC"
                    color: recordingManager.isRecording ? "#ff3b3b" : Qt.rgba(1, 1, 1, 0.6)
                    font.pixelSize: 10
                    font.weight: Font.Medium
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            Column {
                spacing: 6
                Layout.alignment: Qt.AlignVCenter

                Rectangle {
                    width: 52
                    height: 52
                    radius: 12
                    color: infoMouse.pressed ? Qt.rgba(0, 212, 255, 0.3) : Qt.rgba(1, 1, 1, 0.1)
                    border.color: "#00d4ff"
                    border.width: 1.5
                    anchors.horizontalCenter: parent.horizontalCenter

                    Behavior on color { ColorAnimation { duration: 100 } }

                    Text {
                        anchors.centerIn: parent
                        text: "⚙️"
                        font.pixelSize: 22
                    }

                    MouseArea {
                        id: infoMouse
                        anchors.fill: parent
                        onClicked: showSettings = true
                    }
                }

                Text {
                    text: "SET"
                    color: Qt.rgba(1, 1, 1, 0.6)
                    font.pixelSize: 10
                    font.weight: Font.Medium
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
    }

    // ── Snapshot flash effect ─────────────────────────────────────
    Rectangle {
        id: snapshotFlash
        anchors.fill: parent
        color: "white"
        opacity: 0
        z: 20
        visible: !showSettings
        Behavior on opacity { NumberAnimation { duration: 150 } }
    }

    // ── Toast notifications ───────────────────────────────────────
    Rectangle {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: bottomBar.top
        anchors.bottomMargin: 12
        width: toastText.width + 32
        height: 36
        radius: 18
        color: Qt.rgba(0, 0, 0, 0.8)
        border.color: Qt.rgba(0, 212, 255, 0.4)
        border.width: 1
        opacity: 0
        z: 25

        Text {
            id: toastText
            anchors.centerIn: parent
            color: "#ffffff"
            font.pixelSize: 13
        }

        function show(message) {
            toastText.text = message
            toastAnim.restart()
        }

        SequentialAnimation {
            id: toastAnim
            NumberAnimation { target: toast; property: "opacity"; to: 1; duration: 200 }
            PauseAnimation { duration: 2000 }
            NumberAnimation { target: toast; property: "opacity"; to: 0; duration: 300 }
        }
    }

    // ── Connections ───────────────────────────────────────────────
    Connections {
        target: recordingManager

        function onSnapshotSaved(path) {
            toast.show("📷 Snapshot saved")
        }

        function onRecordingSaved(path) {
            toast.show("🎥 Recording saved")
        }

        function onError(message) {
            toast.show("⚠️ " + message)
        }
    }

    Component.onCompleted: {
        videoPlayer.setVideoOutput(videoOutput)
    }
}
