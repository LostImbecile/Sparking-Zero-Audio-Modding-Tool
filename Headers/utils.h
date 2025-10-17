#pragma once
#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>

#define MAX_PATH 1000

const char* extract_name_from_path(const char* path);
int create_directory_recursive(const char* path);
int create_directory(const char* path);
const char* get_file_extension(const char* filename);
int copy_file(const char* src_path, const char* dest_path);
bool is_directory(const char* path);
char* get_basename(const char* path);
const char* get_parent_directory(const char* path);
const char* replace_extension(const char* filename, const char* new_extension);
int remove_directory_recursive(const char* path);
int is_path_exists(const char *path);
const char* sanitize_path(const char* path);
void pause_for_user(bool is_cmd_mode, const char* message);
void clear_stdin_buffer(bool is_cmd_mode);


#endif // UTILS_H_INCLUDED
