#!/usr/bin/env bash
set -euo pipefail

echo "Following logs for service: curecraft"
sudo journalctl -u curecraft -f