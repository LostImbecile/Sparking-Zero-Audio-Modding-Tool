#ifndef INJECT_H
#define INJECT_H

#include "hca_tool.h"
#include <unistd.h>
#define BUFFER_SIZE 8192

extern int fixed_size;
bool inject_hca(const char* uasset_path, InjectionInfo* injections, int injection_count);
bool create_backup(const char* filename);
bool find_and_replace_header(FILE* file, const uint8_t* old_header, const uint8_t* new_header);
long find_next_header_offset(FILE* file, long current_offset);
bool replace_header_at_offset(FILE* file, long offset, long next_offset,
                              const uint8_t* new_header, long new_header_size);

#endif
