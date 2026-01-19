#!/usr/bin/env bash
set -euo pipefail

# CureCraft-Labs Dependency Setup Script
# Install build tools and Qt libraries

die() { echo "ERROR: $*" >&2; exit 1; }
info() { echo "INFO: $*"; }

# Detect OS
if [[ -f /etc/os-release ]]; then
    . /etc/os-release
    OS="$ID"
else
    die "Cannot detect OS"
fi

info "Detected OS: $OS"

case "$OS" in
    ubuntu|debian)
        info "Setting up for Debian/Ubuntu..."
        sudo apt-get update
        
        # Build tools
        info "Installing build tools..."
        sudo apt-get install -y \
            build-essential \
            cmake \
            ninja-build \
            git
        
        # Qt6 development libraries (recommended)
        info "Installing Qt6 development libraries..."
        sudo apt-get install -y \
            qt6-base-dev \
            qt6-base-private-dev \
            libqt6gui6 \
            libqt6widgets6 \
            libqt6core6 \
            libqt6printsupport6 \
            libqt6printsupport6-dev \
            qt6-tools-dev
        
        # Alternative: Qt5 (if Qt6 not available)
        # sudo apt-get install -y qt5-qmake qt5-default libqt5widgets5-dev libqt5printsupport5-dev
        
        info "Done! Qt6 should now be available."
        ;;
    
    fedora|rhel|centos)
        info "Setting up for Fedora/RHEL..."
        sudo dnf install -y \
            gcc \
            gcc-c++ \
            cmake \
            ninja-build \
            git \
            qt6-qtbase-devel \
            qt6-qttools-devel
        
        info "Done! Qt6 should now be available."
        ;;
    
    arch)
        info "Setting up for Arch Linux..."
        sudo pacman -S --noconfirm \
            base-devel \
            cmake \
            ninja \
            git \
            qt6-base
        
        info "Done! Qt6 should now be available."
        ;;
    
    *)
        die "Unsupported OS: $OS. Please manually install Qt6 development libraries."
        ;;
esac

info "All dependencies installed successfully!"
info "You can now run: ./deploy.sh"
