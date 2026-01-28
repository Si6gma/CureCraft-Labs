# Scripts Directory

Utility scripts for CureCraft Patient Monitor deployment and management.

## Main Scripts

### [`setup.sh`](setup.sh)
**First-time setup on Raspberry Pi**
- Installs dependencies (cmake, ninja, i2c-tools, etc.)
- Configures ccache for faster builds
- Creates command aliases in `~/bin`

```bash
./scripts/setup.sh
```

### [`deploy.sh`](deploy.sh)
**Update and deploy the application**
- Fetches latest code from git
- Builds with CMake + Ninja
- Installs/restarts systemd service

```bash
./scripts/deploy.sh
# Or use alias: deploy
```

## Service Management

### [`setup-service.sh`](setup-service.sh)
**Install/reinstall systemd user service**
- Creates systemd user service file
- Enables auto-start on boot
- Restarts the service

```bash
./scripts/setup-service.sh
```

### [`start-curecraft.sh`](start-curecraft.sh)
**Service entry point**  
- Called by systemd service
- Launches the curecraft executable
- Do not run directly (use service instead)

### [`status.sh`](status.sh)
**Check service status**

```bash
./scripts/status.sh
# Or use alias: status
```

## Logging

### [`logs.sh`](logs.sh)
**View live service logs**

```bash
./scripts/logs.sh
# Or use alias: logs
```

### [`logs-clear.sh`](logs-clear.sh)
**Clear old journal logs**

```bash
./scripts/logs-clear.sh
# Or use alias: logs-clear
```

## Quick Command Aliases

After running `setup.sh`, these aliases are available:

| Alias | Script | Description |
|-------|--------|-------------|
| `deploy` | `deploy.sh` | Update and deploy |
| `status` | `status.sh` | Check service status |
| `logs` | `logs.sh` | View live logs |
| `logs-clear` | `logs-clear.sh` | Clear old logs |

## Workflow

**First time setup:**
```bash
./scripts/setup.sh
./scripts/deploy.sh
```

**Regular updates:**
```bash
deploy          # Fetch, build, restart
logs            # Watch logs
```

**Troubleshooting:**
```bash
status          # Check if running
logs            # See what's happening
logs-clear      # Clear old logs if disk is full
```
