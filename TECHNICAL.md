# Technical Documentation: Tests and Scripts

This document provides an overview of the testing infrastructure and utility scripts included in the `screamplay` repository.

## Scripts (`scripts/`)

The `scripts` directory contains automation tools for setting up the environment and debugging the audio stream.

### `install_libsndfile.sh` & `install_samplerate.sh`
These scripts automate the installation of the core audio processing dependencies on Debian/Ubuntu and Arch Linux systems.
- **Usage:** `./scripts/install_libsndfile.sh`

### `setup_scream_receiver.sh`
Downloads and builds the official [Scream Unix Receiver](https://github.com/duncanthrax/scream) locally in the `vendor/` directory. This is useful for end-to-end testing without affecting your system-wide installation.
- **Usage:** `./scripts/setup_scream_receiver.sh`

### `debug_capture.sh`
Runs a local Scream receiver in **raw mode** and dumps the incoming PCM data to a file. This is the primary tool for diagnosing audio distortion or protocol issues.
- **Usage:** `./scripts/debug_capture.sh <port> <output_file.raw>`
- **Analysis:** You can verify the captured raw file using `aplay`:
  `aplay -f S16_LE -r 44100 -c 2 output_file.raw`

---

## Tests (`tests/`)

The `tests` directory contains automated validation suites to ensure protocol compliance and prevent regression.

### `test_screamplayer.py`
A Python-based test suite using `pytest`. It performs the following actions:
1. Generates a temporary dummy WAV file.
2. Starts a mock UDP receiver to intercept packets.
3. Executes the `screamplay` binary.
4. Validates that the **Scream 5-byte header** is correctly formed (Sample Rate, Bit Depth, Channels).
5. Cleans up temporary resources.

### Running Tests
To run the automated tests, ensure you have `pytest` installed:
```bash
pip install pytest
pytest tests/ -v
```

---
*Note: These tools are intended for developers and contributors to maintain the technical integrity of the project.*
