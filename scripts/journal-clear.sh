#!/usr/bin/env bash
set -euo pipefail

echo "Rotating journal logs..."
sudo journalctl --rotate

echo "Vacuuming all journal logs..."
sudo journalctl --vacuum-time=1s

echo "Journal cleared."
journalctl --disk-usage
