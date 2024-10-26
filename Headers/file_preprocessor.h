#pragma once
#ifndef FILE_PREPROCESSOR_H
#define FILE_PREPROCESSOR_H
#include <stdbool.h>
#define MAX_PATH 1024
#define MAX_PATH_LENGTH 260

typedef struct {
    char* path;       // Full path to the file
    char* basename;   // Base name without extension
    bool keep;        // Flag indicating if this entry should be kept
    bool is_dir;      // Flag indicating if this entry is a directory
} FileEntry;

/**
 * @brief Preprocesses command line arguments to filter duplicate base names
 *
 * This function processes the input argv array to ensure no two files with the
 * same base name but different extensions remain. When duplicates are found:
 * - .uasset files are preferred over other extensions
 * - If no .uasset exists, the first encountered file is kept
 * - Directories are automatically skipped
 *
 * @param argc Pointer to argument count, will be modified to reflect filtered count
 * @param argv Array of command line arguments
 * @return char** Newly allocated array of filtered arguments. Must be freed with free_filtered_argv()
 */
char** preprocess_argv(int* argc, char* argv[]);

/**
 * @brief Frees memory allocated by preprocess_argv
 *
 * @param filtered_argv The filtered argument array to free
 */
void free_filtered_argv(char** filtered_argv);

#endif // FILE_PREPROCESSOR_H
