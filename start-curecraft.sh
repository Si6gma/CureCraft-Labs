#!/usr/bin/env bash
# Launcher for CureCraft - displays on native screen
# Works both locally (HDMI) and remotely (when viewing desktop via VNC)

APP_PATH="/home/admin/Code/CureCraft-Labs/build/curecraft"

echo "[$(date '+%Y-%m-%d %H:%M:%S')] Starting CureCraft Patient Monitor..."
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Display will appear on local screen (visible via HDMI or VNC desktop viewer)"

# Run on native display - works for both HDMI and VNC viewing
exec "$APP_PATH"
