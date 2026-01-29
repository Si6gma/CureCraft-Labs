# CureCraft-Labs

**Web-Based Patient Monitor for Raspberry Pi**

Real-time medical signal visualization system with automatic sensor detection, serving a responsive web interface for ECG, SpO2, respiratory, and temperature monitoring.

---

## Features

### 🏥 Medical Monitoring
- **ECG** - Real-time electrocardiogram waveform display
- **SpO2** - Blood oxygen saturation monitoring with plethysmograph
- **Respiratory** - Breathing rate visualization
- **Temperature** - Dual temperature sensing (core and skin)
- **NIBP** - Non-invasive blood pressure monitoring

### 🔌 Automatic Sensor Detection
- Hot-plug sensor detection via SAMD21 SensorHub
- I2C communication over multiple sensor buses
- Automatic rescanning every 3 seconds
- Dynamic UI updates when sensors connect/disconnect

### 🌐 Web-Based Interface
- **Responsive Design** - Access from any device with a browser
- **Real-time Streaming** - Server-Sent Events for live data
- **20 FPS Updates** - Smooth waveform rendering
- **Authentication** - Simple login system
- **Dark Mode** - Modern, eye-friendly interface

### ⚡ Performance Optimized
- Runs on Raspberry Pi 400 with excellent performance
- Incremental builds: 5-30 seconds (ccache)
- Embedded web server (no external dependencies)
- Mock mode for testing without hardware

---

## Quick Start

```bash
# Clone repository
cd ~/Code
git clone https://github.com/Si6gma/CureCraft-Labs.git
cd CureCraft-Labs

# First-time setup (installs dependencies)
./scripts/setup.sh

# Build and deploy
./scripts/deploy.sh
```

Access the web interface:
```
http://localhost:8080
```

**For detailed setup instructions**, see [RASPBERRY_PI_SETUP.md](RASPBERRY_PI_SETUP.md)

---

## Architecture

### System Overview

```
┌─────────────────┐     HTTP/SSE      ┌──────────────────┐
│   Web Browser   │ ◄────────────────►│  C++ Web Server  │
│  (Dashboard UI) │   Port 8080       │   (httplib)      │
└─────────────────┘                   └────────┬─────────┘
                                               │
                                               │
                                 ┌─────────────▼──────────┐
                                 │   Signal Generator     │
                                 │  (Synthesized Medical  │
                                 │      Waveforms)        │
                                 └─────────────┬──────────┘
                                               │
                                 ┌─────────────▼──────────┐
                                 │   Sensor Manager       │
                                 │  (Presence Detection)  │
                                 └─────────────┬──────────┘
                                               │ I2C
                                 ┌─────────────▼──────────┐
                                 │   SAMD21 SensorHub     │
                                 │  (Multi-bus I2C Mux)   │
                                 └─────────────┬──────────┘
                                               │
                        ┌──────────────────────┼──────────────────────┐
                        │ W1                   │ W2                   │
                 ┌──────▼──────┐        ┌──────▼──────┐       ┌──────▼──────┐
                 │ ECG (0x40)  │        │ NIBP (0x43) │       │ Temp (0x68) │
                 │ SpO2 (0x41) │        │ Temp (0x68) │       │             │
                 └─────────────┘        └─────────────┘       └─────────────┘
```

### Technology Stack

- **Backend**: C++17, httplib (embedded web server)
- **Frontend**: HTML5, CSS3, Vanilla JavaScript
- **Real-time Communication**: Server-Sent Events (SSE)
- **Build System**: CMake + Ninja, ccache
- **Hardware Interface**: Linux I2C (`/dev/i2c-1`)
- **Service Management**: systemd user service

### Key Components

