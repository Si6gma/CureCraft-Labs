#!/usr/bin/env bash
set -euo pipefail

# CureCraft-Labs Service Installation Script
# Sets up the systemd user service for the CureCraft application

REPO_DIR="${REPO_DIR:-/home/admin/Code/CureCraft-Labs}"
SERVICE_NAME="curecraft.service"

echo "=== Installing systemd user service ==="

SERVICE_FILE_USER="${HOME}/.config/systemd/user/${SERVICE_NAME}"
mkdir -p "${HOME}/.config/systemd/user"

SERVICE_CONTENT="[Unit]
Description=CureCraft Patient Monitor
After=graphical-session.target

[Service]
Type=simple
WorkingDirectory=${REPO_DIR}
ExecStart=${REPO_DIR}/scripts/run-production.sh
Restart=always
RestartSec=10

[Install]
WantedBy=default.target"

echo "$SERVICE_CONTENT" > "$SERVICE_FILE_USER"

# Stop and disable system service if it exists (migration)
if systemctl list-unit-files | grep -q "^${SERVICE_NAME}"; then
    echo "Removing old system service..."
    sudo systemctl stop "${SERVICE_NAME}" 2>/dev/null || true
    sudo systemctl disable "${SERVICE_NAME}" 2>/dev/null || true
fi

# Enable and start user service
systemctl --user daemon-reload
systemctl --user enable "${SERVICE_NAME}"
systemctl --user restart "${SERVICE_NAME}"

# Enable lingering so service starts on boot
loginctl enable-linger "$USER"

sleep 2

echo "✓ Service installed and started"
systemctl --user status "${SERVICE_NAME}" --no-pager || true
