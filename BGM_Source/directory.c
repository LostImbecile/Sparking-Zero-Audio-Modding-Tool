#include "directory.h"
#include "inject.h"
#include "utils.h"
#include "awb.h"
bool check_and_process_hca(const char* filepath, const char* dirpath,
                           InjectionInfo* injections, int* injection_count) {
	const char* filename = get_basename(filepath);
	char* endptr;
	// In case file name is the index (1.hca, 00001.hca)
	int filename_value = strtol(filename, &endptr, 10);
	if (*endptr != '\0') {
		filename_value = -1;
	}

	// Find matching entry in BGM dictionary
	for (int i = 0; i < csv_data.bgm_entry_count; i++) {
		if (strcasecmp(filename, csv_data.bgm_entries[i].cueName) == 0 ||
		        filename_value == csv_data.bgm_entries[i].index) {

			// Check against the banned indices list
			for (int j = 0; j < csv_data.banned_index_count; j++) {
				bool is_banned = false;
				// A single index is banned if index2 is less than index1 (e.g., 67, -1)
				if (csv_data.banned_indices[j].index2 < csv_data.banned_indices[j].index1) {
					if (csv_data.bgm_entries[i].index == csv_data.banned_indices[j].index1) {
						is_banned = true;
					}
				}
				// Otherwise, check if the index falls within the banned range
				else {
					if (csv_data.bgm_entries[i].index >= csv_data.banned_indices[j].index1
					        && csv_data.bgm_entries[i].index <= csv_data.banned_indices[j].index2) {
						is_banned = true;
					}
				}

				if (is_banned) {
					printf("You are not allowed to change index %d as it's vital to the game (protected).\n",
					       csv_data.bgm_entries[i].index);
					return false;
				}
			}

			// Check if this index has already been processed (including pairs)
			for (int j = 0; j < *injection_count; j++) {
				if (injections[j].index == csv_data.bgm_entries[i].index ||
				        is_index_in_pair(injections[j].index, csv_data.bgm_entries[i].index)) {
					printf("Warning: Ignoring file '%s' as index %d (or its hca pair) has already been processed\n",
					       extract_name_from_path(filepath), csv_data.bgm_entries[i].index);
					return false;
				}
			}

			// Process the main index
			if (!process_hca_entry(filepath, dirpath, injections, injection_count,
			                       csv_data.bgm_entries[i].index)) {
				return false; // Error occurred during processing
			}

			// Process any paired indices
			process_paired_hca_entries(filepath, dirpath, injections, injection_count,
			                           csv_data.bgm_entries[i].index);

			return true; // Successfully processed main and any paired entries
		}
	}
	printf("\"%s\" has no matching index in bgm_dictionary.csv and will be ignored.\n",
	       get_basename(filepath));
	return false;
}

// Helper function to check if an index is part of an HCA pair
bool is_index_in_pair(int index1, int index2) {
	for (int i = 0; i < csv_data.hca_pair_count; i++) {
		if ((csv_data.hca_pairs[i].index1 == index1 && csv_data.hca_pairs[i].index2 == index2) ||
		        (csv_data.hca_pairs[i].index1 == index2 && csv_data.hca_pairs[i].index2 == index1)) {
			return true;
		}
	}
	return false;
}

