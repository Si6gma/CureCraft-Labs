#!/usr/bin/env bash
# Launcher for CureCraft - displays on native screen
# Works both locally (HDMI) and remotely (when viewing desktop via VNC)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
APP_PATH="$PROJECT_ROOT/build/curecraft"

# Redirect output to log file for debugging since journald is having issues
LOG_FILE="$PROJECT_ROOT/curecraft.log"
exec >> "$LOG_FILE" 2>&1

echo "[$(date '+%Y-%m-%d %H:%M:%S')] Starting CureCraft Patient Monitor Web Server..."
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Web server will be available at http://localhost:8080"
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Project Root: $PROJECT_ROOT"

# Ensure we run from the project root so default paths work, 
# but also pass explicit web-root for safety
cd "$PROJECT_ROOT"

# Start the web server binary
# Note: The binary runs in foreground, which is what systemd expects for Type=simple
exec "$APP_PATH" --web-root "$PROJECT_ROOT/web"
