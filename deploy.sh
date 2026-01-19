#!/usr/bin/env bash
set -euo pipefail

# CureCraft-Labs Deployment Script
# Updates code from git and starts the service

REPO_DIR="/home/admin/Code/CureCraft-Labs"
REMOTE="origin"
BRANCH="main"
BUILD_DIR="${REPO_DIR}/build"
APP_NAME="curecraft"
APP_PATH="${BUILD_DIR}/${APP_NAME}"
SERVICE_NAME="curecraft.service"
SERVICE_FILE="/etc/systemd/system/${SERVICE_NAME}"

die() { echo "ERROR: $*" >&2; exit 1; }
need() { command -v "$1" >/dev/null || die "Missing: $1"; }

need git
need cmake
need ninja

echo "=== CureCraft Deploy ==="

# Update code
echo "=== Fetching latest code ==="
cd "$REPO_DIR" || die "Repo not found: $REPO_DIR"
git fetch "$REMOTE" "$BRANCH"
git reset --hard "$REMOTE/$BRANCH"
git submodule update --init --recursive

# Build
echo "=== Building ==="
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake -G Ninja ..
ninja

[[ -x "$APP_PATH" ]] || die "Build failed: $APP_PATH not found"

# Setup systemd service
echo "=== Installing systemd service ==="
SERVICE_CONTENT="[Unit]
Description=CureCraft Patient Monitor
After=network.target

[Service]
Type=simple
User=admin
WorkingDirectory=${REPO_DIR}
Environment="DISPLAY=:0"
Environment="XAUTHORITY=/home/admin/.Xauthority"
Environment="QT_QPA_PLATFORM=xcb"
ExecStart=${APP_PATH}
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
"

echo "$SERVICE_CONTENT" | sudo tee "$SERVICE_FILE" > /dev/null
sudo systemctl daemon-reload

# Start service
echo "=== Starting service ==="
sudo systemctl restart "${SERVICE_NAME}"
sleep 2

echo "=== Status ==="
sudo systemctl status "${SERVICE_NAME}"

echo "=== Done ==="
echo "View logs: journalctl -u curecraft.service -f"
