# screamplay

A command-line utility for playing back audio files over a network using the [Scream](https://github.com/duncanthrax/scream) virtual network sound card protocol. 

> **Notice:** This project was developed with the assistance of an AI agent (Gemini CLI) with minimal human intervention. Please review the code before production use.

## Features
- **Multi-format Support:** Uses `libsndfile` to decode WAV, FLAC, OGG, AIFF, and MP3 (if supported by the local libsndfile version).
- **Intelligent Resampling:** Uses `libsamplerate` to automatically convert non-standard sample rates (like 24kHz) to a receiver-friendly 48kHz.
- **High-Precision Pacing:** Utilizes `clock_nanosleep` to stream packets at the exact required bitrate without drift.

## Dependencies
- `libsndfile` (>= 1.0)
- `libsamplerate` (>= 0.1)

### Installation
**Debian/Ubuntu:**
```bash
sudo apt install libsndfile1-dev libsamplerate0-dev
```

**Arch Linux:**
```bash
sudo pacman -S libsndfile libsamplerate
```

## Build Instructions
```bash
make
```

## Usage
```bash
./screamplay -H <receiver_ip> -P <port> <audio_file>
```

**Example:**
```bash
./screamplay -H 127.0.0.1 -P 4010 music.mp3
```

## License
This project is licensed under the terms of the GNU Lesser General Public License v2.1. See the [LICENSE](LICENSE) file for details.

---
*Developed with the assistance of Gemini CLI.*