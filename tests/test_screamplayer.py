import socket
import subprocess
import time
import wave
import struct
import os

def create_dummy_wav(filename):
    with wave.open(filename, 'wb') as f:
        f.setnchannels(2)
        f.setsampwidth(2)
        f.setframerate(44100)
        # 0.1 seconds of silence
        f.writeframes(b'\x00' * (44100 * 2 * 2 // 10))

def test_screamplayer_udp_transmission():
    wav_file = "dummy.wav"
    create_dummy_wav(wav_file)
    
    # Start a dummy UDP receiver
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('127.0.0.1', 4010))
    sock.settimeout(2.0)
    
    # Run screamplay
    process = subprocess.Popen(['./screamplay', '-H', '127.0.0.1', '-P', '4010', wav_file])
    
    try:
        data, addr = sock.recvfrom(2048)
        assert len(data) > 5, "Packet too small"
        
        # Verify Scream 5-byte header
        sample_rate, sample_size, channels, channel_map = struct.unpack('<BBBH', data[:5])
        assert sample_rate == 0x01 # 44100 Hz
        assert sample_size == 16   # 16-bit
        assert channels == 2       # Stereo
        
    finally:
        process.kill()
        sock.close()
        os.remove(wav_file)