| Component | Location | Purpose |
|-----------|----------|---------|
| Web Server | `src/server/webserver.cpp` | HTTP server, SSE streaming, API endpoints |
| Signal Generator | `src/core/signal_generator.cpp` | Synthesizes realistic medical waveforms |
| Sensor Manager | `src/hardware/sensor_manager.cpp` | Detects sensors via I2C hub |
| I2C Driver | `src/hardware/i2c_driver.cpp` | Low-level I2C communication |
| Frontend | `web/` | HTML/CSS/JS dashboard interface |
| Firmware | `firmware/sensor_hub/` | SAMD21 Arduino firmware |

---

## Project Structure

```
CureCraft-Labs/
├── src/                    # C++ source code
│   ├── main.cpp            # Application entry point
│   ├── core/               # Signal generation
│   ├── hardware/           # I2C drivers, sensor management
│   └── server/             # Web server, authentication
├── include/                # Header files
│   ├── core/
│   ├── hardware/
│   └── server/
├── web/                    # Web interface
│   ├── index.html          # Dashboard UI
│   ├── app.js              # Frontend logic
│   └── styles.css          # Styling
├── firmware/               # SAMD21 firmware
│   └── sensor_hub/         # Arduino sketch for hub
├── scripts/                # Deployment & management scripts
│   ├── setup.sh            # Install dependencies
│   ├── deploy.sh           # Build and deploy
│   └── start-curecraft.sh  # Service launcher
├── tests/                  # Diagnostic tools
│   └── diagnostics/        # I2C testing utilities
├── CMakeLists.txt          # Build configuration
└── README.md               # This file
```

---

## Development

### Adding New Features

```bash
# Make code changes in src/ or include/
nano src/core/my_feature.cpp

# CMake auto-discovers new files - just rebuild
upd

# View logs
jctl
```

### API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Serve dashboard UI |
| `/api/login` | POST | User authentication |
| `/api/sensors` | GET | Get sensor status (JSON) |
| `/api/status` | GET | Server status and metrics |
| `/ws` | GET | Real-time data stream (SSE) |

### Build Configuration

**Optimizations enabled:**
- `-O3` - Maximum optimization
- `-march=native` - Pi 400 CPU features
- `-ffast-math` - Fast floating-point
- `-flto` - Link-time optimization
- ccache - Compilation cache

**MISRA-C++ Compliance:**
The codebase follows MISRA-C++ guidelines where practical:
- Explicit type conversions
- Named constants instead of magic numbers
- Const correctness
- Defensive initialization

---

## Command Reference

After running `./scripts/setup.sh`, use these aliases:

```bash
upd      # Update code, rebuild, restart service
status   # Check if service is running
jctl     # View live logs
jclr     # Clear old logs
```

Manual service control:
```bash
systemctl --user start curecraft.service
systemctl --user stop curecraft.service
systemctl --user restart curecraft.service
```

---

## Documentation

- **[RASPBERRY_PI_SETUP.md](RASPBERRY_PI_SETUP.md)** - Complete Pi setup guide
- **[firmware/README.md](firmware/sensor_hub/README.md)** - SAMD21 firmware documentation
- **[include/hardware/i2c_protocol.h](include/hardware/i2c_protocol.h)** - I2C protocol specification

---

## Performance

| Metric | Value |
|--------|-------|
| **First build** | 8-10 minutes |
| **Incremental build** | 5-30 seconds |
| **Frame rate** | 20 FPS |
| **Update interval** | 50ms |
| **Memory usage** | ~50MB |
| **CPU usage** | ~15% (one core) |

---

## Requirements

- **Hardware**: Raspberry Pi 400 (BCM2711, 4 cores @ 1.8 GHz, 4GB RAM)
- **OS**: Raspberry Pi OS (Debian 12 Bookworm or later)
- **Disk**: ~500MB build + 2GB ccache
- **Network**: For web access (localhost or remote)

**Optional:**
- SAMD21 SensorHub for hardware sensor detection
- Medical sensors (ECG, SpO2, Temperature, NIBP)

---

## License

This project is part of CureCraft Labs medical device development.

---

## Team

Built with ❤️ by the CureCraft Labs team.
