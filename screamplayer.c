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
    char *host = "127.0.0.1";
    int port = 4010;

    while ((opt = getopt(argc, argv, "H:P:")) != -1) {
        switch (opt) {
            case 'H': host = optarg; break;
            case 'P': port = atoi(optarg); break;
            default:
                fprintf(stderr, "Usage: %s [-H host] [-P port] <wav_file>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Expected wav file argument\n");
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

    // Seek to data chunk (simplistic approach, assuming data chunk is next)
    char chunk_id[4];
    uint32_t chunk_size;
    while (fread(chunk_id, 1, 4, file) == 4 && fread(&chunk_size, 4, 1, file) == 1) {
        if (strncmp(chunk_id, "data", 4) == 0) {
            break;
        }
        fseek(file, chunk_size, SEEK_CUR);
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &dest_addr.sin_addr);

    uint8_t packet[1157]; // 5 byte header + up to 1152 bytes PCM payload
    uint8_t sample_rate_code = 0x00;
    if (header.sample_rate == 44100) sample_rate_code = 0x01;
    else if (header.sample_rate == 48000) sample_rate_code = 0x02;
    else sample_rate_code = 0x01; // default fallback

    packet[0] = sample_rate_code;
    packet[1] = header.bits_per_sample;
    packet[2] = header.num_channels;
    packet[3] = 0x03; // Stereo channel map (Front Left | Front Right)
    packet[4] = 0x00;

    int payload_max = 1152;
    size_t bytes_read;
    
    // Pacing calculation
    double bytes_per_sec = header.sample_rate * header.num_channels * (header.bits_per_sample / 8);

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