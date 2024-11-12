#include "initialization.h"
#include "file_processor.h"
#include "file_preprocessor.h"
#include "file_packer.h"
#include "utils.h"
#include <stdio.h>

extern Config config;

int process_and_package_folders(char** filtered_argv, int argc) {
	// First process all folders
	for (int i = 1; i < argc; i++) {
		if (is_directory(filtered_argv[i])) {
			if (process_input(filtered_argv[i]) != 0) {
				printf("Error processing folder: %s\n",
				       extract_name_from_path(filtered_argv[i]));
			}
			printf("\n");
		}
	}

	// If folders were processed and we're creating a combined mod, handle packaging
	if (was_folder_processed() && !config.Create_Separate_Mods) {
		// Package all processed folders into one mod
		const char* mod_name = get_mod_name();
		if (package_combined_mod(mod_name) != 0) {
			printf("Error creating combined mod package\n");
			return -1;
		}
	}
	return 0;
}

int process_files(char** filtered_argv, int argc) {
	// Process individual files
	for (int i = 1; i < argc; i++) {
		if (!is_directory(filtered_argv[i])) {
			if (process_input(filtered_argv[i]) != 0) {
				printf("Error processing file: %s\n",
				       extract_name_from_path(filtered_argv[i]));
			}
			printf("\n");
		}
	}
	return 0;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Usage: %s <file or folder paths>\n", extract_name_from_path(argv[0]));
		printf("Press Enter to exit...");
		getchar();
		return 1;
	}

	char program_location[MAX_PATH] = {0};
	if (strstr(argv[0], ":") == NULL) {
		if (getcwd(program_location, MAX_PATH) != NULL) {
			// Add trailing slash if not present
			size_t len = strlen(program_location);
			if (len > 0 && program_location[len - 1] != '\\') {
				strcat(program_location, "\\");
			}
		} else {
			strcpy(program_location, ".\\");
			strcat(program_location, argv[0]);
		}
	} else
		strcpy(program_location, argv[0]);

	strcpy(program_location, sanitize_path(program_location));

	if (!is_path_exists(program_location)) {
		fprintf(stderr, "You have special characters in your path!\n"\
		        "My tool does not support it\n"\
		        "Move it to a folder with only English/ASCII characters.\n");
		printf("Press Enter to exit...");
		getchar();

		return 1;
	}

	// Initialise everything
	if (initialise_program(program_location) != 0) {
		printf("Initialization failed. Press Enter to exit...");
		getchar();
		return 1;
	}

	// Process arguments
	char** filtered_argv = preprocess_argv(&argc, argv);
	process_files(filtered_argv, argc);
	process_and_package_folders(filtered_argv, argc);

	// Clean up
	free_filtered_argv(filtered_argv);
	printf("Processing complete. Press Enter to exit...");
	getchar();
	return 0;
}
