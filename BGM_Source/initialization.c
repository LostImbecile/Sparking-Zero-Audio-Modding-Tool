#include "initialization.h"
#include "bgm_data.h"
#include <stdio.h>
#include <string.h>

AppConfig app_config;

char* get_program_file_path(const char* filename, char* buffer, size_t buffer_size) {
    snprintf(buffer, buffer_size, "%s%s", app_config.program_directory, filename);
    return buffer;
}

int initialize_program(const char* program_path) {
    // Set program directory
    strncpy(app_config.program_directory, program_path, MAX_PATH - 1);
    app_config.program_directory[MAX_PATH - 1] = '\0';

    char* last_backslash = strrchr(app_config.program_directory, '\\');
    if (last_backslash) {
        *(last_backslash + 1) = '\0';
    }

    // Load data from CSV files
    if (!read_bgm_dictionary("bgm_dictionary.csv")) {
        fprintf(stderr, "Error loading BGM dictionary.\n");
        return 1;
    }
    if (!read_acb_mapping("acb_mapping.csv")) {
        fprintf(stderr, "Error loading ACB mappings.\n");
        return 1;
    }
    if (!read_hca_pairs("hca_pairs.csv")) {
        fprintf(stderr, "Error loading HCA Pairs.\n");
        return 1;
    }
    if (!read_banned_indices("protected_indices.csv")) {
        fprintf(stderr, "Error loading Banned Indices.\n");
        return 1;
    }

    return 0;
}
