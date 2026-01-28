#!/usr/bin/env bash
set -euo pipefail

# CureCraft-Labs Dependency Setup Script for Raspberry Pi
# Install build tools and Qt libraries

die() { echo "ERROR: $*" >&2; exit 1; }
info() { echo "INFO: $*"; }

info "Installing build dependencies for Raspberry Pi..."

sudo apt-get update

# Build tools
info "Installing build tools..."
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    pkg-config \
    ccache

# Configure ccache for optimal performance
info "Configuring ccache..."
ccache --max-size=2G  # 2GB cache is sufficient for this project
ccache --set-config=compression=true
ccache --set-config=compression_level=6

# Hardware dependencies
info "Installing hardware dependencies..."
sudo apt-get install -y \
    libi2c-dev \
    i2c-tools

# ============================================================================
# Create Command Aliases (Symlinks)
# ============================================================================
info "Setting up convenient command aliases..."

# Create ~/bin if it doesn't exist
mkdir -p "$HOME/bin"

# Create symlinks for quick access
SCRIPT_DIR="$HOME/Code/CureCraft-Labs/scripts"

ln -sf "${SCRIPT_DIR}/logs.sh" "$HOME/bin/logs"
ln -sf "${SCRIPT_DIR}/logs-clear.sh" "$HOME/bin/logs-clear"
ln -sf "${SCRIPT_DIR}/status.sh" "$HOME/bin/status"
ln -sf "${SCRIPT_DIR}/deploy.sh" "$HOME/bin/deploy"

# Make sure ~/bin is in PATH
if ! echo "$PATH" | grep -q "$HOME/bin"; then
    echo 'export PATH="$HOME/bin:$PATH"' >> "$HOME/.bashrc"
    info "Added ~/bin to PATH in .bashrc"
fi

source "$HOME/.bashrc"

info "Command aliases created:"
info "  logs        - View live service logs"
info "  logs-clear  - Clear journal logs"
info "  status      - Check service status"
info "  deploy      - Update and deploy"

info "Dependencies installed successfully!"
info "You can now run: ./deploy.sh"

# Reload PATH for current session
export PATH="$HOME/bin:$PATH"
