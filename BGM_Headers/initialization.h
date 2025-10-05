#pragma once
#ifndef INITIALIZATION_H
#define INITIALIZATION_H

#include <stdlib.h>
#include "bgm_data.h" // MAX_PATH

// global variables
typedef struct {
	char program_directory[MAX_PATH];
} AppConfig;

extern AppConfig app_config;

int initialize_program(const char* program_path);
char* get_program_file_path(const char* filename, char* buffer, size_t buffer_size);

#endif // INITIALIZATION_H
