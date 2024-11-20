#pragma once
#ifndef AUDIO_CONVERTER_H
#define AUDIO_CONVERTER_H

#include <stdint.h>
#include <inttypes.h>
#include "utils.h"

extern char vgaudio_cli_path[];
extern char vgmstream_path[];

// Extract HCA key from the .hcakey file in the given folder
uint64_t extract_hca_key(const char* folder);

// Process all WAV files in a folder
int process_wav_files(const char* folder, uint64_t hca_key, int set_looping_points);
int encrypt_hcas(const char* folder, uint64_t hcakey);

int convert_hca_to_wav(const char* hca_path, const char* output_path);
int process_hca_files(const char* folder);

#endif // AUDIO_CONVERTER_H
