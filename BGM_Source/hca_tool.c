#include "hca_tool.h"
#include "awb.h"
#include "directory.h"
#include "utils.h"

const uint8_t hca_signature[] = {0xC8, 0xC3, 0xC1, 0x00};
bool read_bgm_dictionary(const char* filename) {
    char dict_path[MAX_PATH];
    snprintf(dict_path, sizeof(dict_path), "%sbgm_dictionary.csv", program_directory);

    FILE* file = fopen(dict_path, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open BGM dictionary file: %s\n", dict_path);
        return false;
    }

    bgm_entry_count = 0;
    char line[1024];

    // Skip header line if it exists
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return false;
    }

    while (bgm_entry_count < MAX_BGM_ENTRIES && fgets(line, sizeof(line), file)) {
        // Parse CSV line and populate bgm_entries
        char* token = strtok(line, ",");
        if (token) {
            strncpy(bgm_entries[bgm_entry_count].cueName, token, MAX_PATH - 1);

            token = strtok(NULL, ",");
            if (token) {
                strncpy(bgm_entries[bgm_entry_count].targetFile, token, MAX_PATH - 1);

                token = strtok(NULL, ",");
                if (token) {
                    bgm_entries[bgm_entry_count].index = atoi(token);
                    (bgm_entry_count)++;
                }
            }
        }
    }

    fclose(file);
    return true;
}

