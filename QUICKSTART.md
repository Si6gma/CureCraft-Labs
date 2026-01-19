# CureCraft-Labs - Simplified Workflow

## Setup (One Time)

```bash
./setup-dependencies.sh     # Install Qt and build tools
./deploy.sh                 # Build and start service
```

## Update & Deploy (After Each Git Push)

```bash
./deploy.sh
```

That's it. It:

- Pulls latest code
- Rebuilds
- Restarts the service

## View Logs

```bash
journalctl -u curecraft.service -f
```

## Service Commands

```bash
sudo systemctl status curecraft.service    # Check status
sudo systemctl stop curecraft.service      # Stop
sudo systemctl start curecraft.service     # Start
sudo systemctl restart curecraft.service   # Restart
```

See BUILD_GUIDE.md for troubleshooting.
