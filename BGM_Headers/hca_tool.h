// hca_tool.h
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
#define HCA_HEADER_SIZE 128
#define HCA_MAX_SIZE 6144

#define UASSET_NAME "bgm_main.uasset"

extern char program_directory[MAX_PATH];
extern const uint8_t hca_signature[4];

// Structure to hold BGM dictionary entries
typedef struct {
    char cueName[MAX_PATH];
    char targetFile[MAX_PATH];
    int index;
} BGMEntry;

// Structure to hold HCA header information
typedef struct {
    int index;
    long offset;
    uint8_t header[HCA_HEADER_SIZE];
} HCAHeader;

// Structure to hold injection information
typedef struct {
    char hca_path[MAX_PATH];
    char target_file[MAX_PATH];
    int index;
    uint8_t new_header[HCA_MAX_SIZE];
} InjectionInfo;

// Structure to hold ACB mapping information
typedef struct {
    char awbName[MAX_PATH];
    char acbName[MAX_PATH];
    int tracks;
    int portNo;
} AcbMapping;

// Structure to hold HCA pairs information
typedef struct {
    int index1;
    int index2;
} HcaPair;

extern BGMEntry bgm_entries[MAX_BGM_ENTRIES];
extern int bgm_entry_count;
extern AcbMapping acb_mappings[MAX_ACB_MAPPINGS];
extern int acb_mapping_count;
extern HcaPair hca_pairs[MAX_HCA_PAIRS];
extern int hca_pair_count;

int get_file_index_start(const char* awb_name);
// CSV reading functions
bool read_bgm_dictionary(const char* filename);
bool read_acb_mapping(const char* filename);
bool read_hca_pairs(const char* filename);


#endif // HCA_TOOL_H
