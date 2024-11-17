#pragma once
#ifndef UASSET_INJECTOR_H
#define UASSET_INJECTOR_H

#include <stdio.h>
#include "utils.h"

#define BUFFER_SIZE 4096


// Function declarations for the injector
int inject_process_file(const char* input_path);
void create_backup(const char* file_path);
int inject_acb_content(const char* uasset_path, const char* acb_path);
long find_utf_marker_inject(FILE* fp);
void build_acb_path(const char* input_path, char* output_path, size_t max_len);
void build_uasset_path(const char* input_path, char* output_path, size_t max_len);

#endif // UASSET_INJECTOR_H
