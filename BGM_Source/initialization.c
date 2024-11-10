#include "initialization.h"
#include "hca_tool.h"
#include <stdio.h>
#include <string.h>

#define MAX_PATH 1024

char program_directory[MAX_PATH] = {0};
BGMEntry bgm_entries[MAX_BGM_ENTRIES];
int bgm_entry_count = 0;


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

    if (!read_bgm_dictionary("bgm_dictionary.csv")) {  //No need for bgm_entries and count params
        fprintf(stderr, "Error loading BGM dictionary.\n");
        return 1; // Or handle the error as appropriate
    }


    return 0;
}
