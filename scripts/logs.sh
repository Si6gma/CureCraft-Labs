#!/usr/bin/env bash
set -euo pipefail

echo "Following logs for service: curecraft (user service)"
journalctl --user -u curecraft.service -f