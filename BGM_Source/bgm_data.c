#include "bgm_data.h"
#include "initialization.h" // Required for app_config
#include "awb.h"
#include "directory.h"
#include "utils.h"
#include <ctype.h>

const uint8_t hca_signature[] = {0xC8, 0xC3, 0xC1, 0x00};

CsvData csv_data;

int get_file_index_start(const char* awb_name) {
    int index_start = 0;
    bool found = false;

    for (int i = 0; i < csv_data.acb_mapping_count; i++) {
        if (strcasecmp(csv_data.acb_mappings[i].awbName, extract_name_from_path(awb_name)) == 0) {
            found = true;
            break;
        } else {
            index_start += csv_data.acb_mappings[i].tracks;
        }
    }

    if (!found) {
        return -1;
    }

    return index_start;
}

void trim(char* str) {
    if (str == NULL) return;
    char* start = str;
    while (*start && isspace((unsigned char)*start)) start++;
    char* end = str + strlen(str) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    if (start != str) memmove(str, start, end - start + 2);
}

bool read_bgm_dictionary(const char* filename) {
    char dict_path[MAX_PATH];
    get_program_file_path(filename, dict_path, sizeof(dict_path));
    trim(dict_path);

    FILE* file = fopen(dict_path, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open BGM dictionary file: %s\n", dict_path);
        return false;
    }

    csv_data.bgm_entry_count = 0;
    char line[1024];
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return false;
    }

    while (csv_data.bgm_entry_count < MAX_BGM_ENTRIES && fgets(line, sizeof(line), file)) {
        char* token = strtok(line, ",");
        if (token) {
            trim(token);
            strncpy(csv_data.bgm_entries[csv_data.bgm_entry_count].cueName, token, MAX_PATH - 1);
            csv_data.bgm_entries[csv_data.bgm_entry_count].cueName[MAX_PATH - 1] = '\0';

            token = strtok(NULL, ",");
            if (token) {
                trim(token);
                strncpy(csv_data.bgm_entries[csv_data.bgm_entry_count].targetFile, token, MAX_PATH - 1);
                csv_data.bgm_entries[csv_data.bgm_entry_count].targetFile[MAX_PATH - 1] = '\0';

                token = strtok(NULL, ",");
                if (token) {
                    csv_data.bgm_entries[csv_data.bgm_entry_count].index = atoi(token);
                    (csv_data.bgm_entry_count)++;
                }
            }
        }
    }
    fclose(file);
    return true;
}

bool read_acb_mapping(const char* filename) {
    char mapping_path[MAX_PATH];
    get_program_file_path(filename, mapping_path, sizeof(mapping_path));
    trim(mapping_path);

    FILE* file = fopen(mapping_path, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open ACB mapping file: %s\n", mapping_path);
        return false;
    }

    csv_data.acb_mapping_count = 0;
    char line[1024];
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return false;
    }

    while (csv_data.acb_mapping_count < MAX_ACB_MAPPINGS && fgets(line, sizeof(line), file)) {
        char* token = strtok(line, ",");
        if (token) {
            trim(token);
            strncpy(csv_data.acb_mappings[csv_data.acb_mapping_count].awbName, token, MAX_PATH - 1);
            csv_data.acb_mappings[csv_data.acb_mapping_count].awbName[MAX_PATH - 1] = '\0';

            token = strtok(NULL, ",");
            if (token) {
                trim(token);
                strncpy(csv_data.acb_mappings[csv_data.acb_mapping_count].acbName, token, MAX_PATH - 1);
                csv_data.acb_mappings[csv_data.acb_mapping_count].acbName[MAX_PATH - 1] = '\0';

                token = strtok(NULL, ",");
                if (token) {
                    csv_data.acb_mappings[csv_data.acb_mapping_count].tracks = atoi(token);

                    token = strtok(NULL, ",");
                    if (token) {
                        csv_data.acb_mappings[csv_data.acb_mapping_count].portNo = atoi(token);
                        (csv_data.acb_mapping_count)++;
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
    get_program_file_path(filename, pairs_path, sizeof(pairs_path));
    trim(pairs_path);

    FILE* file = fopen(pairs_path, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open HCA pairs file: %s\n", pairs_path);
        return false;
    }

    csv_data.hca_pair_count = 0;
    char line[1024];
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return false;
    }

    while (csv_data.hca_pair_count < MAX_HCA_PAIRS && fgets(line, sizeof(line), file)) {
        char* token = strtok(line, ",");
        if (token) {
            csv_data.hca_pairs[csv_data.hca_pair_count].index1 = atoi(token);
            token = strtok(NULL, ",");
            if (token) {
                csv_data.hca_pairs[csv_data.hca_pair_count].index2 = atoi(token);
                (csv_data.hca_pair_count)++;
            }
        }
    }
    fclose(file);
    return true;
}

bool read_banned_indices(const char* filename) {
    char path[MAX_PATH];
    get_program_file_path(filename, path, sizeof(path));
    trim(path);

    FILE* file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open banned index file: %s\n", path);
        return false;
    }

    csv_data.banned_index_count = 0;
    char line[1024];
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return true;
    }

    while (csv_data.banned_index_count < MAX_BANNED_INDICES && fgets(line, sizeof(line), file)) {
        char* token = strtok(line, ",");
        if (token) {
            csv_data.banned_indices[csv_data.banned_index_count].index1 = atoi(token);
            token = strtok(NULL, ",");
            if (token) {
                csv_data.banned_indices[csv_data.banned_index_count].index2 = atoi(token);
                (csv_data.banned_index_count)++;
            }
        }
    }
    fclose(file);
    return true;
}
