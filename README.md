# CamView — Camera Product Ecosystem

A production-grade camera streaming system built for the **Infinity6C SoC EVB** (ARMv7, BusyBox Linux) with an **Android app** (Qt 6.11.1). Supports local RTSP streaming and remote WebRTC streaming via a media server.

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                        EVB (Infinity6C SoC)                     │
│  Sensor → ISP → VENC → RTSP Server (:554)                       │
│                    ↓                                            │
│           camera_agent_arm (UDP broadcast :5005)                │
└──────────────────────┬──────────────────────────────────────────┘
                       │ RTSP
                       ↓
┌─────────────────────────────────────────────────────────────────┐
│                    Media Server (PC / Cloud VPS)                │
│  FFmpeg: pulls RTSP → pushes RTMP → mediamtx                    │
│  mediamtx: RTMP in → WebRTC out (:8889)                         │
└──────────────────────┬──────────────────────────────────────────┘
                       │ WebRTC / RTSP
                       ↓
┌─────────────────────────────────────────────────────────────────┐
│                    Android App (CamView)                        │
│  CameraDiscovery → CameraManager → VideoPlayer (RTSP)           │
│  StreamManager → WebView (WebRTC)                               │
└─────────────────────────────────────────────────────────────────┘
```

---

## Features

- **Auto-discovery** — EVB broadcasts its IP via UDP; app finds camera automatically, no hardcoded IPs
- **Local RTSP streaming** — low latency, direct H.264 stream on the same network
- **Remote WebRTC streaming** — stream from anywhere via mediamtx media server
- **Recording** — captures frames to app-private storage
- **Snapshots** — single frame capture saved as JPEG
- **Configurable server URL** — mediamtx server URL saved via QSettings, changeable from in-app settings
- **Modern dark UI** — animated loading screen, live indicator, REC timer, toast notifications

---

## Repository Structure

```
CamTest1/
├── android/
│   ├── AndroidManifest.xml
│   └── res/xml/
│       └── network_security_config.xml
├── cameradiscovery.h/.cpp      # UDP broadcast listener
├── cameramanager.h/.cpp        # Camera abstraction layer
├── recordingmanager.h/.cpp     # Frame capture / recording
├── streammanager.h/.cpp        # WebRTC server URL management
├── videoplayer.h/.cpp          # QMediaPlayer wrapper
├── main.cpp                    # App entry point
├── Main.qml                    # Full UI
└── CMakeLists.txt
```

---

## Prerequisites

### Development Machine (Windows)
- **Qt 6.11.1** with Android arm64-v8a kit
- **Android SDK** (API 36) + **NDK 27.x**
- **JDK 17** (Eclipse Adoptium recommended)
- **Qt Creator** (latest)
- **WSL 2** (Ubuntu) — for cross-compiling FFmpeg for EVB
- **ARM GNU Toolchain 13.3** (`arm-none-linux-gnueabihf`) — for cross-compiling `camera_agent`
  - Download from: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
  - Select: `AArch32 GNU/Linux target (arm-none-linux-gnueabihf)` → `.exe` installer

### Media Server (PC or Cloud VPS)
- **mediamtx v1.19.1** — https://github.com/bluenviron/mediamtx/releases
- **FFmpeg** (Windows build) — https://www.gyan.dev/ffmpeg/builds/

### EVB Hardware
- Infinity6C SoC EVB
- ARMv7, 128MB RAM, BusyBox v1.20.2, Linux 5.10.61
- Static IP: `192.168.0.150` (set in `/customer/demo.sh`)

---

## Setup Guide

There are two streaming modes. Follow **Path A** first (RTSP) — it is simpler and works on the local network. Once that is working, follow **Path B** (WebRTC) to enable remote streaming from anywhere.

---

## Path A — Local RTSP Streaming (same network)

> What this does: The EVB broadcasts its IP via UDP. The app discovers the camera automatically and plays the RTSP stream directly. No media server needed. Phone and EVB must be on the same WiFi/LAN.

```
EVB camera → RTSP (:554) → Android app (same network)
EVB camera_agent → UDP broadcast (:5005) → Android app discovers IP automatically
```

### A1 — Build `camera_agent_arm`

`camera_agent_arm` is the EVB daemon that broadcasts the camera's IP and RTSP path every 2 seconds so the app can find it automatically.

On Windows, open **Command Prompt** (not PowerShell):

```cmd
"C:\Program Files (x86)\Arm GNU Toolchain arm-none-linux-gnueabihf\13.3 rel1\bin\arm-none-linux-gnueabihf-g++.exe" ^
  -O2 -std=c++17 -static -o camera_agent_arm camera_agent.cpp
