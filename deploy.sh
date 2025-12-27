#!/usr/bin/env bash
set -euo pipefail

# ---- Config ----
REPO_DIR="${HOME}/Code/CureCraft-Labs"
REMOTE="origin"
BRANCH="main"
BUILD_DIR="${REPO_DIR}/build"
APP_NAME="curecraft"
APP_PATH="${BUILD_DIR}/${APP_NAME}"

LOG_DIR="${REPO_DIR}/logs"
LOG_FILE="${LOG_DIR}/${APP_NAME}.log"
PID_FILE="${REPO_DIR}/${APP_NAME}.pid"

# If you later use systemd to run the app, set MODE=systemd in the service
MODE="${MODE:-nohup}"  # nohup | systemd

# ---- Helpers ----
die() { echo "ERROR: $*" >&2; exit 1; }

need_cmd() { command -v "$1" >/dev/null 2>&1 || die "Missing command: $1"; }

stop_nohup_app() {
  if [[ -f "$PID_FILE" ]]; then
    local pid
    pid="$(cat "$PID_FILE" || true)"

    if [[ -n "${pid:-}" ]] && kill -0 "$pid" 2>/dev/null; then
      echo "Stopping $APP_NAME (pid $pid)..."
      kill -TERM "$pid" || true

      # wait up to 5s, then force kill
      for _ in {1..50}; do
        if ! kill -0 "$pid" 2>/dev/null; then break; fi
        sleep 0.1
      done

      if kill -0 "$pid" 2>/dev/null; then
        echo "Force killing $APP_NAME (pid $pid)..."
        kill -KILL "$pid" || true
      fi
    fi
  fi
  rm -f "$PID_FILE"
}

start_nohup_app() {
  mkdir -p "$LOG_DIR"
  echo "Starting $APP_NAME (nohup)..."
  nohup "$APP_PATH" >>"$LOG_FILE" 2>&1 &
  local pid=$!
  echo "$pid" >"$PID_FILE"
  echo "Started (pid $pid, logs: $LOG_FILE)"
}

restart_systemd_app() {
  echo "Restarting systemd service curecraft.service..."
  systemctl restart curecraft.service
  systemctl --no-pager --full status curecraft.service || true
}

# ---- Preconditions ----
need_cmd git
need_cmd cmake
need_cmd ninja

[[ -d "$REPO_DIR" ]] || die "Repo dir not found: $REPO_DIR"

cd "$REPO_DIR"
git rev-parse --is-inside-work-tree >/dev/null

echo "=== Updating repo: $REPO_DIR ==="
git fetch "$REMOTE" "$BRANCH"
git reset --hard "$REMOTE/$BRANCH"
git submodule update --init --recursive

echo "=== Building ==="
cmake -S "$REPO_DIR" -B "$BUILD_DIR" -G Ninja
cmake --build "$BUILD_DIR" -j "$(nproc)"

[[ -x "$APP_PATH" ]] || die "Built binary not found/executable: $APP_PATH"

echo "=== Restarting app (MODE=$MODE) ==="
if [[ "$MODE" == "systemd" ]]; then
  restart_systemd_app
else
  stop_nohup_app
  start_nohup_app
fi

echo "=== Done ==="
