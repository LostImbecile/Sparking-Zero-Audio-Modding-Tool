#include "acb_mapping.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

AcbMappingData acb_data;

// trim leading/trailing whitespace
static void trim(char* str) {
    if (!str) return;
    char* start = str;
    while (isspace((unsigned char)*start)) {
        start++;
    }
    char* end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) {
        end--;
    }
    *(end + 1) = '\0';
    if (start != str) {
        memmove(str, start, end - start + 2);
    }
}

bool read_acb_mapping(const char* filepath, AcbMappingData* data) {
    FILE* file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open ACB mapping file: %s\n", filepath);
        data->mapping_count = 0;
        return false;
    }

    data->mapping_count = 0;
    char line[1024];

    // Skip header line
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return true;
    }

    while (data->mapping_count < MAX_ACB_MAPPINGS && fgets(line, sizeof(line), file)) {
        AcbMapping* current = &data->mappings[data->mapping_count];

        char* token = strtok(line, ",");
        if (!token) continue;
        trim(token);
        strncpy(current->awbName, token, MAX_PATH - 1);

        token = strtok(NULL, ",");
        if (!token) continue;
        trim(token);
        strncpy(current->acbName, token, MAX_PATH - 1);

        token = strtok(NULL, ",");
        if (!token) continue;
        current->tracks = atoi(token);

        token = strtok(NULL, ",");
        if (!token) continue;
        current->portNo = atoi(token);

        (data->mapping_count)++;
    }

    fclose(file);
    return true;
}
