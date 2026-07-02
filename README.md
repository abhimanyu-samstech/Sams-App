# CamView — Camera Product Ecosystem

A production-grade camera streaming system built for the Infinity6C SoC EVB (ARMv7, BusyBox Linux) with an Android app (Qt 6.11.1). Supports local RTSP streaming and remote WebRTC streaming via a media server.

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
- **NAT traversal** — STUN/TURN support via Google STUN + self-hosted coturn on Raspberry Pi
- **Tailscale overlay network** — secure cross-network access without port forwarding (works behind CGNAT)
- **Recording** — captures frames to app-private storage
- **Snapshots** — single frame capture saved as JPEG
- **Configurable server URL** — mediamtx server URL saved via QSettings, changeable from in-app settings at runtime
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
- Qt 6.11.1 with Android arm64-v8a kit
- Android SDK (API 36) + NDK 27.x
- JDK 17 (Eclipse Adoptium recommended)
- Qt Creator (latest)
- WSL 2 (Ubuntu) — for cross-compiling FFmpeg for EVB
- ARM GNU Toolchain 13.3 (arm-none-linux-gnueabihf) — for cross-compiling camera_agent
  - Download: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
  - Select: AArch32 GNU/Linux target (arm-none-linux-gnueabihf) → .exe installer

### EVB Hardware
- Infinity6C SoC EVB
- ARMv7, 128MB RAM, BusyBox v1.20.2, Linux 5.10.61
- Static IP: 192.168.0.150 (set in /customer/demo.sh)

### Remote Streaming Infrastructure (V3)
- Raspberry Pi (any model with Linux) — runs coturn TURN relay server
- Tailscale account (free) — https://tailscale.com
- All three devices (PC, Pi, Android phone) enrolled in the same Tailscale network

---

## Setup Guide

There are two streaming modes. Follow Path A first (RTSP) — it is simpler and works on the local network. Once that is working, follow Path B (WebRTC) to enable remote streaming from anywhere.

---

## Path A — Local RTSP Streaming (same network)

**What this does:** The EVB broadcasts its IP via UDP. The app discovers the camera automatically and plays the RTSP stream directly. No media server needed. Phone and EVB must be on the same WiFi/LAN.

```
EVB camera → RTSP (:554) → Android app (same network)
EVB camera_agent → UDP broadcast (:5005) → Android app discovers IP automatically
```

### A1 — Build camera_agent_arm

`camera_agent_arm` is the EVB daemon that broadcasts the camera's IP and RTSP path every 2 seconds so the app can find it automatically.

On Windows, open Command Prompt (not PowerShell):

```cmd
"C:\Program Files (x86)\Arm GNU Toolchain arm-none-linux-gnueabihf\13.3 rel1\bin\arm-none-linux-gnueabihf-g++.exe" ^
  -O2 -std=c++17 -static -o camera_agent_arm camera_agent.cpp
```

This produces a statically linked ARMv7 Linux binary (not Android — standard Linux).

### A2 — Transfer camera_agent_arm to EVB

Start a Python HTTP server on your PC in the folder containing the binary:

```cmd
cd C:\path\to\binaries
python -m http.server 8000
```

On the EVB via PuTTY (SSH to 192.168.0.150):

```bash
cd /customer/streamapp
wget http://<your-pc-ip>:8000/camera_agent_arm
chmod +x camera_agent_arm
```

### A3 — Start the EVB camera stack and discovery agent

```bash
# Start vendor camera stack (creates rtsp://192.168.0.150/video0)
cd /customer/test
echo 0 | ./prog_vif_isp_scl_ut param_snr0.ini

# Start discovery agent in background
/customer/streamapp/camera_agent_arm &
```

You should see repeated output like:
```
camera_agent: broadcast -> {"device_id":"CAM001","model":"C524","ip":"192.168.0.150","rtsp_path":"/video0"}
```

### A4 — Configure auto-start on boot (optional)

Edit `/customer/demo.sh` with `vi` and add these lines at the end:

