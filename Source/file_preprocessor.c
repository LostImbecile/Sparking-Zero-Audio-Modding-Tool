#include "file_preprocessor.h"
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

	// Create entries and mark duplicates
	for (int i = 1; i < *argc && filtered_count < MAX_PATH - 1; i++) {
		// Skip flags
		if (strncmp(argv[i], "--", 2) == 0) {
			continue;
		}

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

		// Check for duplicates
		for (int j = 0; j < filtered_count; j++) {
			if (strcmp(entries[j].basename, entries[filtered_count].basename) == 0) {
				// Duplicate found
				if (entries[filtered_count].is_dir) {
					entries[filtered_count].keep = false;  // Remove duplicate directory
				} else if (entries[j].is_dir) {
					entries[j].keep = false; // Remove the previous directory entry
				} else {
					// Duplicate file, mark current one for removal (for now)
					entries[filtered_count].keep = false;
				}
				break;
			}
		}
		filtered_count++;
	}

	// Prioritise within duplicate groups
	for (int i = 0; i < filtered_count; i++) {
		if (!entries[i].keep) continue; // Skip already marked for removal

		for (int j = i + 1; j < filtered_count; j++) {
			if (strcmp(entries[i].basename, entries[j].basename) == 0
			        && !entries[j].keep) {
				// Found a duplicate group, prioritize .uasset
				bool is_uasset_i = strstr(entries[i].path, ".uasset") != NULL;
				bool is_uasset_j = strstr(entries[j].path, ".uasset") != NULL;

				if (is_uasset_j && !is_uasset_i) {
					entries[i].keep = false; // Remove non-uasset in favor of uasset
					entries[j].keep = true;
					i = j; // Start the outer loop from the new kept entry
					break;
				} else if (is_uasset_i && !is_uasset_j) {
					// Keep i, remove j (j is already marked as false)
					break;
				}
				// If both or neither are uasset, keep the first one (i)
			}
		}
	}

	// Filter
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
