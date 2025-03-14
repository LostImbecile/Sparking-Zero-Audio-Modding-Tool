#include "file_mapping.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 300
#define MAPPING_FILENAME "file_mapping.csv"

FileMappingList* create_mapping_list() {
    FileMappingList* list = (FileMappingList*)malloc(sizeof(FileMappingList));
    if (!list) return NULL;

    list->mappings = (FileMapping*)malloc(INITIAL_CAPACITY * sizeof(FileMapping));
    if (!list->mappings) {
        free(list);
        return NULL;
    }

    list->count = 0;
    list->capacity = INITIAL_CAPACITY;
    return list;
}

int save_file_mapping(const char* folder_path, const FileMappingList* mapping) {
    char filepath[MAX_PATH];
    snprintf(filepath, sizeof(filepath), "%s\\%s", folder_path, MAPPING_FILENAME);

    FILE* file = fopen(filepath, "w");
    if (!file) return 0;

    fprintf(file, "Number,CueName\n"); // CSV header
    for (int i = 0; i < mapping->count; i++) {
        fprintf(file, "%d,\"%s\"\n", mapping->mappings[i].number, mapping->mappings[i].cue_name);
    }

    fclose(file);
    return 1;
}

FileMappingList* load_file_mapping(const char* folder_path) {
    char filepath[MAX_PATH];
    snprintf(filepath, sizeof(filepath), "%s\\%s", folder_path, MAPPING_FILENAME);

    FILE* file = fopen(filepath, "r");
    if (!file) return create_mapping_list();

    FileMappingList* list = create_mapping_list();
    if (!list) {
        fclose(file);
        return NULL;
    }

    char line[MAX_PATH * 2];
    // Skip header
    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file)) {
        int number;
        // Parse CSV line, handling quoted strings
        char* pos = strchr(line, ',');
        if (pos) {
            *pos = '\0';
            number = atoi(line);
            char* name_start = pos + 1;
            // Remove quotes if present
            if (*name_start == '"') {
                name_start++;
                char* name_end = strrchr(name_start, '"');
                if (name_end) *name_end = '\0';
            }
            // Remove newline if present
            char* newline = strchr(name_start, '\n');
            if (newline) *newline = '\0';

            add_file_mapping(list, number, name_start);
        }
    }

    fclose(file);
    return list;
}

void free_file_mapping(FileMappingList* mapping) {
    if (mapping) {
        free(mapping->mappings);
        free(mapping);
    }
}

int count_cue_name_occurrences(FileMappingList* mapping, const char* cue_name) {
    int count = 0;
    for (int i = 0; i < mapping->count; i++) {
        if (strcmp(mapping->mappings[i].cue_name, cue_name) == 0) {
            count++;
        }
    }
    return count;
}

char* generate_unique_cue_name(FileMappingList* mapping, const char* base_name, int number) {
    static char unique_name[MAX_PATH];

    // Special handling for null/empty names - always use numbered format
    if (!base_name || strlen(base_name) == 0 || strcmp(base_name, "null") == 0) {
        snprintf(unique_name, sizeof(unique_name), "Cue_%d - Null", number);
        return unique_name;
    }

    // For non-null names, first try without number
    snprintf(unique_name, sizeof(unique_name), "%s", base_name);

    // If this name already exists in the mapping, use numbered format
    if (count_cue_name_occurrences(mapping, unique_name) > 0) {
        snprintf(unique_name, sizeof(unique_name), "Cue_%d - %s", number, base_name);
    }

    return unique_name;
}

int add_file_mapping(FileMappingList* mapping, int number, const char* cue_name) {
    if (mapping->count >= mapping->capacity) {
        int new_capacity = mapping->capacity * 2;
        FileMapping* new_mappings = (FileMapping*)realloc(mapping->mappings,
                                                        new_capacity * sizeof(FileMapping));
        if (!new_mappings) return 0;

        mapping->mappings = new_mappings;
        mapping->capacity = new_capacity;
    }

    // Generate a unique name for this mapping
    const char* unique_name = generate_unique_cue_name(mapping, cue_name, number);

    mapping->mappings[mapping->count].number = number;
    strncpy(mapping->mappings[mapping->count].cue_name, unique_name, MAX_PATH - 1);
    mapping->mappings[mapping->count].cue_name[MAX_PATH - 1] = '\0';
    mapping->count++;

    return 1;
}

const char* get_cue_name_from_number(FileMappingList* mapping, int number) {
    for (int i = 0; i < mapping->count; i++) {
        if (mapping->mappings[i].number == number) {
            return mapping->mappings[i].cue_name;
        }
    }
    return NULL;
}

int get_number_from_cue_name(FileMappingList* mapping, const char* cue_name) {
    for (int i = 0; i < mapping->count; i++) {
        if (strcmp(mapping->mappings[i].cue_name, cue_name) == 0) {
            return mapping->mappings[i].number;
        }
    }
    return -1;
}