```

This produces a statically linked ARMv7 Linux binary (not Android — standard Linux).

### A2 — Transfer `camera_agent_arm` to EVB

Start a Python HTTP server on your PC in the folder containing the binary:

```cmd
cd C:\path\to\binaries
python -m http.server 8000
```

On the EVB via PuTTY (SSH to `192.168.0.150`):

```sh
cd /customer/streamapp
wget http://<your-pc-ip>:8000/camera_agent_arm
chmod +x camera_agent_arm
```

### A3 — Start the EVB camera stack and discovery agent

The vendor camera program must start first (it initialises the sensor and creates the RTSP server). Then start `camera_agent_arm` in the background:

```sh
# Terminal 1 on EVB — start vendor camera stack (creates rtsp://192.168.0.150/video0)
cd /customer/test
echo 0 | ./prog_vif_isp_scl_ut param_snr0.ini

# Terminal 2 on EVB — start discovery agent in background
/customer/streamapp/camera_agent_arm &
```

You should see repeated output like:
```
camera_agent: broadcast -> {"device_id":"CAM001","model":"C524","ip":"192.168.0.150","rtsp_path":"/video0"}
```

### A4 — Configure auto-start on boot (optional)

Edit `/customer/demo.sh` with `vi` and add these lines at the end:

```sh
# Start vendor camera stack
sleep 5
echo 0 | /customer/test/prog_vif_isp_scl_ut /customer/test/param_snr0.ini > /dev/null 2>&1 &

