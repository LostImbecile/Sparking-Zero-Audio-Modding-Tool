#pragma once
#ifndef HCAKEY_GENERATOR_H
#define HCAKEY_GENERATOR_H

#include <stdio.h>
#include <stdint.h>
#include "utils.h"

#define MAX_FILENAME 256
#define MAX_KEY_LENGTH 33
#define MAX_ENTRIES 1000

typedef struct {
    char filename[MAX_FILENAME];
    char key[MAX_KEY_LENGTH];
} KeyEntry;

extern int csv_loaded;

int read_csv(const char* csv_filename);
void remove_suffix(char* filename);
const char* find_key(const char* filename);
void write_binary_key(const char* key, const char* folder);
char* get_directory(const char* path);
void generate_hcakey(const char* filepath);
void generate_hcakey_dir(const char* filepath, const char* directory);

#endif // HCAKEY_GENERATOR_H
