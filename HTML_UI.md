# HTML UI - Quick Start

## 🌐 New Experimental HTML Interface

This branch provides an alternative HTML-based UI that's **much lighter and faster** than the Qt GUI on Raspberry Pi!

### Why HTML UI?

- ✅ **No Qt dependencies** - Just needs a browser (Chromium pre-installed on Pi)
- ⚡ **GPU-accelerated rendering** - Smoother animations, better performance  
- 🚀 **Faster builds** - 2-3 seconds vs 8-10 minutes for Qt
- 🔌 **Remote access** - Access from any device on your network
- 💾 **Lower memory** - ~20-30MB vs ~50MB+ for Qt
- 🎨 **Easy UI updates** - Edit HTML/CSS/JS without recompiling

### Quick Start

```bash
# Build web server only (faster)
cmake -B build -DBUILD_QT_GUI=OFF -DBUILD_WEB_SERVER=ON
cmake --build build

# Launch
./scripts/start-web.sh
# Or directly:
./build/curecraft-web
```

Then open browser to: **http://localhost:8080**

### Architecture

```
C++ Backend          │  HTML Frontend
─────────────────────┼──────────────────
Hardware Interface   │  Canvas Charts
Signal Processing    │  UI Controls  
HTTP Server          │  Animations
Real-time Streaming  │  60 FPS Rendering
```

**All hardware communication stays in C++!** The HTML is just for the UI.

### Build Options

```bash
# Web server only (recommended for Pi)
cmake -B build -DBUILD_QT_GUI=OFF -DBUILD_WEB_SERVER=ON

# Both Qt and Web server
cmake -B build -DBUILD_QT_GUI=ON -DBUILD_WEB_SERVER=ON

# Qt only (original)
cmake -B build -DBUILD_QT_GUI=ON -DBUILD_WEB_SERVER=OFF
```

### Screenshots

See the walkthrough for screenshots and demo video!

### Hardware Integration

To connect real sensors, edit `src/signal_generator.cpp` and replace the simulated signals with actual hardware readings (I2C, SPI, GPIO, UART, etc.). All sensor code stays in C++!

### Remote Access

Access the monitor from any device on your network:

```bash
# Find your Pi's IP
hostname -I

# Access from another device
http://192.168.1.X:8080
```

---

For the original Qt GUI, see [README.md](README.md)
