# screamplay

A command-line utility for playing back audio files over a network using the [Scream](https://github.com/duncanthrax/scream) virtual network sound card protocol.

> **Notice:** This project was developed with the assistance of an AI agent (Gemini CLI) with minimal human intervention. Please review the code before production use.

## Features

- **Multi-format Support:** Uses `libsndfile` to decode WAV, FLAC, OGG, AIFF, and MP3 (if supported by the local libsndfile version).
- **Intelligent Resampling:** Uses `libsamplerate` to automatically convert non-standard sample rates (like 24kHz) to a receiver-friendly 48kHz.
- **High-Precision Pacing:** Utilizes `clock_nanosleep` to stream packets at the exact required bitrate without drift.

## Clone

```bash
git clone https://github.com/zirize/screamplay.git
cd screamplay
```

## Dependencies

- `libsndfile` (>= 1.0)
- `libsamplerate` (>= 0.1)

**Debian/Ubuntu:**
```bash
sudo apt update
sudo apt install build-essential pkg-config libsndfile1-dev libsamplerate0-dev
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc make pkgconf-pkg-config libsndfile-devel libsamplerate-devel
```

**Arch Linux:**
```bash
sudo pacman -S base-devel pkgconf libsndfile libsamplerate
```

## Build

```bash
make
```

The `screamplay` binary will be created in the current directory.

### Optional: system-wide install

```bash
sudo install -m755 screamplay /usr/local/bin/
```

## Usage

```bash
./screamplay [options] <audio_file>
```

### Options

| Flag | Long form | Description |
|------|-----------|-------------|
| `-H <host>` | `--host <host>` | Target receiver IP address (default: `239.255.77.77`) |
| `-P <port>` | `--port <port>` | Target UDP port (default: `4010`) |
| `-v` | `--verbose` | Print file info and resampling details |
| `-h` | `--help` | Show help and exit |

### Examples

```bash
# Default Scream multicast broadcast (no -H/-P needed)
./screamplay music.flac

# Specific host and port
./screamplay -H 192.168.1.100 -P 4010 music.flac

# Multicast with explicit address
./screamplay -H 239.255.77.77 -P 4010 music.wav

# Verbose output (shows sample rate, channels, resampling info)
./screamplay -v -H 192.168.1.100 -P 4010 music.ogg

# Loopback test (receiver on the same machine)
./screamplay -H 127.0.0.1 -P 4010 music.mp3

# Verbose output
./screamplay -v music.ogg
```

## Receivers

Any standard Scream receiver can be used to play the incoming audio stream:

- **Linux:** [Scream Unix receiver](https://github.com/duncanthrax/scream) — build and run `./scream -o alsa` or `./scream -o pulse`
- **Windows:** ScreamReader (included in the [Scream](https://github.com/duncanthrax/scream) Windows package)

The `scripts/setup_scream_receiver.sh` helper script can automatically download and build the Unix receiver locally (see [Scripts](#scripts) below).

## Scripts

The `scripts/` directory contains helper utilities:

| Script | Description |
|--------|-------------|
| `install_libsndfile.sh` | Installs `libsndfile` on Debian/Ubuntu or Arch Linux |
| `install_samplerate.sh` | Installs `libsamplerate` on Debian/Ubuntu or Arch Linux |
| `setup_scream_receiver.sh` | Downloads and builds the official Scream Unix receiver into `vendor/` for local testing |
| `debug_capture.sh` | Runs a local Scream receiver in raw mode and dumps PCM data to a file for analysis |

**Debug capture example:**
```bash
# Capture incoming stream to a raw PCM file
./scripts/debug_capture.sh 4010 output.raw

# Verify with aplay
aplay -f S16_LE -r 44100 -c 2 output.raw
```

## Testing

Requires Python and `pytest`:

```bash
pip install pytest
pytest tests/ -v
```

The test suite generates a dummy WAV file, starts a mock UDP receiver, runs `screamplay`, and validates that the Scream protocol header (sample rate, bit depth, channels) is correctly formed.

## Troubleshooting

**No audio on the receiver side**
- Confirm the receiver is listening on the correct IP and port
- Check firewall rules: `sudo ufw allow 4010/udp` (or equivalent)
- Verify the network route: `ping <receiver_ip>`

**`pkg-config` error during `make`**
- Make sure the `-dev` / `-devel` packages for `libsndfile` and `libsamplerate` are installed

**Unsupported sample rate**
- `screamplay` automatically resamples to 48kHz when the source rate is not supported by the Scream protocol; use `-v` to confirm

## Contributing

Contributions and bug reports are welcome. Please open an issue or pull request on [GitHub](https://github.com/zirize/screamplay).

## License

This project is licensed under the GNU Lesser General Public License v2.1. See the [LICENSE](LICENSE) file for details.

---
*Developed with the assistance of Gemini CLI.*
