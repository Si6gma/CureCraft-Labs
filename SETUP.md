# CureCraft-Labs Setup Guide

Complete installation and configuration guide for Raspberry Pi 400.

---

## Prerequisites

- Raspberry Pi 400 with Raspberry Pi OS
- Internet connection
- SSH or direct access to the Pi

---

## Installation Steps

### 1. Clone the Repository

```bash
cd ~/Code
git clone https://github.com/Si6gma/CureCraft-Labs.git
cd CureCraft-Labs
```

### 2. Run Setup Script

This installs all dependencies and creates command aliases:

```bash
./scripts/setup.sh
```

**What it installs:**
- Build tools (cmake, ninja, ccache, build-essential)
- Qt6 (or Qt5 fallback)
- libxkbcommon-dev (X11 keyboard support)

**Command aliases created:**
- `upd` → Update and deploy
- `status` → Service status
- `jctl` → View live logs
- `jclr` → Clear journal logs

### 3. First Deploy

Build and start the application:

```bash
./scripts/deploy.sh
# or use the alias:
upd
```

**First build**: ~8-10 minutes (compiles everything, populates ccache)  
**Subsequent builds**: 5-30 seconds (only changed files)

---

## Verification

### Check the Service

```bash
status
```

You should see:
- **Loaded**: loaded
- **Active**: active (running)

### View Logs

```bash
jctl
```

You should see: "CureCraft Patient Monitor started"

### Check the Display

The GUI should appear on your:
- **HDMI screen** (if connected locally)
- **VNC viewer** (if viewing desktop remotely)

---

## Build System Details

### ccache (Compiler Cache)

ccache dramatically speeds up rebuilds:

```bash
ccache --show-stats    # View cache statistics
ccache --clear         # Clear cache (if issues)
```

### CMake Auto-Discovery

The build system automatically finds source files:
- Just add `.cpp` files to `src/` or `src/gui/`
- Add `.h` files to `include/`
- No need to edit CMakeLists.txt!

### Build Optimizations

- **-O3**: Maximum optimization for performance
- **-march=native**: Uses Pi 400 CPU features
- **-ffast-math**: Fast floating-point operations
- **LTO**: Link-time optimization
- **Parallel builds**: Uses all 4 cores + 1

---

## Systemd Service

The app runs as a user systemd service:

**Service file**: `~/.config/systemd/user/curecraft.service`

**Auto-start**: Enabled via `loginctl enable-linger`

**Commands**:
```bash
systemctl --user status curecraft.service
systemctl --user restart curecraft.service
systemctl --user stop curecraft.service
systemctl --user start curecraft.service
```

---

## Troubleshooting

### Build Fails

```bash
rm -rf build/
upd
```

### Slow Builds

Check if ccache is working:
```bash
ccache --show-stats
```

If not installed:
```bash
sudo apt-get install ccache
./scripts/setup.sh
```

### Service Won't Start

View detailed logs:
```bash
journalctl --user -u curecraft.service -n 100
```

### Can't Find Qt

```bash
qmake --version  # Should show Qt5 or Qt6
```

Re-run setup if needed:
```bash
./scripts/setup.sh
```

### GUI Not Showing

1. Check service is running: `status`
2. Check logs: `jctl`
3. Try manual run:
   ```bash
   systemctl --user stop curecraft.service
   cd ~/Code/CureCraft-Labs/build
   ./curecraft
   ```

---

## Performance Tuning

### VNC Optimization

If using VNC:
- Disable animated backgrounds
- Use solid colors instead of gradients
- Limit resolution to 1024x768
- Match VNC server resolution to actual display

### Build Cache Management

```bash
# Check cache size
du -sh ~/.ccache

# Configure cache size (default: 2GB)
ccache --max-size=2G

# View detailed stats
ccache --show-stats --verbose
```

---

## Development

### Adding New Files

Just create the file in the appropriate directory:

```bash
# New source file
touch src/my_feature.cpp
touch include/my_feature.h
# Rebuild - CMake auto-detects it!
upd
```

### Project Layout

- `src/` - C++ implementation files (.cpp)
- `include/` - Header files (.h)
- `src/gui/` - GUI-specific code
- `scripts/` - Helper scripts
- `build/` - Build output (git-ignored)

---

## Updating

To get the latest code:

```bash
upd
```

This automatically:
1. Pulls from git
2. Rebuilds changed files (fast!)
3. Restarts the service

---

## Uninstall

```bash
# Stop and disable service
systemctl --user stop curecraft.service
systemctl --user disable curecraft.service

# Remove user service file
rm ~/.config/systemd/user/curecraft.service

# Remove command aliases
rm ~/bin/{upd,status,jctl,jclr}

# Remove code
rm -rf ~/Code/CureCraft-Labs
```

---

## Build Time Expectations

| Scenario | Time | Notes |
|----------|------|-------|
| First build | 8-10 min | Compiles 35k+ lines of QCustomPlot |
| Code change in main.cpp | 5-10 sec | ccache reuses cached objects |
| Code change in mainwindow.cpp | 10-20 sec | Recompiles GUI + links |
| No changes | <5 sec | Just checks and exits |
| Clean rebuild | 8-10 min | Only if cache cleared |

---

## System Requirements

- **CPU**: BCM2711 (4 cores @ 1.8 GHz)
- **RAM**: 4 GB
- **OS**: Raspberry Pi OS (Debian 12 Bookworm or later)
- **Disk**: ~500MB for build artifacts + 2GB for ccache
- **Display**: HDMI or VNC

---

## Support

For issues or questions, check the logs first:
```bash
jctl
```

Common issues are documented in the Troubleshooting section above.
