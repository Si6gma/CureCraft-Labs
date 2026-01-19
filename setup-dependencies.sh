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
    ubuntu|debian|raspbian)
        info "Setting up for Debian/Ubuntu/Raspbian..."
        sudo apt-get update
        
        # Build tools
        info "Installing build tools..."
        sudo apt-get install -y \
            build-essential \
            cmake \
            ninja-build \
            git \
            pkg-config
        
        # Try Qt6 first with flexible package matching
        info "Attempting to install Qt6 development libraries..."
        
        # Install Qt6 packages that are likely available
        sudo apt-get install -y \
            qt6-base-dev \
            libqt6gui6 \
            libqt6widgets6 \
            libqt6core6 \
            libqt6printsupport6 \
            qt6-tools-dev 2>/dev/null || true
        
        # Check if qmake is available, if not fall back to Qt5
        if ! qmake --version 2>/dev/null | grep -q "Qt"; then
            info "Qt6 not fully available, installing Qt5 as fallback..."
            sudo apt-get install -y \
                qt5-qmake \
                libqt5core5a \
                libqt5gui5 \
                libqt5widgets5 \
                libqt5printsupport5 \
                libqt5printsupport5-dev \
                qtbase5-dev
        fi
        
        info "Done! Qt should now be available."
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
        die "Unsupported OS: $OS. Please manually install Qt development libraries."
        ;;
esac

info "All dependencies installed successfully!"
info "You can now run: ./deploy.sh"
