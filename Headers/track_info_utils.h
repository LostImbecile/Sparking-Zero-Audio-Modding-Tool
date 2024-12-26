// track_info_utils.h
#ifndef TRACK_INFO_UTILS_H
#define TRACK_INFO_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "utils.h"


typedef struct {
    char stream_name[100];
    char cue_id[100];
} StreamInfo;

typedef struct {
    StreamInfo* records;
    int num_records;
} StreamData;

extern char vgmstream_path[];

int generate_txtm(const char* inputfile);
int run_vgmstream(const char* inputfile, StreamData* data);
int parse_vgmstream_output(FILE* output_file, StreamData* data);

#endif // TRACK_INFO_UTILS_H
