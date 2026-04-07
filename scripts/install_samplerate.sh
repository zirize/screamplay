#!/bin/bash
set -e

if [ -f /etc/debian_version ]; then
    sudo apt update
    sudo apt install -y libsamplerate0-dev pkg-config
elif [ -f /etc/arch-release ]; then
    sudo pacman -S --noconfirm libsamplerate pkg-config
else
    echo "Unsupported OS. Please install libsamplerate-dev manually."
    exit 1
fi