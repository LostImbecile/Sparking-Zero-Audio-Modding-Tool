#pragma once
#ifndef HCA_TOOL_H
#define HCA_TOOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include "utils.h"

#define MAX_BGM_ENTRIES 500
#define MAX_ACB_MAPPINGS 100
#define MAX_HCA_PAIRS 100
#define MAX_BANNED_INDICES 250
#define HCA_HEADER_SIZE 128
#define HCA_MAX_SIZE 6144
#define UASSET_NAME "bgm_main.uasset"

typedef struct {
    char cueName[MAX_PATH];
    char targetFile[MAX_PATH];
    int index;
} BGMEntry;

typedef struct {
    int index;
    long offset;
    uint8_t header[HCA_HEADER_SIZE];
} HCAHeader;

typedef struct {
    char hca_path[MAX_PATH];
    char target_file[MAX_PATH];
    int index;
    uint8_t new_header[HCA_MAX_SIZE];
} InjectionInfo;

typedef struct {
    char awbName[MAX_PATH];
    char acbName[MAX_PATH];
    int tracks;
    int portNo;
} AcbMapping;

typedef struct {
    int index1;
    int index2;
} HcaPair;

typedef struct {
    int index1;
    int index2;
} BannedIndex;

// all data loaded from CSV files (global)
typedef struct {
    BGMEntry bgm_entries[MAX_BGM_ENTRIES];
    int bgm_entry_count;
    AcbMapping acb_mappings[MAX_ACB_MAPPINGS];
    int acb_mapping_count;
    HcaPair hca_pairs[MAX_HCA_PAIRS];
    int hca_pair_count;
    BannedIndex banned_indices[MAX_BANNED_INDICES];
    int banned_index_count;
} CsvData;

// Global constant
extern const uint8_t hca_signature[4];

extern CsvData csv_data;

// Function prototypes
int get_file_index_start(const char* awb_name);
bool read_bgm_dictionary(const char* filename);
bool read_acb_mapping(const char* filename);
bool read_hca_pairs(const char* filename);
bool read_banned_indices(const char* filename);

#endif // HCA_TOOL_H
