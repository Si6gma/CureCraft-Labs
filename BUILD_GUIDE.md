# CureCraft-Labs Build & Deploy Guide

## First Time Setup (on Linux machine)

### 1. Install Dependencies

Run the automated setup script:

```bash
./setup-dependencies.sh
```

This will detect your OS and install:

- Build tools (gcc, cmake, ninja)
- Git
- Qt6 development libraries (or Qt5 as fallback)

**Manual installation** (if needed):

```bash
# Ubuntu/Debian
sudo apt-get install -y build-essential cmake ninja-build git qt6-base-dev qt6-base-private-dev libqt6printsupport6-dev

# Fedora/RHEL
sudo dnf install -y gcc gcc-c++ cmake ninja-build git qt6-qtbase-devel qt6-qttools-devel

# Arch Linux
sudo pacman -S base-devel cmake ninja git qt6-base
```

### 2. Deploy & Build

```bash
./deploy.sh
```

This will:

- Fetch latest code from git
- Auto-detect Qt installation
- Build the project
- Create/update systemd service
- Start the service

## Running the Application

### Start the GUI

```bash
./deploy.sh MODE=run
```

Or via systemd:

```bash
sudo systemctl start curecraft.service
```

### View Logs

```bash
journalctl -u curecraft.service -f    # Follow logs
journalctl -u curecraft.service -n 50 # Last 50 lines
```

### Check Status

```bash
sudo systemctl status curecraft.service
```

### Stop Service

```bash
sudo systemctl stop curecraft.service
```

## Troubleshooting

### CMake Can't Find Qt

The `deploy.sh` script now automatically searches for Qt in common locations and sets CMAKE_PREFIX_PATH.

If it still fails, manually set:

```bash
export CMAKE_PREFIX_PATH=/path/to/qt/lib/cmake
./deploy.sh
```

Common Qt paths:

- `/opt/Qt/6.x.x/gcc_64/lib/cmake`
- `/usr/lib/x86_64-linux-gnu/cmake`
- `$HOME/Qt/6.x.x/gcc_64/lib/cmake`

### Find Qt Installation

```bash
qmake --version       # Shows Qt location
find /opt -name "Qt6Config.cmake"
locate Qt6Config.cmake
```

### Clean Build

```bash
rm -rf build/
./deploy.sh
```

## Project Structure

```
CureCraft-Labs/
├── src/
│   ├── main.cpp              # Entry point (starts GUI)
│   ├── math.cpp
│   └── gui/                  # Patient Monitor GUI
│       ├── mainwindow.cpp
│       ├── mainwindow.ui
│       └── qcustomplot.cpp
├── include/                  # Headers
├── CMakeLists.txt
├── deploy.sh                 # Build & deploy script
└── setup-dependencies.sh     # Install build requirements
```

## Systemd Service

The service is automatically created at: `/etc/systemd/system/curecraft.service`

It will:

- Start automatically on boot
- Auto-restart on crash (10 sec delay)
- Run as `admin` user
- Log to journalctl
