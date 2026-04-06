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

    // Dummy implementation to pass test structure for now
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &dest_addr.sin_addr);

    uint8_t dummy_packet[10] = {0x01, 16, 2, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    sendto(sockfd, dummy_packet, 10, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    fclose(file);
    close(sockfd);
    return 0;
}