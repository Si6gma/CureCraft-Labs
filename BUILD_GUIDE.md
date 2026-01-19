# CureCraft-Labs Setup & Deploy Guide

## One-Time Setup

### 1. Install Dependencies

```bash
cd ~/Code/CureCraft-Labs
./setup-dependencies.sh
```

This installs: build tools, cmake, ninja, and Qt libraries for Raspberry Pi.

### 2. First Deploy

```bash
./deploy.sh
```

This will:

- Pull latest code from git
- Build the project
- Create/start the systemd service
- Application runs automatically on boot

## Daily Use

### Update and Restart

```bash
./deploy.sh
```

That's it. This pulls the latest code, rebuilds, and restarts the service.

### View Live Logs

```bash
journalctl -u curecraft.service -f
```

### Check Service Status

```bash
sudo systemctl status curecraft.service
```

### Stop Service

```bash
sudo systemctl stop curecraft.service
```

### Start Service

```bash
sudo systemctl start curecraft.service
```

## Troubleshooting

### Build Fails

```bash
rm -rf build/
./deploy.sh
```

### Can't Find Qt

Qt should auto-detect. If it fails:

```bash
qmake --version  # Check if Qt is installed
```

If nothing shows up, re-run setup:

```bash
./setup-dependencies.sh
```

### Service Won't Start

```bash
journalctl -u curecraft.service -n 100  # See last 100 log lines
```

### Manual Build & Run

```bash
cd ~/Code/CureCraft-Labs/build
./curecraft
```

## Project Structure

```
CureCraft-Labs/
├── src/
│   ├── main.cpp           # Entry point (starts GUI)
│   ├── math.cpp
│   └── gui/               # Patient Monitor GUI
│       ├── mainwindow.cpp
│       ├── mainwindow.ui
│       └── qcustomplot.cpp
├── include/               # Headers
├── CMakeLists.txt
├── deploy.sh              # Update code and restart
└── setup-dependencies.sh  # Install requirements (run once)
```

## What Happens on Boot

1. Systemd starts `curecraft.service` automatically
2. App runs in background, logs to journalctl
3. If it crashes, it auto-restarts after 10 seconds
4. View logs with: `journalctl -u curecraft.service -f`
