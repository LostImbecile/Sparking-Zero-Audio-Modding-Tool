#ifndef BGM_PROCESSOR_H
#define BGM_PROCESSOR_H

#include <stdbool.h>
#include <stdint.h>
#include "utils.h"
#include "initialization.h"

int process_bgm_input(const char* input);
int process_bgm_directory(const char* dir_path);
int process_bgm_awb_file(const char* file_path);
int pack_bgm_files(const char* foldername);
int extract_bgm_awb(const char* file_path);

#endif
