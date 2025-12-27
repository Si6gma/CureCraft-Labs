#!/usr/bin/env bash
set -euo pipefail

SERVICE="${1:-}"

if [[ -n "$SERVICE" ]]; then
  echo "Following logs for service: $SERVICE"
  sudo journalctl -u "$SERVICE" -f
else
  echo "Following all journal logs"
  sudo journalctl -f
fi
