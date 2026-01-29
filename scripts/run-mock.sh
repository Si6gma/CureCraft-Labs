#!/usr/bin/env bash
# Start CureCraft in MOCK mode (for development/testing without hardware)
# This simulates all sensors and doesn't require actual I2C devices

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
APP_PATH="$PROJECT_ROOT/build/curecraft"

# Detect OS
OS_TYPE="$(uname -s)"

# Check if binary exists
if [ ! -f "$APP_PATH" ]; then
    echo "ERROR: Binary not found at $APP_PATH"
    echo ""
    echo "The project needs to be built first."
    echo ""
    
    if [ "$OS_TYPE" = "Darwin" ]; then
        # macOS-specific instructions
        echo "📦 macOS Development Setup:"
        echo ""
        echo "1. Install Homebrew (if not installed):"
        echo "   /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
        echo ""
        echo "2. Install dependencies:"
        echo "   brew install cmake"
        echo ""
        echo "3. Build the project:"
        echo "   cd $PROJECT_ROOT"
        echo "   cmake -B build"
        echo "   cmake --build build"
        echo ""
        echo "Then run this script again: ./scripts/run-mock.sh"
    else
        # Linux/Pi instructions
        echo "Build the project:"
        echo "   cd $PROJECT_ROOT"
        echo "   cmake -B build"
        echo "   cmake --build build"
        echo ""
        echo "If CMake is not installed, run: ./scripts/install-dependencies.sh"
    fi
    
    exit 1
fi

# Ensure we run from the project root so default paths work
cd "$PROJECT_ROOT"

echo "==========================================="
echo "  CureCraft Patient Monitor - MOCK MODE  "
echo "==========================================="
echo ""
echo "🎭 Mock mode enabled - simulating all sensors"
echo "🌐 Web interface: http://localhost:8080"
echo "📂 Project root: $PROJECT_ROOT"
echo ""
echo "Press Ctrl+C to stop"
echo ""

# Start the web server with --mock flag
exec "$APP_PATH" --mock --web-root "$PROJECT_ROOT/web"
