#include "file_preprocessor.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char** preprocess_argv(int* argc, char* argv[]) {
    if (argc == NULL || argv == NULL || *argc < 1) {
        return NULL;
    }

    FileEntry* entries = calloc(MAX_PATH, sizeof(FileEntry));
    if (entries == NULL) {
        return NULL;
    }

    char** filtered_argv = calloc(MAX_PATH, sizeof(char*));
    if (filtered_argv == NULL) {
        free(entries);
        return NULL;
    }

    int filtered_count = 0;

    // First pass: create entries
    for (int i = 1; i < *argc && filtered_count < MAX_PATH - 1; i++) {
        entries[filtered_count].path = strdup(argv[i]);
        if (entries[filtered_count].path == NULL) {
            goto cleanup_and_exit;
        }

        entries[filtered_count].is_dir = is_directory(argv[i]);
        entries[filtered_count].basename = get_basename(argv[i]);
        if (entries[filtered_count].basename == NULL) {
            free(entries[filtered_count].path);
            goto cleanup_and_exit;
        }

        entries[filtered_count].keep = true;

        // Check for duplicates and apply rules
        for (int j = 0; j < filtered_count; j++) {
            if (strcmp(entries[j].basename, entries[filtered_count].basename) == 0) {
                // Current entry has same base name as a previous entry
                if (entries[filtered_count].is_dir) {
                    // Current entry is a directory
                    entries[filtered_count].keep = false;  // Always remove duplicate directory
                } else if (entries[j].is_dir) {
                    // Current entry is a file, previous is directory
                    entries[j].keep = false;  // Remove the directory
                } else {
                    // Both are files
                    bool is_uasset = strstr(argv[i], ".uasset") != NULL;
                    bool other_is_uasset = strstr(entries[j].path, ".uasset") != NULL;

                    if (is_uasset) {
                        entries[j].keep = false;
                        entries[filtered_count].keep = true;
                    } else if (!other_is_uasset) {
                        entries[filtered_count].keep = false;
                    }
                }
                break;
            }
        }
        filtered_count++;
    }

    // Second pass: create filtered argv
    int new_argc = 1;  // Keep argv[0] (program name)
    filtered_argv[0] = argv[0];

    for (int i = 0; i < filtered_count; i++) {
        if (entries[i].keep) {
            filtered_argv[new_argc] = entries[i].path;
            new_argc++;
        } else {
            free(entries[i].path);
        }
        free(entries[i].basename);
    }

    free(entries);
    *argc = new_argc;
    return filtered_argv;

cleanup_and_exit:
    // Clean up on error
    for (int i = 0; i < filtered_count; i++) {
        free(entries[i].path);
        free(entries[i].basename);
    }
    free(entries);
    free(filtered_argv);
    return NULL;
}

void free_filtered_argv(char** filtered_argv) {
    if (filtered_argv != NULL) {
        // Start from 1 to skip argv[0] which points to the original program name
        for (int i = 1; filtered_argv[i] != NULL; i++) {
            free(filtered_argv[i]);
        }
        free(filtered_argv);
    }
}
