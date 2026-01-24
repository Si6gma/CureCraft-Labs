#!/usr/bin/env bash
# Smart launcher for CureCraft - tries VNC first, falls back to HDMI

APP_PATH="/home/admin/Code/CureCraft-Labs/build/curecraft"
LOG_TAG="curecraft-launcher"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $LOG_TAG: $*"
}

log "Starting CureCraft Patient Monitor..."

# Check if VNC server is available on port 5900
if netstat -tln | grep -q ':5900 '; then
    log "VNC server detected on port 5900 - using VNC display"
    export QT_QPA_PLATFORM=vnc
    exec "$APP_PATH"
else
    log "No VNC server detected - using HDMI display"
    # Let Qt auto-detect the display (will use X11 on HDMI)
    exec "$APP_PATH"
fi
