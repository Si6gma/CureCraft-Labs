# Raspberry Pi Setup Guide

Complete installation and deployment guide for CureCraft Patient Monitor on Raspberry Pi 400.

---

## Hardware Requirements

- **Raspberry Pi 400** (BCM2711, 4 cores @ 1.8 GHz, 4GB RAM)
- **Operating System**: Raspberry Pi OS (Debian 12 Bookworm or later)
- **Storage**: ~500MB for build artifacts + 2GB for ccache
- **Display**: HDMI monitor or VNC access
- **Network**: Internet connection for setup

### I2C Hardware Setup

The system communicates with the SAMD21 SensorHub via I2C:

- **GPIO 2 (SDA)** → SAMD21 PA23 (W0_SDA)
- **GPIO 3 (SCL)** → SAMD21 PA22 (W0_SCL)
- This uses **I2C Bus 1** (`/dev/i2c-1`)

**Enable I2C on Raspberry Pi:**

```bash
sudo raspi-config
# Interface Options → I2C → Enable
sudo reboot
```

Verify I2C is enabled:

```bash
ls /dev/i2c-*
# Should show: /dev/i2c-1
```

---

## Initial Setup

### 1. Clone Repository

```bash
cd ~/Code
git clone https://github.com/Si6gma/CureCraft-Labs.git
cd CureCraft-Labs
```

### 2. Run Setup Script

Install dependencies and configure the system:

```bash
./scripts/install-dependencies.sh
```

**What it installs:**

- Build tools: `cmake`, `ninja-build`, `ccache`, `build-essential`
- I2C tools: `i2c-tools`, `libi2c-dev`
- System libraries: `libssl-dev`, `libpthread-stubs0-dev`

**Command aliases created in `~/bin`:**

- `upd` → Update code and deploy
- `status` → Check service status
- `jctl` → View live logs
- `jclr` → Clear old logs

After setup, restart your shell or run:

```bash
source ~/.bashrc
```

### 3. First Deployment

Build and start the application:

```bash
./scripts/deploy.sh
# Or use the alias:
upd
```

**Build times:**

- **First build**: ~8-10 minutes (compiles all dependencies, populates ccache)
- **Incremental builds**: 5-30 seconds (only changed files)

---

## Deploy Script Workflow

The `deploy.sh` script automates the complete update and deployment process:

```bash
./scripts/deploy.sh
```

**What it does:**

1. Pulls latest changes from git (`git pull`)
2. Creates/updates build directory
3. Configures CMake with optimizations (`-O3`, `-march=native`, LTO)
4. Builds using Ninja (parallel compilation)
5. Installs systemd user service
6. Restarts the service

**Build optimizations:**

- **ccache**: 95% faster incremental builds
- **-O3**: Maximum performance optimization
- **-march=native**: Uses Pi 400 CPU features
- **LTO**: Link-time optimization
- **Parallel builds**: Uses all 4 CPU cores

---

## Systemd Service Management

The application runs as a systemd user service for automatic startup and management.

### Service File Location

`~/.config/systemd/user/curecraft.service`

### Service Commands

```bash
# Check status
systemctl --user status curecraft.service

# Start service
systemctl --user start curecraft.service

# Stop service
systemctl --user stop curecraft.service

# Restart service
systemctl --user restart curecraft.service

# View logs (live)
journalctl --user -u curecraft.service -f

# View recent logs
journalctl --user -u curecraft.service -n 100
```

### Auto-start on Boot

The setup script enables auto-start using `loginctl enable-linger`:

```bash
# Check linger status
loginctl show-user $USER | grep Linger

# Should show: Linger=yes
```

This ensures the service starts even when not logged in via SSH/VNC.

---

## Command Aliases

After running `install-dependencies.sh`, these shortcuts are available:

| Alias    | Full Command                                | Description                                |
| -------- | ------------------------------------------- | ------------------------------------------ |
| `upd`    | `./scripts/deploy.sh`                       | Pull latest code, rebuild, restart service |
| `status` | `./scripts/status.sh`                       | Show service status                        |
| `jctl`   | `journalctl --user -u curecraft.service -f` | Follow live logs                           |
| `jclr`   | `journalctl --user --vacuum-time=1s`        | Clear old journal logs                     |

