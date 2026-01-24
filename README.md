# CureCraft-Labs

**Patient Monitor GUI for Raspberry Pi 400**

Real-time medical signal visualization (ECG, SpO2, Respiratory) optimized for embedded systems.

---

## Quick Start

### First-Time Setup

```bash
cd ~/Code/CureCraft-Labs
./setup-dependencies.sh     # Install Qt, build tools, ccache
./deploy.sh                 # Build and start service (~8-10 min first time)
```

### Daily Updates

```bash
./deploy.sh                 # Pull latest code, rebuild, restart (~5-30 sec)
```

**Quick status check:**
```bash
./status.sh                 # View service status and helpful commands
```

The deploy script automatically:
- Pulls latest code from git
- Rebuilds only changed files
- Restarts the systemd service

### View Live Logs

```bash
journalctl --user -u curecraft.service -f
```

---

## Display Configuration

**Smart Display Switching**: The app automatically chooses the best display:

1. **VNC Priority**: If a VNC server is running (port 5900), the GUI displays via VNC
2. **HDMI Fallback**: If no VNC connection, the GUI displays on your physical HDMI screen

This allows seamless remote access via VNC when connected, while still working on the local display when VNC isn't available.

**To use VNC**: Just connect your VNC client - the app will automatically switch to VNC mode on next restart.

---

## Service Management

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
├── src/
│   ├── main.cpp           # Entry point
│   ├── math.cpp           # Math utilities
│   └── gui/               # Patient Monitor GUI
│       ├── mainwindow.cpp
│       ├── mainwindow.ui
│       └── qcustomplot.cpp (35k+ lines)
├── include/               # Headers
├── CMakeLists.txt         # Build configuration
├── deploy.sh              # Deploy and restart
└── setup-dependencies.sh  # Install dependencies
```

---

## Performance

### Build Times (Raspberry Pi 400)

| Scenario | Time | Notes |
|----------|------|-------|
| **First build** | ~8-10 min | Populates ccache |
| **Incremental** (code change) | 5-30 sec | Only rebuilds changed files |
| **Incremental** (no changes) | <5 sec | Uses cached objects |

### Runtime Performance

- **GUI Update Rate**: 20 Hz (50ms interval)
- **Memory**: ~1500 data points per signal with circular buffers
- **CPU**: Optimized with zero-copy plotting (QVector native)
- **Rendering**: Antialiasing disabled for weak GPUs

### Key Optimizations

**Build System**:
- **ccache**: Caches compiled objects (95% faster incremental builds)
- **QCustomPlot separation**: 35k line file cached as separate library
- **Ninja parallel builds**: Uses N+1 cores (5 cores on Pi 400)
- **LTO**: Link-time optimization for smaller binaries

**Runtime**:
- **Zero-copy plotting**: QVector used natively (eliminates 120 conversions/sec)
- **Adaptive sampling**: QCustomPlot reduces plotted points intelligently
- **Visibility checks**: Skips rendering hidden plots
- **Vector pre-allocation**: Prevents reallocations

---

## Troubleshooting

### Build Fails

```bash
rm -rf build/
./deploy.sh
```

### Slow Incremental Builds

Check if ccache is working:

```bash
ccache --show-stats
```

If not installed or misconfigured:

```bash
sudo apt-get install ccache
ccache --max-size=2G
ccache --set-config=compression=true
```

### Can't Find Qt

Qt should auto-detect. If it fails:

```bash
qmake --version  # Check if Qt is installed
```

Re-run setup if needed:

```bash
./setup-dependencies.sh
```

### Service Won't Start

```bash
journalctl -u curecraft.service -n 100  # Last 100 log lines
```

### Manual Build & Run

```bash
cd ~/Code/CureCraft-Labs/build
./curecraft
```

---

## Build Cache Management

### Check Cache Statistics

```bash
ccache --show-stats
```

### Clear Cache (If Needed)

```bash
ccache --clear       # Clear all cached objects
rm -rf build/        # Remove build directory
./deploy.sh          # Fresh rebuild
```

### Monitor Cache Size

```bash
ccache --show-config | grep max_size
du -sh ~/.ccache
```

---

## VNC Optimization Tips

- Disable animated backgrounds on Pi
- Use solid colors instead of gradients
- Limit window resolution to 1024x768 or less
- Run VNC server at same display size as monitor

---

## System Requirements

- **Platform**: Raspberry Pi 400 (BCM2711, 4 cores @ 1.8 GHz)
- **RAM**: 4 GB
- **OS**: Raspberry Pi OS (Debian-based)
- **Dependencies**: Qt5/Qt6, cmake, ninja, ccache, build-essential

---

## Auto-Start on Boot

The application runs as a systemd service (`curecraft.service`) that:
- Starts automatically on boot
- Restarts on crash (after 10 seconds)
- Logs to journalctl

Service file location: `/etc/systemd/system/curecraft.service`

---

## Development

### Compiler Flags

- `-O2`: Balanced optimization for embedded systems
- `-march=native`: Pi 400 CPU-specific optimizations
- `-fno-exceptions -fno-rtti`: Reduced binary size (~10%)
- `-pipe`: Reduces disk I/O during compilation

### Code Quality

- **Memory Safety**: Vectors pre-allocated, no dynamic allocations in hot path
- **Performance**: Visibility checks prevent unnecessary rendering
- **Modular**: QCustomPlot as separate cached library

---

## License

© CureCraft-Labs Team

---

## Team

This our lovely team, we love our team.
