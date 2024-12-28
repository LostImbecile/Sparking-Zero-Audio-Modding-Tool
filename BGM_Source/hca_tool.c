#include "hca_tool.h"
#include "awb.h"
#include "directory.h"
#include "utils.h"
#include <ctype.h>

const uint8_t hca_signature[] = {0xC8, 0xC3, 0xC1, 0x00};
BGMEntry bgm_entries[MAX_BGM_ENTRIES];
int bgm_entry_count = 0;
AcbMapping acb_mappings[MAX_ACB_MAPPINGS];
int acb_mapping_count;
HcaPair hca_pairs[MAX_HCA_PAIRS];
int hca_pair_count;

int get_file_index_start(const char* awb_name) {
    int index_start = 0;
    bool found = false;

    for (int i = 0; i < acb_mapping_count; i++) {
        if (strcasecmp(acb_mappings[i].awbName, extract_name_from_path(awb_name)) == 0) {
            found = true;
            break;
        } else {
            index_start += acb_mappings[i].tracks;
        }
    }

    if (!found) {
        return -1;
    }

    return index_start;
}

// Function to trim leading and trailing spaces from a string
void trim(char* str) {
    if (str == NULL) {
        return;
    }

    // Trim leading spaces
    char* start = str;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    // Trim trailing spaces
    char* end = str + strlen(str) - 1;
    while (end > start && isspace((unsigned char)*end)) {
        end--;
    }

    // Null-terminate the trimmed string
    *(end + 1) = '\0';

    // Shift characters to the beginning if needed
    if (start != str) {
        memmove(str, start, end - start + 2); // +2 to include null terminator
    }
}

bool read_bgm_dictionary(const char* filename) {
    char dict_path[MAX_PATH];
    snprintf(dict_path, sizeof(dict_path), "%s%s", program_directory, filename);
    trim(dict_path);

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
            trim(token);
            strncpy(bgm_entries[bgm_entry_count].cueName, token, MAX_PATH - 1);
            bgm_entries[bgm_entry_count].cueName[MAX_PATH - 1] = '\0'; // Ensure null-termination

            token = strtok(NULL, ",");
            if (token) {
                trim(token);
                strncpy(bgm_entries[bgm_entry_count].targetFile, token, MAX_PATH - 1);
                bgm_entries[bgm_entry_count].targetFile[MAX_PATH - 1] = '\0'; // Ensure null-termination

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

bool read_acb_mapping(const char* filename) {
    char mapping_path[MAX_PATH];
    snprintf(mapping_path, sizeof(mapping_path), "%s%s", program_directory, filename);
    trim(mapping_path);

    FILE* file = fopen(mapping_path, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open ACB mapping file: %s\n", mapping_path);
        return false;
    }

    acb_mapping_count = 0;
    char line[1024];

    // Skip header line
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return false;
    }

    while (acb_mapping_count < MAX_ACB_MAPPINGS && fgets(line, sizeof(line), file)) {
        char* token = strtok(line, ",");
        if (token) {
            trim(token);
            strncpy(acb_mappings[acb_mapping_count].awbName, token, MAX_PATH - 1);
            acb_mappings[acb_mapping_count].awbName[MAX_PATH - 1] = '\0';

            token = strtok(NULL, ",");
            if (token) {
                trim(token);
                strncpy(acb_mappings[acb_mapping_count].acbName, token, MAX_PATH - 1);
                acb_mappings[acb_mapping_count].acbName[MAX_PATH - 1] = '\0';

                token = strtok(NULL, ",");
                if (token) {
                    acb_mappings[acb_mapping_count].tracks = atoi(token);

                    token = strtok(NULL, ",");
                    if(token) {
                        acb_mappings[acb_mapping_count].portNo = atoi(token);
                        (acb_mapping_count)++;
                    }
                }
            }
        }
    }

    fclose(file);
    return true;
}

bool read_hca_pairs(const char* filename) {
    char pairs_path[MAX_PATH];
    snprintf(pairs_path, sizeof(pairs_path), "%s%s", program_directory, filename);
    trim(pairs_path);

    FILE* file = fopen(pairs_path, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open HCA pairs file: %s\n", pairs_path);
        return false;
    }

    hca_pair_count = 0;
    char line[1024];

    // Skip header line
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return false;
    }

    while (hca_pair_count < MAX_HCA_PAIRS && fgets(line, sizeof(line), file)) {
        char* token = strtok(line, ",");
        if (token) {
            hca_pairs[hca_pair_count].index1 = atoi(token);

            token = strtok(NULL, ",");
            if (token) {
                hca_pairs[hca_pair_count].index2 = atoi(token);
                (hca_pair_count)++;
            }
        }
    }

    fclose(file);
    return true;
}
