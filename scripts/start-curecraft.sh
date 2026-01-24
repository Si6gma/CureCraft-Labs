#!/usr/bin/env bash
# Launcher for CureCraft - displays on native screen
# Works both locally (HDMI) and remotely (when viewing desktop via VNC)

APP_PATH="/home/admin/Code/CureCraft-Labs/build/curecraft"

echo "[$(date '+%Y-%m-%d %H:%M:%S')] Starting CureCraft Patient Monitor Web Server..."
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Web server will be available at http://localhost:8080"

# Start the web server binary
# Note: The binary runs in foreground, which is what systemd expects for Type=simple
exec "$APP_PATH"
