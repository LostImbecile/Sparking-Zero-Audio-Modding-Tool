#pragma once
#ifndef FILE_EXTRACTOR_H
#define FILE_EXTRACTOR_H

#include <stdbool.h>
#include <stdlib.h>
#include "config.h"

#define MAX_PATH 1024

extern char acb_editor_path[];
extern Config config;

/**
 * @brief Finds the corresponding .acb file for a given input file
 * @param input_file Path to the input file
 * @param acb_path Buffer to store the found .acb file path
 * @param path_size Size of the acb_path buffer
 * @return true if .acb file is found, false otherwise
 */
bool find_acb_file(const char* input_file, char* acb_path, size_t path_size);

/**
 * @brief Runs the ACBEditor on the specified file
 * @param filepath Path to the file to process
 * @return 0 on success, non-zero on failure
 */
int run_acb_editor(const char* filepath);

/**
 * @brief Handles the complete extraction process
 * @param input_file Path to the input file
 * @return 0 on success, non-zero on failure
 */
int extract_and_process(const char* input_file);

#endif // FILE_EXTRACTOR_H
