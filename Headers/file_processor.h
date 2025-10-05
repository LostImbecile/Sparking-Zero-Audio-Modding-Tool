#pragma once
#ifndef FILE_PROCESSOR_H
#define FILE_PROCESSOR_H

#include "uasset_extractor.h"
#include "hcakey_generator.h"
#include "utils.h"

#define MAX_FILES 1000

#include "initialization.h"

int process_input(const char* input);
int process_directory(const char* dir_path);
int process_acb_file(const char* file_path);
int process_uasset_file(const char* file_path);
int process_awb_file(const char* file_path);
int check_pair_exists(const char* path, const char* extension);
int handle_uasset_directory(const char* dir_path);
int process_pak_file(const char* file_path);

#endif // FILE_PROCESSOR_H
