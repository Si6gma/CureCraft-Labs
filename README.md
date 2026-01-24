# CureCraft-Labs

**Patient Monitor GUI for Raspberry Pi 400**

Real-time medical signal visualization system displaying ECG, SpO2, and respiratory waveforms, optimized for embedded performance on Raspberry Pi hardware.

---

## Quick Start

```bash
./scripts/setup.sh    # First-time setup (installs dependencies)
./scripts/deploy.sh   # Build and start the app
```

After setup, use convenient aliases:
```bash
upd     # Update and deploy
status  # Check service status
jctl    # View live logs
jclr    # Clear logs
```

---

## Features

🏥 **Medical Signal Display**
- ECG waveform with realistic heartbeat simulation
- SpO2 (blood oxygen) monitoring
- Respiratory rate visualization
- Toggle individual signals on/off

⚡ **Performance Optimized**
- Runs smoothly at 20 FPS on Raspberry Pi 400
- ccache for 95% faster incremental builds (5-30 sec vs 10 min)
- Zero-copy plotting with QVector
- Batch rendering for minimal CPU usage

🖥️ **Display Options**
- Works on HDMI (local screen)
- Works via VNC (remote viewing)
- Auto-starts on boot as systemd user service

---

## Usage

### Daily Workflow

```bash
upd        # Pull latest code, rebuild, restart
status     # Check if app is running
jctl       # Watch live logs
```

### Service Management

```bash
systemctl --user status curecraft.service    # Check status
systemctl --user stop curecraft.service      # Stop
systemctl --user start curecraft.service     # Start
systemctl --user restart curecraft.service   # Restart
```

---

## Project Structure

```
CureCraft-Labs/
├── scripts/           # All helper scripts
│   ├── setup.sh       # Install dependencies
│   ├── deploy.sh      # Deploy and restart (alias: upd)
│   ├── status.sh      # Service status
│   ├── journal-*.sh   # Log viewing/clearing
│   └── start-*.sh     # Launcher scripts
├── src/               # C++ source code
│   ├── main.cpp
│   ├── math.cpp
│   └── gui/
│       ├── mainwindow.cpp
│       ├── mainwindow.ui
│       └── qcustomplot.cpp
├── include/           # Header files
├── CMakeLists.txt     # Build configuration
├── README.md          # This file
└── SETUP.md           # Detailed setup guide
```

---

## Performance

- **Build**: 8-10 min (first) → 5-30 sec (incremental)
- **FPS**: Smooth 20 FPS rendering
- **Optimizations**: ccache, O3, native arch, batch rendering

---

## Requirements

- Raspberry Pi 400 (BCM2711, 4 cores @ 1.8 GHz, 4GB RAM)
- Raspberry Pi OS (Debian-based)
- Qt5 or Qt6

---

## Documentation

- **[SETUP.md](SETUP.md)** - Detailed setup instructions
- Project uses modern C++17
- Auto-discovers source files (just add .cpp to src/)

---

## Team

This our lovely team, we love our team.
