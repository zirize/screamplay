#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sndfile.h>
#include <time.h>

int main(int argc, char *argv[]) {
    int opt;
    char *host = NULL;
    int port = 0;

    while ((opt = getopt(argc, argv, "H:P:")) != -1) {
        switch (opt) {
            case 'H': host = optarg; break;
            case 'P': port = atoi(optarg); break;
            default: exit(EXIT_FAILURE);
        }
    }

    if (!host || port == 0 || optind >= argc) {
        fprintf(stderr, "Error: -H <host>, -P <port>, and <audio_file> are mandatory.\n");
        fprintf(stderr, "Usage: %s -H <host> -P <port> <audio_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    SF_INFO sfinfo;
    memset(&sfinfo, 0, sizeof(sfinfo));
    SNDFILE *infile = sf_open(argv[optind], SFM_READ, &sfinfo);
    if (!infile) {
        fprintf(stderr, "Error opening file %s: %s\n", argv[optind], sf_strerror(NULL));
        exit(EXIT_FAILURE);
    }

    printf("Opening file: %s\n", argv[optind]);
    printf("Sample Rate: %d Hz\n", sfinfo.samplerate);
    printf("Channels:    %d\n", sfinfo.channels);
    printf("Format:      0x%08x\n", sfinfo.format);

    // Scream Protocol Setup
    uint8_t sample_rate_code;
    switch (sfinfo.samplerate) {
        case 44100:  sample_rate_code = 0x01; break;
        case 48000:  sample_rate_code = 0x02; break;
        case 88200:  sample_rate_code = 0x03; break;
        case 96000:  sample_rate_code = 0x04; break;
        case 176400: sample_rate_code = 0x05; break;
        case 192000: sample_rate_code = 0x06; break;
        case 352800: sample_rate_code = 0x07; break;
        case 384000: sample_rate_code = 0x08; break;
        case 24000:  sample_rate_code = 0x02; break; // Map to 48kHz for the header
        default:     sample_rate_code = 0x02; break; 
    }

    // Networking Setup
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &dest_addr.sin_addr);

    uint8_t packet[1157];
    packet[0] = sample_rate_code;
    packet[1] = 16; 
    packet[2] = sfinfo.channels;
    packet[3] = (sfinfo.channels == 1) ? 0x04 : 0x03;
    packet[4] = 0x00;

    int frames_per_packet = 288; // Always send 1152 bytes payload (288 frames stereo 16-bit)
    short *buffer = malloc(frames_per_packet * sfinfo.channels * sizeof(short));
    sf_count_t read_count;
    
    // Precise pacing: duration of one packet in nanoseconds
    long packet_duration_ns = (long)((double)frames_per_packet * 1000000000.0 / sfinfo.samplerate);

    struct timespec next_packet_time;
    clock_gettime(CLOCK_MONOTONIC, &next_packet_time);

    while ((read_count = sf_readf_short(infile, buffer, frames_per_packet)) > 0) {
        size_t payload_size = read_count * sfinfo.channels * sizeof(short);
        memcpy(packet + 5, buffer, payload_size);
        sendto(sockfd, packet, payload_size + 5, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        
        // Pacing: Calculate next absolute time to send
        next_packet_time.tv_nsec += packet_duration_ns;
        while (next_packet_time.tv_nsec >= 1000000000) {
            next_packet_time.tv_sec++;
            next_packet_time.tv_nsec -= 1000000000;
        }
        
        // Sleep until the exact target time to prevent drift
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_packet_time, NULL);
    }

    free(buffer);
    sf_close(infile);
    close(sockfd);
    return 0;
}