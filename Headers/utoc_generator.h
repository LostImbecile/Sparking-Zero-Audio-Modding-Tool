#pragma once
#ifndef UTOC_GENERATOR_H
#define UTOC_GENERATOR_H
#include "config.h"
#include "utils.h"
#define MAX_PATH 1024

extern char unrealrezen_path[];
extern char program_directory[];
extern Config config;

// Generate UTOC and folder structure for the given file
int utoc_generate(const char* file_path, const char* mod_name);

// Create directory structure and copy files
int utoc_create_structure(const char* file_path, const char* mod_name);

// Generate utoc and related files, clean up after
int utoc_package_and_cleanup(const char* mod_name);

#endif // UTOC_GENERATOR_H
