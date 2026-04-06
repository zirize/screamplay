/*
 * screamplay - A command-line utility for playing back audio files over Scream
 * Copyright (C) 2026 zirize
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sndfile.h>
#include <time.h>
#include <samplerate.h>

static uint8_t get_scream_rate_code(int rate) {
    if (rate % 44100 == 0) {
        int multiplier = rate / 44100;
        if (multiplier < 128) return (uint8_t)(multiplier | 0x80);
    }
    if (rate % 48000 == 0) {
        int multiplier = rate / 48000;
        if (multiplier < 128) return (uint8_t)multiplier;
    }
    return 0; // Unsupported
}

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
        fprintf(stderr, "Usage: screamplay -H <host> -P <port> <audio_file>\n");
        exit(EXIT_FAILURE);
    }

    SF_INFO sfinfo;
    memset(&sfinfo, 0, sizeof(sfinfo));
    SNDFILE *infile = sf_open(argv[optind], SFM_READ, &sfinfo);
    if (!infile) {
        fprintf(stderr, "Error opening file %s: %s\n", argv[optind], sf_strerror(NULL));
        exit(EXIT_FAILURE);
    }

    int target_rate = sfinfo.samplerate;
    uint8_t rate_code = get_scream_rate_code(sfinfo.samplerate);
    int needs_resampling = (rate_code == 0);

    if (needs_resampling) {
        target_rate = 48000;
        rate_code = get_scream_rate_code(target_rate);
    }

    // Networking Setup
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &dest_addr.sin_addr);

    uint8_t packet[1157];
    packet[0] = rate_code;
    packet[1] = 16; 
    packet[2] = sfinfo.channels;
    packet[3] = (sfinfo.channels == 1) ? 0x04 : 0x03;
    packet[4] = 0x00;

    const int payload_limit = 1152;
    const int samples_per_packet = payload_limit / (sizeof(short) * sfinfo.channels);
    
    // Buffer for reading/resampling
    float *input_buffer = malloc(samples_per_packet * sfinfo.channels * sizeof(float));
    float *output_buffer = malloc(samples_per_packet * 2 * sfinfo.channels * sizeof(float)); // Extra space for resampling
    short *short_buffer = malloc(samples_per_packet * 2 * sfinfo.channels * sizeof(short));

    SRC_STATE *src_state = NULL;
    if (needs_resampling) {
        int error;
        src_state = src_new(SRC_SINC_FASTEST, sfinfo.channels, &error);
    }

    struct timespec next_packet_time;
    clock_gettime(CLOCK_MONOTONIC, &next_packet_time);

    sf_count_t read_count;
    while ((read_count = sf_readf_float(infile, input_buffer, samples_per_packet)) > 0) {
        int frames_to_send;
        float *buffer_to_send;

        if (needs_resampling) {
            SRC_DATA src_data;
            src_data.data_in = input_buffer;
            src_data.input_frames = read_count;
            src_data.data_out = output_buffer;
            src_data.output_frames = samples_per_packet * 2;
            src_data.src_ratio = (double)target_rate / sfinfo.samplerate;
            src_data.end_of_input = 0;

            src_process(src_state, &src_data);
            frames_to_send = src_data.output_frames_gen;
            buffer_to_send = output_buffer;
        } else {
            frames_to_send = read_count;
            buffer_to_send = input_buffer;
        }

        // Convert to 16-bit PCM and send in chunks
        src_float_to_short_array(buffer_to_send, short_buffer, frames_to_send * sfinfo.channels);
        
        int samples_sent = 0;
        int total_samples = frames_to_send * sfinfo.channels;
        
        while (samples_sent < total_samples) {
            int to_send = total_samples - samples_sent;
            if (to_send > (payload_limit / sizeof(short))) to_send = payload_limit / sizeof(short);

            memcpy(packet + 5, short_buffer + samples_sent, to_send * sizeof(short));
            sendto(sockfd, packet, (to_send * sizeof(short)) + 5, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            
            // Pacing based on target rate
            long chunk_duration_ns = (long)((double)to_send / sfinfo.channels * 1000000000.0 / target_rate);
            next_packet_time.tv_nsec += chunk_duration_ns;
            while (next_packet_time.tv_nsec >= 1000000000) {
                next_packet_time.tv_sec++;
                next_packet_time.tv_nsec -= 1000000000;
            }
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_packet_time, NULL);
            
            samples_sent += to_send;
        }
    }

    if (src_state) src_delete(src_state);
    free(input_buffer);
    free(output_buffer);
    free(short_buffer);
    sf_close(infile);
    close(sockfd);
    return 0;
}