#pragma once
#ifndef FILE_PROCESSOR_H
#define FILE_PROCESSOR_H

#include <stdio.h>

int bgm_process_input(const char* input);
int bgm_process_directory(const char* dir_path);
int bgm_process_uasset_file(const char* file_path);
int bgm_process_awb_file(const char* file_path);

#endif