```bash
# Start vendor camera stack
sleep 5
echo 0 | /customer/test/prog_vif_isp_scl_ut /customer/test/param_snr0.ini > /dev/null 2>&1 &

# Start discovery agent
sleep 3
/customer/streamapp/camera_agent_arm > /dev/null 2>&1 &
```

### A5 — Build and deploy the Android app

1. Open the project in Qt Creator
2. Select kit: Qt 6.11.1 for Android arm64-v8a
3. Press `Ctrl+B` to build
4. Connect your Android phone via USB with USB debugging enabled
5. Press Run (green play button)

### A6 — Test RTSP streaming

1. Make sure your phone is on the same WiFi as the EVB
2. Launch the app — it shows "Searching for camera..."
3. Within a few seconds it auto-discovers CAM001 and starts the RTSP stream
4. You should see the live camera feed on your phone

RTSP is now working. The toggle button in the top bar shows **RTSP** — this is local mode.

---

## Path B — Remote WebRTC Streaming (any network)

**What this does:** A media server runs on your PC. It receives the RTSP stream from the EVB and converts it to WebRTC. The app connects to the media server using WebRTC — this works from anywhere, even on mobile data.

```
EVB camera → RTSP → PC FFmpeg → RTMP → mediamtx → WebRTC → Android app (any network)
```

**Prerequisite:** Complete Path A first. The EVB vendor camera stack must be running.

### B1 — Install mediamtx

**What is mediamtx?** mediamtx (formerly rtsp-simple-server) is a lightweight, open-source media server. It accepts live video streams in one format and re-serves them in another. In this project it receives RTMP from FFmpeg and converts it to WebRTC for the app.

**Why do we need it?** RTSP works great on a local network but is blocked by most routers when coming from the internet. WebRTC (the technology used by Zoom, Google Meet) is specifically designed to work through firewalls and routers — making remote viewing possible. mediamtx handles this conversion so the EVB stays simple.

```
FFmpeg RTMP → mediamtx → WebRTC (Android app, works from anywhere)
                       → HLS  (browser fallback)
                       → RTSP (VLC, other players)
```

Download mediamtx v1.19.1 from:
```
https://github.com/bluenviron/mediamtx/releases
```
Extract the ZIP — you get `mediamtx.exe` and `mediamtx.yml`. No installation needed.

### B2 — Open firewall ports on your PC

Run Command Prompt as Administrator:

```cmd
netsh advfirewall firewall add rule name="mediamtx-webrtc" dir=in action=allow protocol=TCP localport=8889
netsh advfirewall firewall add rule name="mediamtx-ice" dir=in action=allow protocol=UDP localport=8189
netsh advfirewall firewall add rule name="mediamtx-rtmp" dir=in action=allow protocol=TCP localport=1935
```

### B3 — Install FFmpeg on your PC

**What is FFmpeg?** FFmpeg is a free, open-source command-line tool for reading, converting, and streaming audio/video. In this project it acts as a bridge — it pulls the H.264 video from the EVB via RTSP and immediately pushes it to mediamtx via RTMP without re-encoding.

```
EVB RTSP (:554) → FFmpeg → mediamtx RTMP (:1935)
```

The `-c copy` flag means "copy the video as-is, don't re-encode" — this is fast and uses minimal CPU.

Download Windows FFmpeg from:
```
https://www.gyan.dev/ffmpeg/builds/
```
Download `ffmpeg-release-essentials.zip`, extract it, and note the path to `ffmpeg.exe` in the `bin` folder.

### B4 — Set up Tailscale (required for cross-network access)

**What is Tailscale?** Tailscale creates a private virtual overlay network (using WireGuard) between your devices. Even when each device is on a different network — home WiFi, mobile data, office — they can all reach each other via stable `100.x.x.x` addresses. This solves CGNAT (common with Indian ISPs) where your router doesn't have a real public IP, making port forwarding ineffective.

**Install Tailscale on all three devices:**

