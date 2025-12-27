#!/usr/bin/env bash
set -euo pipefail

# SERVICE="${1:-}"

  echo "Following logs for service: curecraft"
  sudo journalctl -u curecraft -f