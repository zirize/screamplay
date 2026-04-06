#!/bin/bash
set -e

# Usage: ./scripts/debug_capture.sh <port> <output_raw_file>
PORT=${1:-4010}
OUTPUT=${2:-"debug_dump.raw"}

RECEIVER="./vendor/scream/Receivers/unix/build/scream"

if [ ! -f "$RECEIVER" ]; then
    echo "Official Scream receiver not found. Running setup script first..."
    ./scripts/setup_scream_receiver.sh
fi

echo "Starting Scream raw capture on port $PORT..."
echo "Run screamplayer in another terminal to start streaming."
echo "The capture will output raw 16-bit PCM. Use 'aplay' to verify it."
echo "Example: aplay -f S16_LE -r 44100 -c 2 $OUTPUT"
echo "Press Ctrl+C when finished."

# Run the receiver in raw mode and dump to file
"$RECEIVER" -u -p "$PORT" -o raw > "$OUTPUT"