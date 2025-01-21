#ifndef HCAKEY_GENERATOR_H
#define HCAKEY_GENERATOR_H

#include <stdint.h>
#include "utils.h"

// Core interface
void generate_hcakey(const char* filepath);
void generate_hcakey_dir(const char* filepath, const char* directory);

// Key extraction (from AFS2 extractor)
uint64_t get_key(const char* filepath);

#endif // HCAKEY_GENERATOR_H