1. **PC (Windows):** Download from https://tailscale.com/download/windows — sign in with Google or GitHub
2. **Raspberry Pi:** SSH into the Pi and run:
   ```bash
   curl -fsSL https://tailscale.com/install.sh | sh
   sudo tailscale up
   ```
   Open the URL it prints, sign in with the same account
3. **Android phone:** Install "Tailscale" from Play Store — sign in with the same account

Once all three are connected, you will see them all in the Tailscale dashboard at https://login.tailscale.com/admin/machines with green "Connected" status. Note each device's `100.x.x.x` Tailscale IP — you'll need them below.

Example addresses from this project:
- PC (abhimanyu): `100.72.216.66`
- Pi (abhiraspberry): `100.104.224.122`
- Android phone (poco-m2-pro): `100.100.241.44`

### B5 — Set up coturn on Raspberry Pi

**What is coturn?** coturn is an open-source STUN/TURN server. WebRTC uses ICE (Interactive Connectivity Establishment) to find a path between two devices. STUN tells a device its public-facing address. TURN relays actual media traffic when a direct connection is impossible — which is the case when your PC is behind CGNAT. Since the Pi is reachable via Tailscale, it acts as the TURN relay.

SSH into the Pi and run:

```bash
sudo apt update
sudo apt install coturn -y
```

Enable the service:
```bash
sudo nano /etc/default/coturn
```
Uncomment (remove the `#`):
```
TURNSERVER_ENABLED=1
```

Configure coturn — replace the contents of `/etc/turnserver.conf` with:
```
listening-ip=100.104.224.122
listening-port=3478
fingerprint
lt-cred-mech
user=camuser:campass123
realm=abhiraspberry
total-quota=100
no-multicast-peers
min-port=49152
max-port=49352
```

> Replace `100.104.224.122` with your Pi's actual Tailscale IP if different.

Start coturn:
```bash
sudo systemctl enable coturn
sudo systemctl start coturn
sudo systemctl status coturn
```
You should see `active (running)`.

### B6 — Configure mediamtx ICE servers

Open `mediamtx.yml` and find the `webrtcICEServers2` section. Update it to:

```yaml
webrtcICEServers2:
  - url: stun:stun.l.google.com:19302
  - url: turn:100.104.224.122:3478
    username: camuser
    password: campass123
```

> Use `password:` not `credential:` — mediamtx v1.19.1 uses `password` as the key name.
> Replace `100.104.224.122` with your Pi's actual Tailscale IP if different.

### B7 — Start mediamtx

```cmd
cd C:\path\to\mediamtx_v1.19.1_windows_amd64
mediamtx.exe
```

You should see:
```
INF [RTMP] started with listener on :1935
INF [WebRTC] started with listeners on :8889, :8189
```

### B8 — Start FFmpeg relay

Open a new Command Prompt:

```cmd
ffmpeg.exe -i rtsp://192.168.0.150/video0 -c copy -f flv rtmp://127.0.0.1:1935/live/CAM001
```

You should see `frame=` lines counting up. In the mediamtx window:
```
INF [path live/CAM001] stream is available and online, 1 track (H264)
```

### B9 — Configure the app with the server URL

1. Launch the app
2. Tap the ⚙️ button in the bottom bar to open Settings
3. Tap the URL field — the Android keyboard will appear
4. Enter the mediamtx server URL using the PC's **Tailscale IP**: `http://100.72.216.66:8889`
5. Tap **Save** — the URL is stored permanently on the device via QSettings
6. Tap the **RTSP/WEB** toggle button in the top bar to switch to WEB mode
7. The WebRTC stream should appear

### B10 — Test from a different network

1. Turn off WiFi on your phone — switch to mobile data only
2. Make sure Tailscale is still active on the phone (VPN toggle on)
3. Open the app and tap the WEB toggle
4. The stream should appear from mobile data — confirming cross-network streaming works

---

## Upgrading to a Cloud VPS (Future)

