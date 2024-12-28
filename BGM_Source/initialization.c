#include "initialization.h"
#include "hca_tool.h"
#include <stdio.h>
#include <string.h>

char program_directory[MAX_PATH] = {0};


char* get_program_file_path(const char* filename, char* buffer, size_t buffer_size) {
    snprintf(buffer, buffer_size, "%s%s", program_directory, filename);
    return buffer;
}

int initialize_program(const char* program_path) {
    // Set program directory
    strncpy(program_directory, program_path, MAX_PATH - 1);
    program_directory[MAX_PATH - 1] = '\0';

    char* last_backslash = strrchr(program_directory, '\\');
    if (last_backslash) {
        *(last_backslash + 1) = '\0';
    }

    if (!read_bgm_dictionary("bgm_dictionary.csv")) {
        fprintf(stderr, "Error loading BGM dictionary.\n");
        return 1;
    }

    if(!read_acb_mapping("acb_mapping.csv")){
        fprintf(stderr, "Error loading ACB mappings.\n");
        return 1;
    }
    if (!read_hca_pairs("hca_pairs.csv")) {
        fprintf(stderr, "Error loading HCA Pairs.\n");
        return 1;
    }

    return 0;
}
