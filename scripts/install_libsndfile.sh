#!/bin/bash
set -e

if [ -f /etc/debian_version ]; then
    sudo apt update
    sudo apt install -y libsndfile1-dev pkg-config
elif [ -f /etc/arch-release ]; then
    sudo pacman -S --noconfirm libsndfile pkg-config
else
    echo "Unsupported OS. Please install libsndfile-dev manually."
    exit 1
fi