When a cloud VPS becomes available, only **one config file needs to change** — `mediamtx.yml` on the PC:

```yaml
# Before (Tailscale/Pi TURN):
webrtcICEServers2:
  - url: stun:stun.l.google.com:19302
  - url: turn:100.104.224.122:3478
    username: camuser
    password: campass123

# After (Cloud VPS):
webrtcICEServers2:
  - url: stun:stun.l.google.com:19302
  - url: turn:your-cloud-vps-ip:3478
    username: camuser
    password: campass123
```

No code changes, no recompile, no app update required. The rest of the pipeline is unchanged.

---

## Network Requirements

### Local Network (RTSP mode)
- Phone and EVB must be on the same network
- EVB broadcasts UDP on port 5005
- RTSP stream on port 554

### Remote Access (WebRTC mode — Tailscale)
- All devices enrolled in the same Tailscale network
- coturn running on Raspberry Pi (reachable via Tailscale)
- mediamtx running on PC (reachable via Tailscale)
- No port forwarding or public IP required

### Remote Access (WebRTC mode — Cloud VPS, future)
- mediamtx server must have a publicly reachable IP or domain
- Open firewall ports: 8889 (TCP), 8189 (UDP)
- coturn on VPS with ports 3478 (UDP/TCP), 49152-49352 (UDP relay range)

---

## App Components

| Class | File | Purpose |
|---|---|---|
| CameraDiscovery | cameradiscovery.h/.cpp | Listens on UDP port 5005, parses JSON broadcast packets |
| CameraManager | cameramanager.h/.cpp | Resolves device ID to RTSP URL, owns discovery |
| VideoPlayer | videoplayer.h/.cpp | Wraps QMediaPlayer + QAudioOutput for RTSP playback |
| RecordingManager | recordingmanager.h/.cpp | Frame capture via QVideoSink, snapshot and recording |
| StreamManager | streammanager.h/.cpp | Stores mediamtx server URL via QSettings, builds WebRTC endpoint |

---

## EVB Files

| Path | Purpose |
|---|---|
| /customer/demo.sh | Boot script — sets static IP, loads modules, starts programs |
| /customer/streamapp/camera_agent_arm | Discovery daemon binary |
| /customer/streamapp/ffmpeg | Cross-compiled static FFmpeg binary |
| /customer/test/prog_vif_isp_scl_ut | Vendor camera stack (do not modify) |
| /customer/test/param_snr0.ini | Vendor camera configuration |

---

## Key Design Decisions

**Why Qt Multimedia instead of GStreamer?** GStreamer's Android package ships only static `.a` libraries — runtime plugin registration fails. Qt Multimedia uses FFmpeg 7.1.3 internally and works out of the box.

**Why ARM GNU Toolchain instead of Android NDK?** Android NDK produces Bionic libc binaries incompatible with the EVB's standard Linux. ARM GNU Toolchain (`arm-none-linux-gnueabihf`) produces correct Linux ELF binaries.

**Why UDP broadcast for discovery?** Zero configuration, no server needed. `camera_agent` auto-detects its own IP via `getifaddrs()` and broadcasts the current IP every 2 seconds. The app always gets the correct IP even if DHCP reassigns it.

**Why directed broadcast (192.168.0.255) instead of 255.255.255.255?** Routers typically block `255.255.255.255` (limited broadcast) between Ethernet and WiFi interfaces. Directed broadcast is forwarded more reliably.

**Why cloud media server instead of P2P WebRTC on EVB?** libwebrtc requires several GB to build and is incompatible with 128MB RAM ARMv7. The cloud media server pattern (Ring/Nest/Arlo) keeps the camera firmware simple — it just pushes H.264 via RTMP to a server which handles WebRTC, multi-viewer, and NAT traversal.

**Why QSettings for server URL?** The mediamtx server IP changes during development and differs between LAN and production. Storing it in QSettings means the user sets it once in the app and it persists across restarts — no recompile needed when the server changes.