# Start discovery agent
sleep 3
/customer/streamapp/camera_agent_arm > /dev/null 2>&1 &
```

### A5 — Build and deploy the Android app

1. Open the project in **Qt Creator**
2. Select kit: **Qt 6.11.1 for Android arm64-v8a**
3. Press **Ctrl+B** to build
4. Connect your Android phone via USB with USB debugging enabled
5. Press **Run** (green play button)

### A6 — Test RTSP streaming

1. Make sure your phone is on the same WiFi as the EVB
2. Launch the app — it shows **"Searching for camera..."**
3. Within a few seconds it auto-discovers CAM001 and starts the RTSP stream
4. You should see the live camera feed on your phone

**RTSP is now working.** The toggle button in the top bar shows **RTSP** — this is local mode.

---

## Path B — Remote WebRTC Streaming (any network)

> What this does: A media server (mediamtx) runs on your PC or a cloud server. It receives the RTSP stream from the EVB via FFmpeg and serves it as WebRTC. The app connects to the media server using WebRTC — this works from anywhere, even on mobile data.

```
EVB camera → RTSP → PC FFmpeg → RTMP → mediamtx → WebRTC → Android app (any network)
```

> **Prerequisite:** Complete Path A first. The EVB vendor camera stack must be running.

### B1 — Install mediamtx on your PC

Download **mediamtx v1.19.1** from:
```
https://github.com/bluenviron/mediamtx/releases
```

Extract the ZIP — you get `mediamtx.exe` and `mediamtx.yml`.

### B2 — Open firewall ports on your PC

Run Command Prompt as **Administrator**:

```cmd
netsh advfirewall firewall add rule name="mediamtx-webrtc" dir=in action=allow protocol=TCP localport=8889
netsh advfirewall firewall add rule name="mediamtx-ice" dir=in action=allow protocol=UDP localport=8189
netsh advfirewall firewall add rule name="mediamtx-rtmp" dir=in action=allow protocol=TCP localport=1935
```

### B3 — Download Windows FFmpeg

Download from:
```
https://www.gyan.dev/ffmpeg/builds/
```
Download `ffmpeg-release-essentials.zip`, extract it, and note the path to `ffmpeg.exe` inside the `bin` folder.

### B4 — Start mediamtx

```cmd
cd C:\path\to\mediamtx_v1.19.1_windows_amd64
mediamtx.exe
```

You should see:
```
INF [RTMP] started with listener on :1935
INF [WebRTC] started with listeners on :8889, :8189
```

### B5 — Start FFmpeg relay (PC pulls RTSP from EVB, pushes to mediamtx)

> Note: FFmpeg runs on the PC, not the EVB. This is because the EVB's vendor RTSP server does not accept loopback connections from the EVB itself.

Open a new Command Prompt:

```cmd
ffmpeg.exe -i rtsp://192.168.0.150/video0 -c copy -f flv rtmp://127.0.0.1:1935/live/CAM001
```

You should see `frame=` lines counting up — video is flowing. In the mediamtx window you should see:
```
INF [path live/CAM001] stream is available and online, 1 track (H264)
```

Verify it works by opening this URL in your phone's browser (while on the same WiFi):
```
http://<your-pc-ip>:8889/live/CAM001
```

You should see the live stream in the browser.

### B6 — Configure the app with the server URL

1. Launch the app (RTSP stream should already be working from Path A)
2. Tap the **⚙️** button in the bottom bar
3. Enter the mediamtx server URL: `http://<your-pc-ip>:8889`
4. Tap **Save**
5. Tap the **RTSP** toggle button in the top bar to switch to **WEB** mode
6. The WebRTC stream should appear

### B7 — Test from a different network (optional)

To stream from outside your home/office network:

1. Find your router's public IP at `https://whatismyip.com`
2. Set up port forwarding on your router:
   - TCP port `8889` → `<your-pc-ip>:8889`
   - UDP port `8189` → `<your-pc-ip>:8189`
3. In the app Settings, change the server URL to: `http://<public-ip>:8889`
4. Switch your phone to mobile data and tap the WEB toggle — you should see the stream from anywhere

> For production, deploy mediamtx on a cloud VPS with a fixed public IP or domain name instead of relying on your home router's IP.

---

---

## Network Requirements

### Local Network (RTSP mode)
- Phone and EVB must be on the same network
- EVB broadcasts UDP on port `5005`
- RTSP stream on port `554`

### Remote Access (WebRTC mode)
- mediamtx server must have a **publicly reachable IP or domain**
- Open firewall ports: `8889` (TCP), `8189` (UDP)
- For home/office setup: configure port forwarding on your router
- For production: deploy mediamtx on a cloud VPS

### Windows Firewall (if running mediamtx on PC)

Run as Administrator:
```cmd
netsh advfirewall firewall add rule name="mediamtx-webrtc" dir=in action=allow protocol=TCP localport=8889
netsh advfirewall firewall add rule name="mediamtx-ice" dir=in action=allow protocol=UDP localport=8189
netsh advfirewall firewall add rule name="mediamtx-rtmp" dir=in action=allow protocol=TCP localport=1935
```

---

## App Components

| Class | File | Purpose |
|-------|------|---------|
| `CameraDiscovery` | `cameradiscovery.h/.cpp` | Listens on UDP port 5005, parses JSON broadcast packets |
| `CameraManager` | `cameramanager.h/.cpp` | Resolves device ID to RTSP URL, owns discovery |
| `VideoPlayer` | `videoplayer.h/.cpp` | Wraps QMediaPlayer + QAudioOutput for RTSP playback |
| `RecordingManager` | `recordingmanager.h/.cpp` | Frame capture via QVideoSink, snapshot and recording |
| `StreamManager` | `streammanager.h/.cpp` | Stores mediamtx server URL via QSettings, builds WebRTC endpoint |

