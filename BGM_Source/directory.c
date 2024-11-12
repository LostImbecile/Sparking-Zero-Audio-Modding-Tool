#include "directory.h"
#include "inject.h"
#include "utils.h"
#include "awb.h"
bool check_and_process_hca(const char* filepath, const char* dirpath,
                           InjectionInfo* injections, int* injection_count) {
	const char* filename = get_basename(filepath);
	char *endptr;
	// In case file name is the index (1.hca, 00001.hca)
	int filename_value = strtol(filename, &endptr, 10);
	if (*endptr != '\0') {
		filename_value = -1;
	}

	// Find matching entry in BGM dictionary
	for (int i = 0; i < bgm_entry_count; i++) {
		if (strcasecmp(filename, bgm_entries[i].cueName) == 0
		        || filename_value == bgm_entries[i].index) {
			if (bgm_entries[i].index == 82 || bgm_entries[i].index == 83
			        || bgm_entries[i].index == 86 || bgm_entries[i].index < 6) {
				printf("You are not allowed to change index %d as it's vital to the game\n",
				       bgm_entries[i].index);
				       return false;
			}

			// Check if this index has already been processed
			for (int j = 0; j < *injection_count; j++) {
				if (injections[j].index == bgm_entries[i].index) {
					printf("Warning: Ignoring file '%s' as index %d has already been processed\n",
					       filepath, bgm_entries[i].index);
					return false;
				}
			}

			// Check if target file exists in the input directory
			char target_path[MAX_PATH];
			snprintf(target_path, sizeof(target_path), "%s\\%s",
			         dirpath, bgm_entries[i].targetFile);
			if (access(target_path, F_OK) != -1) {
				// Read HCA header
				FILE* hca_file = fopen(filepath, "rb");
				if (!hca_file) continue;

				memset(&injections[*injection_count], 0,
				       sizeof(InjectionInfo)); // Clears the current entry
				strncpy(injections[*injection_count].hca_path, filepath, MAX_PATH - 1);
				injections[*injection_count].hca_path[MAX_PATH - 1] =
				    '\0';  // Ensure null-termination

				strncpy(injections[*injection_count].target_file, target_path, MAX_PATH - 1);
				injections[*injection_count].target_file[MAX_PATH - 1] =
				    '\0';  // Ensure null-termination

				injections[*injection_count].index = bgm_entries[i].index;

				// Initialise new_header to zero to ensure no junk data
				// I know I did a previous memset but shrug, got to double down
				memset(injections[*injection_count].new_header, 0, HCA_MAX_SIZE);

				// Read header with check for full read
				long bytes_read = fread(injections[*injection_count].new_header, 1,
				                        HCA_MAX_SIZE, hca_file);
				if (bytes_read != HCA_MAX_SIZE) {
					printf("Error: Only read %ld bytes from '%s' instead of expected %d bytes.\n",
					       bytes_read, filepath, HCA_MAX_SIZE);
				}

				(*injection_count)++;
				fclose(hca_file);
				return true;
			}
		}
	}
	printf("\"%s\" has no matching index in bgm_dictionary.csv and will be ignored.\n",
				       get_basename(filepath));
	return false;
}

int process_directory(const char* dirpath) {
	DIR* dir = opendir(dirpath);
	if (!dir) {
		fprintf(stderr, "Error: Could not open directory: %s\n", dirpath);
		return  -1;
	}

	// Get the directory part of the input path for checking files
	char dir_name[MAX_PATH] = {0};
	strcpy(dir_name, get_parent_directory(dirpath));

	// First, check if bgm_main.uasset exists in the input directory
	char uasset_path[MAX_PATH];
	snprintf(uasset_path, sizeof(uasset_path), "%s\\%s", dir_name, UASSET_NAME);

	struct stat st;
	if (stat(uasset_path, &st) != 0) {
		fprintf(stderr, "Error: %s not found in directory\n", UASSET_NAME);
		closedir(dir);
		return  -1;
	}

	if (process_uasset_file(uasset_path) != 0) {
		fprintf(stderr, "Failed to generate headers for uasset.\n");
		closedir(dir);
		return  -1;
	}

	// Process HCA files
	struct dirent* entry;
	InjectionInfo* injections = malloc(sizeof(InjectionInfo) * MAX_BGM_ENTRIES);
	int injection_count = 0;

	while ((entry = readdir(dir)) != NULL) {
		const char* ext = get_file_extension(entry->d_name);

		if (strcasecmp(ext, "hca") == 0 ) {
			char filepath[MAX_PATH];
			snprintf(filepath, sizeof(filepath), "%s\\%s", dirpath, entry->d_name);

			check_and_process_hca(filepath, dir_name,
			                           injections, &injection_count);
		}
	}

	closedir(dir);

	// If we found any HCA files to inject, process them
	if (injection_count > 0) {
		if (!inject_hca(uasset_path, injections, injection_count)) {
			free(injections);
			return -1;
		}
	}

	free(injections);
	return 0;
}
