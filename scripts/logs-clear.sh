#!/usr/bin/env bash
set -euo pipefail

echo "Rotating journal logs (user service)..."
journalctl --user --rotate

echo "Vacuuming all journal logs..."
journalctl --user --vacuum-time=1s

echo "Journal cleared."
journalctl --user --disk-usage