---

## EVB Files

| Path | Purpose |
|------|---------|
| `/customer/demo.sh` | Boot script — sets static IP, loads modules, starts programs |
| `/customer/streamapp/camera_agent_arm` | Discovery daemon binary |
| `/customer/streamapp/ffmpeg` | Cross-compiled static FFmpeg binary |
| `/customer/test/prog_vif_isp_scl_ut` | Vendor camera stack (do not modify) |
| `/customer/test/param_snr0.ini` | Vendor camera configuration |

---

## Key Design Decisions

**Why Qt Multimedia instead of GStreamer?**
GStreamer's Android package ships only static `.a` libraries — runtime plugin registration fails. Qt Multimedia uses FFmpeg 7.1.3 internally and works out of the box.

**Why ARM GNU Toolchain instead of Android NDK?**
Android NDK produces Bionic libc binaries incompatible with the EVB's standard Linux. ARM GNU Toolchain (`arm-none-linux-gnueabihf`) produces correct Linux ELF binaries.

**Why UDP broadcast for discovery?**
Zero configuration, no server needed. `camera_agent` auto-detects its own IP via `getifaddrs()` and broadcasts the current IP every 2 seconds. The app always gets the correct IP even if DHCP reassigns it.

**Why directed broadcast (`192.168.0.255`) instead of `255.255.255.255`?**
Routers typically block `255.255.255.255` (limited broadcast) between Ethernet and WiFi interfaces. Directed broadcast is forwarded more reliably.

**Why cloud media server instead of P2P WebRTC on EVB?**
`libwebrtc` requires several GB to build and is incompatible with 128MB RAM ARMv7. The cloud media server pattern (Ring/Nest/Arlo) keeps the camera firmware simple — it just pushes H.264 via RTMP to a server which handles WebRTC, multi-viewer, and NAT traversal.

**Why `QSettings` for server URL?**
The mediamtx server IP changes during development and differs between LAN and production. Storing it in `QSettings` means the user sets it once in the app and it persists across restarts — no recompile needed when the server changes.

---

## Troubleshooting

**"Searching for camera..." never disappears**
- Check `camera_agent_arm` is running on EVB: `ps | grep camera`
- Check phone and EVB are on same network
- Check router forwards UDP broadcasts between Ethernet and WiFi

**RTSP stream shows but WebRTC toggle fails**
- Confirm mediamtx is running and FFmpeg is pushing
- Check the server URL in app Settings matches your PC/server IP
- Confirm Windows Firewall allows ports 8889 and 8189
- Verify `network_security_config.xml` is in `android/res/xml/` and `CMakeLists.txt` sets `QT_ANDROID_PACKAGE_SOURCE_DIR`

**EVB loopback RTSP timeout**
The vendor RTSP server does not accept connections from the EVB itself (loopback). FFmpeg must run on the PC pulling from the EVB's network IP (`192.168.0.150`), not from `127.0.0.1`.

**Binary runs on PC but fails on EVB with "ARM Bionic" error**
The binary was compiled with Android NDK instead of ARM GNU Toolchain. Recompile using `arm-none-linux-gnueabihf-g++` with `-static` flag.

---

## Version Roadmap

| Version | Status | Scope |
|---------|--------|-------|
| V1 | ✅ Complete | RTSP streaming, UDP discovery, CameraManager abstraction |
| V2 | ✅ Complete | Recording, snapshots, modern dark UI |
| V3 | ✅ Complete | WebRTC via mediamtx, StreamManager, configurable server URL |
| V4 | 🔲 Planned | Cloud backend, device registry, user accounts |
| V5 | 🔲 Planned | Remote access from anywhere, push notifications, cloud recording |

---

## License

Internal project — Innofusion. Not for public distribution.