---

## Daily Workflow

### Regular Updates

```bash
upd        # Pull code, rebuild, restart
status     # Verify service is running
jctl       # Watch logs for errors
```

### Accessing the Web Interface

After the service starts, open a browser:

```
http://localhost:8080
```

Or from another device on the network:

```
http://<pi-ip-address>:8080
```

Default credentials:

- **Username**: `admin`
- **Password**: `admin`

---

## Troubleshooting

### Service Won't Start

**Check service status:**

```bash
status
```

**View detailed logs:**

```bash
journalctl --user -u curecraft.service -n 100
```

**Common causes:**

- Port 8080 already in use
- Web directory not found
- I2C bus not accessible

**Try manual run:**

```bash
systemctl --user stop curecraft.service
cd ~/Code/CureCraft-Labs/build
./curecraft --mock
```

### Build Fails

**Clean rebuild:**

```bash
rm -rf build/
upd
```

**Check ccache:**

```bash
ccache --show-stats
```

If ccache isn't working:

```bash
sudo apt-get install ccache
./scripts/install-dependencies.sh
```

### I2C Not Working

**Verify I2C is enabled:**

```bash
ls /dev/i2c-*
```

**Check for SensorHub:**

```bash
sudo i2cdetect -y 1
```

Should show device at `0x08` if hub is connected.

**Enable I2C if missing:**

```bash
sudo raspi-config
# Interface Options → I2C → Enable
sudo reboot
```

### Web Interface Not Accessible

**Check service is running:**

```bash
status
```

**Verify port is listening:**

```bash
sudo netstat -tulpn | grep 8080
```

**Check firewall (if enabled):**

```bash
sudo ufw status
sudo ufw allow 8080/tcp
```

**Try different port:**

```bash
systemctl --user stop curecraft.service
cd ~/Code/CureCraft-Labs/build
./curecraft --port 3000
```

### Slow Builds

**Check ccache stats:**

```bash
ccache --show-stats
```

Look for high hit rate (should be >90% after first build).

**Configure ccache size:**

```bash
ccache --max-size=2G
```

**View cache location:**

```bash
du -sh ~/.ccache
```

### Disk Space Issues

**Clear old logs:**

```bash
jclr
```

**Check disk usage:**

```bash
df -h
```

**Clear build artifacts:**

```bash
rm -rf ~/Code/CureCraft-Labs/build
```

---

## Advanced Configuration

### Running in Mock Mode

Test without hardware sensors:

```bash
systemctl --user stop curecraft.service
cd ~/Code/CureCraft-Labs/build
./curecraft --mock
```

### Changing Port

Edit the service file:

```bash
nano ~/.config/systemd/user/curecraft.service
```

Add `--port` argument to `ExecStart`:

```
ExecStart=/home/pi/Code/CureCraft-Labs/scripts/start-curecraft.sh --port 3000
```

Reload and restart:

```bash
systemctl --user daemon-reload
systemctl --user restart curecraft.service
```

### Performance Tuning

**VNC optimization:**

- Use solid colors instead of gradients
- Limit resolution to 1024x768
- Disable desktop animations

**Build performance:**

```bash
# See detailed ccache stats
ccache --show-stats --verbose

# Increase ccache size for faster builds
ccache --max-size=4G
```

---

## Uninstall

```bash
# Stop and disable service
systemctl --user stop curecraft.service
systemctl --user disable curecraft.service

# Remove service file
rm ~/.config/systemd/user/curecraft.service

# Remove command aliases
rm ~/bin/{upd,status,jctl,jclr}

# Remove code
rm -rf ~/Code/CureCraft-Labs

# (Optional) Remove ccache
rm -rf ~/.ccache
```

---

## Next Steps

- **Firmware Setup**: See [firmware/README.md](firmware/sensor_hub/README.md) for SAMD21 configuration
- **Technical Details**: See main [README.md](README.md) for architecture overview
- **Development**: Modify code in `src/` and run `upd` to rebuild
