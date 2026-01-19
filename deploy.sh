#!/usr/bin/env bash
set -euo pipefail

REPO_DIR="/home/admin/Code/CureCraft-Labs"
REMOTE="origin"
BRANCH="main"
BUILD_DIR="${REPO_DIR}/build"
APP_NAME="curecraft"
APP_PATH="${BUILD_DIR}/${APP_NAME}"
SERVICE_NAME="curecraft.service"
SERVICE_FILE="/etc/systemd/system/${SERVICE_NAME}"

MODE="${MODE:-systemd}" # systemd | run

die() { echo "ERROR: $*" >&2; exit 1; }
need() { command -v "$1" >/dev/null || die "Missing: $1"; }

need git
need cmake
need ninja

# ---- Qt Setup ----
echo "=== Checking Qt installation ==="
QT_PATH=""

# Try common Qt installation paths
for QT_CANDIDATE in \
    /opt/Qt \
    /usr/lib/x86_64-linux-gnu/cmake/Qt6 \
    /usr/lib/x86_64-linux-gnu/cmake/Qt5 \
    /usr/local/opt/qt \
    "$HOME/Qt"; do
    if [[ -d "$QT_CANDIDATE" ]]; then
        QT_PATH="$QT_CANDIDATE"
        echo "Found Qt at: $QT_PATH"
        break
    fi
done

if [[ -z "$QT_PATH" ]]; then
    echo "WARNING: Qt not found in common locations"
    echo "Attempting to use system Qt (if installed via package manager)..."
    # Try to find via pkg-config or let CMake search system paths
    QT_PATH=$(pkg-config --variable=libdir Qt6Core 2>/dev/null || echo "")
fi

# Set CMAKE_PREFIX_PATH if Qt found
CMAKE_PREFIX_PATH=""
if [[ -n "$QT_PATH" ]]; then
    CMAKE_PREFIX_PATH="-DCMAKE_PREFIX_PATH=$QT_PATH"
    echo "Using Qt path: $QT_PATH"
fi

cd "$REPO_DIR" || die "Repo not found: $REPO_DIR"
git rev-parse --is-inside-work-tree >/dev/null

echo "=== Update ==="
git fetch "$REMOTE" "$BRANCH"
git reset --hard "$REMOTE/$BRANCH"
git submodule update --init --recursive

echo "=== Build ==="
cmake -S "$REPO_DIR" -B "$BUILD_DIR" -G Ninja $CMAKE_PREFIX_PATH
cmake --build "$BUILD_DIR" -j "$(nproc)"

[[ -x "$APP_PATH" ]] || die "Binary not found/executable: $APP_PATH"

echo "=== Setup systemd service ==="
if [[ "$MODE" == "systemd" ]]; then
    SERVICE_CONTENT="[Unit]
Description=CureCraft Patient Monitor
After=network.target

[Service]
Type=simple
User=admin
WorkingDirectory=${REPO_DIR}
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
    
    echo "=== Restart (MODE=systemd) ==="
    sudo systemctl restart "${SERVICE_NAME}"
    sleep 2
    sudo systemctl --no-pager --full status "${SERVICE_NAME}" || true
else
    echo "=== Running directly (MODE=run) ==="
    exec "$APP_PATH"
fi

echo "=== Done ==="
