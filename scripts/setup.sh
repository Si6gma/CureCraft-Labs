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

# Qt libraries - try Qt6, fallback to Qt5
info "Installing Qt libraries..."
sudo apt-get install -y \
    qt6-base-dev \
    libqt6gui6 \
    libqt6widgets6 \
    libqt6core6 \
    libqt6printsupport6 \
    qt6-tools-dev \
    libxkbcommon-dev 2>/dev/null || true

# Check if Qt6 is properly installed
if ! pkg-config --exists Qt6Widgets 2>/dev/null; then
    info "Qt6 not found, installing Qt5 as fallback..."
    sudo apt-get install -y \
        qt5-qmake \
        qtbase5-dev \
        libqt5widgets5 \
        libqt5printsupport5 \
        qtbase5-dev-tools
else
    info "Qt6 detected and ready!"
fi

# ============================================================================
# Create Command Aliases (Symlinks)
# ============================================================================
info "Setting up convenient command aliases..."

# Create ~/bin if it doesn't exist
mkdir -p "$HOME/bin"

# Create symlinks for quick access
SCRIPT_DIR="$HOME/Code/CureCraft-Labs/scripts"

ln -sf "${SCRIPT_DIR}/journal-read.sh" "$HOME/bin/jctl"
ln -sf "${SCRIPT_DIR}/journal-clear.sh" "$HOME/bin/jclr"
ln -sf "${SCRIPT_DIR}/status.sh" "$HOME/bin/status"
ln -sf "${SCRIPT_DIR}/deploy.sh" "$HOME/bin/upd"

# Make sure ~/bin is in PATH
if ! echo "$PATH" | grep -q "$HOME/bin"; then
    echo 'export PATH="$HOME/bin:$PATH"' >> "$HOME/.bashrc"
    info "Added ~/bin to PATH in .bashrc"
fi

info "Command aliases created:"
info "  jctl   - View live logs"
info "  jclr   - Clear journal logs"
info "  status - Check service status"
info "  upd    - Update and deploy"

info "Dependencies installed successfully!"
info "You can now run: ./deploy.sh"

# Reload PATH for current session
export PATH="$HOME/bin:$PATH"
