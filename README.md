# CureCraft Patient Monitor

> **Real-time medical monitoring system for Raspberry Pi with web-based interface**

![Screenshot coming soon]

---

## What is CureCraft?

CureCraft is a **web-based patient monitoring system** that runs on Raspberry Pi. It displays real-time medical waveforms (ECG, SpO2, respiratory rate, temperature) through any web browser—no special software needed.

**Key Features:**
- 🏥 Real-time medical signal visualization (ECG, SpO2, breathing, temperature, blood pressure)
- 🌐 Access from any device with a browser
- 🔌 Automatic sensor detection (hot-plug support)
- 🎭 Mock mode for development without hardware
- ⚡ Optimized for Raspberry Pi

---

## Quick Start

### For Development (Mac/Linux - No Hardware)

```bash
# Install dependencies
brew install cmake          # macOS
# or: sudo apt install cmake  # Linux

# Build
cmake -B build
cmake --build build

# Run with simulated sensors
./scripts/run-mock.sh
```

Open browser: `http://localhost:8080`

### For Raspberry Pi (Production)

```bash
# Clone and setup
git clone https://github.com/Si6gma/CureCraft-Labs.git
cd CureCraft-Labs
./scripts/install-dependencies.sh

# Deploy
./scripts/deploy-to-pi.sh
```

Access: `http://<pi-ip-address>:8080`

**📖 Detailed Setup:** See [RASPBERRY_PI_SETUP.md](RASPBERRY_PI_SETUP.md)

---

## How It Works

```
┌─────────────┐                  ┌──────────────┐
│ Web Browser │ ◄── HTTP/SSE ──► │ C++ Server   │
│  (Any Device) │                │ (Port 8080)  │
└─────────────┘                  └──────┬───────┘
                                        │
                                  ┌─────▼──────┐
                                  │  Sensors   │
                                  │ (Mock/Real)│
                                  └────────────┘
```

1. **Backend** (C++) generates or reads medical sensor data
2. **Server** streams data to browser via Server-Sent Events (SSE)
3. **Frontend** renders waveforms on HTML5 canvas at 20 FPS

**🔧 Technical Details:** See [TECHNICAL_DESIGN.md](TECHNICAL_DESIGN.md)

---

## Project Structure

```
CureCraft-Labs/
├── src/                    # C++ source code
├── include/                # Header files
├── web/                    # Web interface (HTML/CSS/JS)
├── firmware/               # SAMD21 sensor hub firmware
├── scripts/                # Deployment & utilities
│   ├── run-mock.sh         # Development mode
│   └── run-production.sh   # Production mode
├── README.md               # This file
├── TECHNICAL_DESIGN.md     # Architecture & design docs
└── RASPBERRY_PI_SETUP.md   # Pi deployment guide
```

---

## Development

**Local testing (no hardware):**
```bash
./scripts/run-mock.sh
```

**On Raspberry Pi:**
```bash
./scripts/run-production.sh
```

**Making changes:**
```bash
# Edit code in src/ or web/
# Rebuild and test
cmake --build build
./scripts/run-mock.sh
```

---

## Documentation

| Document | Purpose |
|----------|---------|
| [README.md](README.md) | Overview & quick start (this file) |
| [TECHNICAL_DESIGN.md](TECHNICAL_DESIGN.md) | Architecture, API, design patterns |
| [RASPBERRY_PI_SETUP.md](RASPBERRY_PI_SETUP.md) | Pi deployment & configuration |

---

## Tech Stack

- **Backend:** C++17, httplib
- **Frontend:** HTML5, CSS3, JavaScript
- **Build:** CMake
- **Comm:** Server-Sent Events (SSE)
- **Hardware:** I2C, SAMD21 Sensor Hub

---

## Team

Built with ❤️ by Ana, Henri, Marko, Tim, Luka & Neiv