// Helper function to process an individual HCA entry
bool process_hca_entry(const char* filepath, const char* dirpath,
                       InjectionInfo* injections, int* injection_count, int index) {
	// Find the corresponding bgm_entry for this index
	BGMEntry* bgm_entry = NULL;
	for (int i = 0; i < csv_data.bgm_entry_count; i++) {
		if (csv_data.bgm_entries[i].index == index) {
			bgm_entry = &csv_data.bgm_entries[i];
			break;
		}
	}

	if (bgm_entry == NULL) {
		printf("Error: Could not find BGM entry for index %d\n", index);
		return false;
	}

	// Check if target file exists in the input directory
	char target_path[MAX_PATH];
	snprintf(target_path, sizeof(target_path), "%s\\%s", dirpath,
	         bgm_entry->targetFile);

	if (stat(target_path, &(struct stat) {
	0
}) != 0) {
		printf("Warning: Target file '%s' not found for index %d\n",
		       extract_name_from_path(target_path),
		       index);
		return false;
	}

	// Read HCA header
	FILE* hca_file = fopen(filepath, "rb");
	if (!hca_file) {
		printf("Error: Could not open HCA file '%s'\n", filepath);
		return false;
	}

	memset(&injections[*injection_count], 0, sizeof(InjectionInfo));
	strncpy(injections[*injection_count].hca_path, filepath, MAX_PATH - 1);
	injections[*injection_count].hca_path[MAX_PATH - 1] = '\0';

	strncpy(injections[*injection_count].target_file, target_path, MAX_PATH - 1);
	injections[*injection_count].target_file[MAX_PATH - 1] = '\0';

	injections[*injection_count].index = index;

	memset(injections[*injection_count].new_header, 0, HCA_MAX_SIZE);

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

// Helper function to process paired HCA entries
void process_paired_hca_entries(const char* filepath, const char* dirpath,
                                InjectionInfo* injections, int* injection_count, int main_index) {
	for (int i = 0; i < csv_data.hca_pair_count; i++) {
		int paired_index = -1;
		if (csv_data.hca_pairs[i].index1 == main_index) {
			paired_index = csv_data.hca_pairs[i].index2;
		} else if (csv_data.hca_pairs[i].index2 == main_index) {
			paired_index = csv_data.hca_pairs[i].index1;
		}

		if (paired_index != -1) {
			// Check if the paired index has already been added to injections
			bool already_added = false;
			for (int j = 0; j < *injection_count; j++) {
				if (injections[j].index == paired_index) {
					already_added = true;
					break;
				}
			}
			if (!already_added) {
				process_hca_entry(filepath, dirpath, injections, injection_count,
				                  paired_index);
			}
		}
	}
}

const char* get_container_file(const char* target_file,
                               const char* dir_name) {
	static char container_path[MAX_PATH];

	// 1. Check ACB Mapping
	for (int i = 0; i < csv_data.acb_mapping_count; i++) {
		if (strcasecmp(csv_data.acb_mappings[i].awbName,
		               extract_name_from_path(target_file)) == 0) {
			// Try with the extension from the mapping first
			snprintf(container_path, sizeof(container_path), "%s\\%s", dir_name,
			         csv_data.acb_mappings[i].acbName);
			if (stat(container_path, &(struct stat) {
			0
		}) == 0) {
				return container_path;
			}

			// Try replacing with .uasset
			snprintf(container_path, sizeof(container_path), "%s\\%s", dir_name,
			         replace_extension(csv_data.acb_mappings[i].acbName, "uasset"));
			if (stat(container_path, &(struct stat) {
			0
		}) == 0) {
				return container_path;
			}

			// Try replacing with .acb
			snprintf(container_path, sizeof(container_path), "%s\\%s", dir_name,
			         replace_extension(csv_data.acb_mappings[i].acbName, "acb"));
			if (stat(container_path, &(struct stat) {
			0
		}) == 0) {
				return container_path;
			}

			fprintf(stderr,
			        "Warning: ACB mapping found for %s, but no corresponding .uasset or .acb file exists.\n",
			        target_file);
		}
	}

	// 2. If not found in ACB mapping, try target file name with .uasset and .acb extensions

	// Try .uasset
	snprintf(container_path, sizeof(container_path), "%s\\%s", dir_name,
	         replace_extension(target_file, "uasset"));
	if (stat(container_path, &(struct stat) {
	0
}) == 0) {
		return container_path;
	}

	// Try .acb
	snprintf(container_path, sizeof(container_path), "%s\\%s", dir_name,
	         replace_extension(target_file, "acb"));
	if (stat(container_path, &(struct stat) {
	0
}) == 0) {
		return container_path;
	}

	// 3. Not found
	return NULL;
}

int process_directory(const char* dirpath) {
	DIR* dir = opendir(dirpath);
	if (!dir) {
		fprintf(stderr, "Error: Could not open directory: %s\n", dirpath);
		return -1;
	}

	// Get the directory part of the input path for checking files
	char dir_name[MAX_PATH];
	strcpy(dir_name, get_parent_directory(dirpath));

	// Process HCA files
	struct dirent* entry;
	InjectionInfo* injections = malloc(sizeof(InjectionInfo) * MAX_BGM_ENTRIES);
	if (!injections) {
		fprintf(stderr, "Error: Could not allocate memory for injections\n");
		closedir(dir);
		return -1;
	}
	int injection_count = 0;

	while ((entry = readdir(dir)) != NULL) {
		const char* ext = get_file_extension(entry->d_name);

		if (strcasecmp(ext, "hca") == 0) {
			char filepath[MAX_PATH];
			snprintf(filepath, sizeof(filepath), "%s\\%s", dirpath, entry->d_name);

			check_and_process_hca(filepath, dir_name,
			                      injections, &injection_count);
		}
	}

	closedir(dir);

	// If we found any HCA files to inject, process them
	if (injection_count > 0) {
		// Create a map to store container file paths and a list for grouped injections
		char container_map[MAX_BGM_ENTRIES][MAX_PATH] = {0};
		int container_count = 0;

		// Use an array of pointers for grouped injections
		InjectionInfo* grouped_injections[MAX_BGM_ENTRIES];
		for (int i = 0; i < MAX_BGM_ENTRIES; i++) {
			grouped_injections[i] = NULL; // Initialize to NULL
		}

		for (int i = 0; i < injection_count; i++) {
			const char* container_file = get_container_file(injections[i].target_file,
			                             dir_name);
			if (container_file == NULL) {
				fprintf(stderr,
				        "Error: Could not find corresponding uasset/acb file for: %s\n",
				        injections[i].target_file);
				free(injections);
				return -1;
			}

			// Check if this container is already in the map
			int container_index = -1;
			for (int j = 0; j < container_count; j++) {
				if (strcasecmp(container_map[j], container_file) == 0) {
					container_index = j;
					break;
				}
			}

			// If not found, add it to the map and allocate memory for the group
			if (container_index == -1) {
				strcpy(container_map[container_count], container_file);
				grouped_injections[container_count] = malloc(sizeof(InjectionInfo) *
				                                      (injection_count + 1));
				if (!grouped_injections[container_count]) {
					fprintf(stderr, "Error: Could not allocate memory for grouped injections\n");
					// Free previously allocated memory
					for (int k = 0; k < container_count; k++) {
						free(grouped_injections[k]);
					}
					free(injections);
					return -1;
				}
				// Initialize the new group with -2 (end-of-group marker)
				for (int j = 0; j <= injection_count; j++) {
					grouped_injections[container_count][j].index = -2;
				}
				container_index = container_count;
				container_count++;
			}

			// Find the first empty slot in the group and add the injection
			int group_index = 0;
			while (grouped_injections[container_index][group_index].index != -2) {
				group_index++;
			}
			grouped_injections[container_index][group_index] = injections[i];
		}

		// Process injections for each container
		for (int i = 0; i < container_count; i++) {
			// Call process_uasset_file() for each container before injecting
			if (process_uasset_file(container_map[i]) != 0) {
				fprintf(stderr, "Failed to generate headers for uasset: %s\n",
				        container_map[i]);
				// Free allocated memory
				for (int j = 0; j < container_count; j++) {
					free(grouped_injections[j]);
				}
				free(injections);
				return -1;
			}

			// Count the number of injections for this container
			int num_injections = 0;
			while (grouped_injections[i][num_injections].index != -2) {
				num_injections++;
			}

			// Inject HCA files for the current container
			if (!inject_hca(container_map[i], grouped_injections[i], num_injections)) {
				fprintf(stderr, "Error: Failed to inject HCA files for container: %s\n",
				        container_map[i]);
				// Free allocated memory
				for (int j = 0; j < container_count; j++) {
					free(grouped_injections[j]);
				}
				free(injections);
				return -1;
			}
		}

		// Free allocated memory for grouped injections
		for (int i = 0; i < container_count; i++) {
			if (grouped_injections[i] != NULL) {
				free(grouped_injections[i]);
			}
		}
	}

	free(injections);
	return 0;
}
