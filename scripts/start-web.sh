#!/bin/bash
# ============================================================================
# CureCraft Patient Monitor - Web Server Launcher
# ============================================================================

set -e

# Configuration
PORT=8080
WEB_ROOT="./web"
BUILD_DIR="./build"
EXECUTABLE="$BUILD_DIR/curecraft"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "=================================================="
echo "  CureCraft Patient Monitor - Web Server"
echo "=================================================="
echo ""

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo "❌ Error: curecraft-web executable not found!"
    echo "   Please build first: cmake --build build"
    exit 1
fi

# Check if web directory exists
if [ ! -d "$WEB_ROOT" ]; then
    echo "⚠️  Warning: web/ directory not found!"
    echo "   Creating directory..."
    mkdir -p "$WEB_ROOT"
fi

# Start the server
echo -e "${BLUE}🚀 Starting web server...${NC}"
echo ""

# Run in foreground so user can see logs and stop with Ctrl+C
"$EXECUTABLE" --port "$PORT" --web-root "$WEB_ROOT" &
SERVER_PID=$!

# Wait a moment for server to start
sleep 1

# Check if server is still running
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "❌ Server failed to start"
    exit 1
fi

echo ""
echo -e "${GREEN}✅ Server is running!${NC}"
echo ""
echo "📱 Open in browser:"
echo -e "   ${BLUE}http://localhost:$PORT${NC}"
echo ""
echo "⌨️  Press Ctrl+C to stop the server"
echo ""

# Try to open browser automatically
if command -v xdg-open > /dev/null; then
    echo "🌐 Opening browser..."
    xdg-open "http://localhost:$PORT" 2>/dev/null || true
elif command -v open > /dev/null; then
    # macOS
    open "http://localhost:$PORT" 2>/dev/null || true
fi

# Wait for server process
wait $SERVER_PID
