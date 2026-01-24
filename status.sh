#!/usr/bin/env bash
# Quick status check for CureCraft service

echo "=== CureCraft Service Status ==="
systemctl --user status curecraft.service

echo ""
echo "=== Quick Commands ==="
echo "Start:   systemctl --user start curecraft.service"
echo "Stop:    systemctl --user stop curecraft.service"
echo "Restart: systemctl --user restart curecraft.service"
echo "Logs:    journalctl --user -u curecraft.service -f"