**Why Tailscale instead of port forwarding?** The development environment is behind CGNAT (router WAN IP is `10.255.x.x` — a private address assigned by the ISP, not a real public IP). Port forwarding is ineffective in CGNAT. Tailscale creates a private overlay network using WireGuard, giving every enrolled device a stable `100.x.x.x` address reachable from any network. No router config required.

**Why coturn on Raspberry Pi instead of a cloud VPS?** A cloud VPS requires a bank card for signup. coturn on the Pi reachable via Tailscale achieves the same TURN relay functionality for the development/demo phase at zero cost. When a cloud VPS becomes available, only the ICE server URLs in `mediamtx.yml` need updating — no code changes.

**Why comment out QT_IM_MODULE in main.cpp?** Setting `QT_IM_MODULE=qtvirtualkeyboard` forces Qt's own virtual keyboard which does not integrate correctly with Android's input method framework — the keyboard never appears. Removing this line lets Android use its native soft keyboard, which works correctly with Qt's `TextEdit` and `TextField` components inside a `Flickable`.

---

## Troubleshooting

**"Searching for camera..." never disappears**
- Check `camera_agent_arm` is running on EVB: `ps | grep camera`
- Check phone and EVB are on same network
- Check router forwards UDP broadcasts between Ethernet and WiFi

**RTSP stream shows but WebRTC toggle fails**
- Confirm mediamtx is running and FFmpeg is pushing
- Check the server URL in app Settings matches your PC's Tailscale IP (`100.x.x.x`)
- Confirm Windows Firewall allows ports 8889 and 8189
- Verify `network_security_config.xml` is in `android/res/xml/` and `CMakeLists.txt` sets `QT_ANDROID_PACKAGE_SOURCE_DIR`

**WebRTC "deadline exceeded while waiting connection"**
- Confirm Tailscale is active on the phone (VPN toggle on in Tailscale app)
- Confirm coturn is running on the Pi: `sudo systemctl status coturn`
- Confirm the Pi's Tailscale IP in `mediamtx.yml` matches the actual Pi Tailscale IP shown in the dashboard
- Restart coturn if needed: `sudo systemctl restart coturn`

**EVB loopback RTSP timeout**
The vendor RTSP server does not accept connections from the EVB itself (loopback). FFmpeg must run on the PC pulling from the EVB's network IP (`192.168.0.150`), not from `127.0.0.1`.

**Binary runs on PC but fails on EVB with "ARM Bionic" error**
The binary was compiled with Android NDK instead of ARM GNU Toolchain. Recompile using `arm-none-linux-gnueabihf-g++` with `-static` flag.

**WebRTC "webpage not available" in app**
- Check `network_security_config.xml` is correctly packaged in the APK
- Run: `jar tf <apk-path> | findstr network` — should show `res/xml/network_security_config.xml`
- If missing, verify `QT_ANDROID_PACKAGE_SOURCE_DIR` is set in `CMakeLists.txt`

**mediamtx error: unknown field "webrtcICEServers2[1].credential"**
mediamtx v1.19.1 uses `password:` not `credential:` as the key name for TURN credentials. Check `mediamtx.yml` and replace `credential:` with `password:`.

**Android keyboard doesn't appear in Settings**
Ensure `QT_IM_MODULE` is not set in `main.cpp`. The line `qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"))` must be commented out or removed.

---

## Version Roadmap

| Version | Status | Scope |
|---|---|---|
| V1 | ✅ Complete | RTSP streaming, UDP discovery, CameraManager abstraction |
| V2 | ✅ Complete | Recording, snapshots, modern dark UI |
| V3 | ✅ Complete | WebRTC via mediamtx, Tailscale overlay, coturn TURN relay, configurable server URL |
| V4 | 🔲 Planned | Cloud backend, device registry, user accounts |
| V5 | 🔲 Planned | Remote access from anywhere, push notifications, cloud recording |

---

## License

Internal project — Innofusion. Not for public distribution.
