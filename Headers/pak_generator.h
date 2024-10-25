#ifndef PAK_GENERATOR_H
#define PAK_GENERATOR_H
#include "config.h"
#include "utils.h"
#define MAX_PATH 1024

extern char unrealpak_path[];
extern char program_directory[];
extern Config config;

// Generate PAK and folder structure for the given file
int pak_generate(const char* file_path, const char* mod_name);

#endif // PAK_GENERATOR_H
