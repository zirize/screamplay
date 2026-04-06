#!/bin/bash
set -e

echo "Setting up official Scream receiver for local debugging..."
mkdir -p vendor
cd vendor

if [ ! -d "scream" ]; then
    git clone https://github.com/duncanthrax/scream.git
fi

cd scream/Receivers/unix
mkdir -p build
cd build
cmake ..
make

echo "Scream receiver built successfully at vendor/scream/Receivers/unix/build/scream"
echo "To run locally: ./vendor/scream/Receivers/unix/build/scream -u -p 4010"