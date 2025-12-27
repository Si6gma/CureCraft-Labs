#!/usr/bin/env bash
set -euo pipefail

REPO_DIR="/home/admin/Code/CureCraft-Labs"
REMOTE="origin"
BRANCH="main"
BUILD_DIR="${REPO_DIR}/build"
APP_NAME="curecraft"
APP_PATH="${BUILD_DIR}/${APP_NAME}"

MODE="${MODE:-systemd}" # systemd | run

die() { echo "ERROR: $*" >&2; exit 1; }
need() { command -v "$1" >/dev/null || die "Missing: $1"; }

need git
need cmake
need ninja

cd "$REPO_DIR" || die "Repo not found: $REPO_DIR"
git rev-parse --is-inside-work-tree >/dev/null

echo "=== Update ==="
git fetch "$REMOTE" "$BRANCH"
git reset --hard "$REMOTE/$BRANCH"
git submodule update --init --recursive

echo "=== Build ==="
cmake -S "$REPO_DIR" -B "$BUILD_DIR" -G Ninja
cmake --build "$BUILD_DIR" -j "$(nproc)"

[[ -x "$APP_PATH" ]] || die "Binary not found/executable: $APP_PATH"

echo "=== Restart (MODE=$MODE) ==="
if [[ "$MODE" == "systemd" ]]; then
  systemctl restart curecraft.service
  systemctl --no-pager --full status curecraft.service || true
else
  exec "$APP_PATH"
fi

echo "=== Done ==="
