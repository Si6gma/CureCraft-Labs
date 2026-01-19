# Raspberry Pi Setup Guide for CureCraft-Labs

## Quick Start

### 1. Update System

```bash
sudo apt-get update
sudo apt-get upgrade -y
```

### 2. Install Dependencies

The setup script now supports Raspbian:

```bash
cd ~/Code/CureCraft-Labs
./setup-dependencies.sh
```

Or manually install packages:

```bash
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    pkg-config \
    qt6-base-dev \
    libqt6gui6 \
    libqt6widgets6 \
    libqt6core6 \
    libqt6printsupport6 \
    qt6-tools-dev
```

**If Qt6 packages fail**, use Qt5 instead:

```bash
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    qt5-qmake \
    qtbase5-dev \
    libqt5widgets5 \
    libqt5printsupport5 \
    libqt5printsupport5-dev
```

### 3. Build and Deploy

```bash
cd ~/Code/CureCraft-Labs
./deploy.sh
```

### 4. Run the Application

```bash
# Via systemd (recommended - auto-restart on crash)
sudo systemctl start curecraft.service
sudo systemctl enable curecraft.service

# Or directly
./deploy.sh MODE=run

# View logs
journalctl -u curecraft.service -f
```

## Fullscreen Mode

Edit `src/main.cpp` and uncomment the fullscreen line:

```cpp
window.show();           // Remove or comment this
window.showFullScreen(); // Uncomment for fullscreen
```

Then rebuild:

```bash
rm -rf build/
./deploy.sh
```

## Performance Optimization for Raspberry Pi

### 1. Disable Desktop Compositor (if using X11)

Add to `/boot/config.txt`:

```bash
gpu_mem=256
```

### 2. Run Without Display Manager

For maximum performance, disable the GUI and run the application directly:

```bash
sudo systemctl set-default multi-user.target
sudo reboot
```

Then start the app:

```bash
cd ~/Code/CureCraft-Labs
./deploy.sh MODE=run
```

### 3. Monitor Performance

```bash
# Check CPU/Memory usage
top
htop

# Check systemd service
sudo systemctl status curecraft.service
journalctl -u curecraft.service -f
```

## Troubleshooting

### Qt Not Found

1. Check what Qt version is installed:

   ```bash
   qmake --version
   dpkg -l | grep qt
   ```

2. If only Qt5 is available, CMake will use it automatically

3. Find Qt installation path:
   ```bash
   qmake -query QT_INSTALL_CMAKE_MODULES
   ```

### Build Fails

1. Clean build directory:

   ```bash
   rm -rf build/
   ```

2. Check for errors:

   ```bash
   ./deploy.sh 2>&1 | tee build.log
   ```

3. View full CMake output:
   ```bash
   cd build/
   cat CMakeOutput.log
   ```

### Service Not Starting

1. Check status:

   ```bash
   sudo systemctl status curecraft.service
   ```

2. View logs:

   ```bash
   journalctl -u curecraft.service -n 50
   ```

3. Run directly to see errors:
   ```bash
   /home/admin/Code/CureCraft-Labs/build/curecraft
   ```

## SSH Remote Deployment

From your development machine:

```bash
ssh admin@CureCraft << 'EOF'
cd ~/Code/CureCraft-Labs
./deploy.sh
EOF
```

Watch logs in real-time:

```bash
ssh admin@CureCraft 'journalctl -u curecraft.service -f'
```

## Network Configuration

If the Raspberry Pi is headless, connect via SSH:

```bash
# Find IP
nmap -sn 192.168.x.0/24 | grep -i pi

# Connect
ssh admin@<ip_address>
```

## Systemd Service Details

Service file: `/etc/systemd/system/curecraft.service`

Key features:

- Auto-restarts on crash (10 second delay)
- Runs as `admin` user
- Logs to systemd journal
- Starts after network is ready

Commands:

```bash
sudo systemctl start curecraft.service      # Start
sudo systemctl stop curecraft.service       # Stop
sudo systemctl restart curecraft.service    # Restart
sudo systemctl status curecraft.service     # Status
sudo systemctl enable curecraft.service     # Auto-start on boot
sudo systemctl disable curecraft.service    # Disable auto-start
```
