#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "hca_tool.h"

int process_directory(const char* dirpatht);
bool check_and_process_hca(const char* filepath, const char* dirpath,
                           InjectionInfo* injections, int* injection_count);
bool is_index_in_pair(int index1, int index2);
bool process_hca_entry(const char* filepath, const char* dirpath,
                       InjectionInfo* injections, int* injection_count, int index);
void process_paired_hca_entries(const char* filepath, const char* dirpath,
                                InjectionInfo* injections, int* injection_count, int main_index);
#endif
