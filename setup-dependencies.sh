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

info "Dependencies installed successfully!"
info "You can now run: ./deploy.sh"

