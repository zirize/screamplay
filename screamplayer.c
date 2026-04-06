#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#pragma pack(push, 1)
typedef struct {
    char chunk_id[4];
    uint32_t chunk_size;
    char format[4];
    char subchunk1_id[4];
    uint32_t subchunk1_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
} WavHeader;
#pragma pack(pop)

int main(int argc, char *argv[]) {
    int opt;
    char *host = NULL;
    int port = 0;

    while ((opt = getopt(argc, argv, "H:P:")) != -1) {
        switch (opt) {
            case 'H': host = optarg; break;
            case 'P': port = atoi(optarg); break;
            default:
                fprintf(stderr, "Usage: %s -H host -P port <wav_file>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (!host || port == 0 || optind >= argc) {
        fprintf(stderr, "Error: -H <host>, -P <port>, and <wav_file> are mandatory.\n");
        fprintf(stderr, "Usage: %s -H <host> -P <port> <wav_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(argv[optind], "rb");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    WavHeader header;
    if (fread(&header, sizeof(WavHeader), 1, file) != 1) {
        fprintf(stderr, "Failed to read WAV header\n");
        exit(EXIT_FAILURE);
    }

    // Seek to data chunk correctly
    char chunk_id[4];
    uint32_t chunk_size;
    while (fread(chunk_id, 1, 4, file) == 4 && fread(&chunk_size, 4, 1, file) == 1) {
        if (strncmp(chunk_id, "data", 4) == 0) {
            break;
        }
        // Skip this chunk
        fseek(file, chunk_size, SEEK_CUR);
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &dest_addr.sin_addr);

    uint8_t packet[1157]; // 5 byte header + up to 1152 bytes PCM payload
    uint8_t sample_rate_code;
    switch (header.sample_rate) {
        case 24000:  sample_rate_code = 0x02; break; // Map to 48kHz for the header, pacing will be 24kHz
        case 44100:  sample_rate_code = 0x01; break;
        case 48000:  sample_rate_code = 0x02; break;
        case 88200:  sample_rate_code = 0x03; break;
        case 96000:  sample_rate_code = 0x04; break;
        case 176400: sample_rate_code = 0x05; break;
        case 192000: sample_rate_code = 0x06; break;
        case 352800: sample_rate_code = 0x07; break;
        case 384000: sample_rate_code = 0x08; break;
        default:
            fprintf(stderr, "Unsupported sample rate: %u Hz\n", header.sample_rate);
            exit(EXIT_FAILURE);
    }

    packet[0] = sample_rate_code;
    packet[1] = (uint8_t)header.bits_per_sample;
    packet[2] = (uint8_t)header.num_channels;
    
    // Set channel map based on number of channels
    if (header.num_channels == 1) {
        packet[3] = 0x04; // Mono (Front Center)
        packet[4] = 0x00;
    } else {
        packet[3] = 0x03; // Stereo (Front Left | Front Right)
        packet[4] = 0x00;
    }

    int payload_max = 1152;
    size_t bytes_read;
    
    // Pacing calculation
    // Bytes per second: SampleRate * NumChannels * (BitDepth / 8)
    double bytes_per_sec = (double)header.sample_rate * header.num_channels * (header.bits_per_sample / 8);
    if (bytes_per_sec <= 0) {
        fprintf(stderr, "Invalid audio format parameters\n");
        exit(EXIT_FAILURE);
    }

    while ((bytes_read = fread(packet + 5, 1, payload_max, file)) > 0) {
        sendto(sockfd, packet, bytes_read + 5, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        
        // Pacing: Sleep to match audio rate
        double sleep_sec = (double)bytes_read / bytes_per_sec;
        usleep((useconds_t)(sleep_sec * 1000000.0));
    }

    fclose(file);
    close(sockfd);
    return 0;
}