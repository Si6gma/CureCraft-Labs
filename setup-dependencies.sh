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
    pkg-config

# Qt libraries - try Qt6, fallback to Qt5
info "Installing Qt libraries..."
sudo apt-get install -y \
    qt6-base-dev \
    libqt6gui6 \
    libqt6widgets6 \
    libqt6core6 \
    libqt6printsupport6 \
    qt6-tools-dev 2>/dev/null || true

# If qmake not available, fallback to Qt5
if ! qmake --version 2>/dev/null | grep -q "Qt"; then
    info "Qt6 not available, installing Qt5..."
    sudo apt-get install -y \
        qt5-qmake \
        qtbase5-dev \
        libqt5widgets5 \
        libqt5printsupport5 \
        libqt5printsupport5-dev
fi

info "Dependencies installed successfully!"
info "You can now run: ./deploy.sh"

