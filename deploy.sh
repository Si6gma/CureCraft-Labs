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

# Detect number of cores for parallel build
NUM_CORES=$(nproc || echo 4)
BUILD_JOBS=$((NUM_CORES + 1))  # One more than cores for optimal throughput

need git
need cmake
need ninja

# Enable ccache if available
if command -v ccache &> /dev/null; then
    export CCACHE_DIR="${HOME}/.ccache"
    export PATH="/usr/lib/ccache:${PATH}"
fi

echo "=== CureCraft Deploy ==="

# Update code
echo "=== Fetching latest code ==="
cd "$REPO_DIR" || die "Repo not found: $REPO_DIR"
git fetch "$REMOTE" "$BRANCH"
git reset --hard "$REMOTE/$BRANCH"
git submodule update --init --recursive

# Build
echo "=== Building (incremental, $BUILD_JOBS parallel jobs) ==="
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR" || die "Failed to enter build directory"

# Only reconfigure if CMakeLists.txt or CMakeCache changed
if [[ ! -f CMakeCache.txt ]] || [[ ../CMakeLists.txt -nt CMakeCache.txt ]]; then
    echo "Configuring CMake..."
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE="-O2 -march=native" .. || die "CMake configuration failed"
fi

# Incremental build with parallel jobs
ninja -j "$BUILD_JOBS" || die "Build failed"

[[ -x "$APP_PATH" ]] || die "Build failed: $APP_PATH not found"

# Show ccache statistics if available
if command -v ccache &> /dev/null; then
    echo "=== Build Cache Statistics ==="
    ccache --show-stats --verbose | head -n 10
fi

# Setup systemd service
echo "=== Installing systemd service ==="
SERVICE_CONTENT="[Unit]
Description=CureCraft Patient Monitor
After=network.target graphical.target

[Service]
Type=simple
User=admin
WorkingDirectory=${REPO_DIR}
Environment=\"DISPLAY=:0\"
Environment=\"XAUTHORITY=/home/admin/.Xauthority\"
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
