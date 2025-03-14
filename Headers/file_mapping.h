// Add these new functions to file_mapping.h
#ifndef FILE_MAPPING_H
#define FILE_MAPPING_H

#include <stdint.h>
#include "utils.h"

typedef struct {
    int number;
    char cue_name[MAX_PATH];
} FileMapping;

typedef struct {
    FileMapping* mappings;
    int count;
    int capacity;
} FileMappingList;

int save_file_mapping(const char* folder_path, const FileMappingList* mapping);
FileMappingList* load_file_mapping(const char* folder_path);
void free_file_mapping(FileMappingList* mapping);
int add_file_mapping(FileMappingList* mapping, int number, const char* cue_name);
const char* get_cue_name_from_number(FileMappingList* mapping, int number);
int get_number_from_cue_name(FileMappingList* mapping, const char* cue_name);

int count_cue_name_occurrences(FileMappingList* mapping, const char* cue_name);
char* generate_unique_cue_name(FileMappingList* mapping, const char* base_name, int number);

#endif // FILE_MAPPING_H
