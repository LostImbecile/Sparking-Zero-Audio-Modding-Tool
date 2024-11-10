#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "hca_tool.h"

int process_directory(const char* dirpatht);
bool check_and_process_hca(const char* filepath, const char* dirpath, InjectionInfo* injections, int* injection_count);


#endif
