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
	if (was_folder_processed() && !app_data.config.Create_Separate_Mods) {
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
		printf("Usage (CMD): %s <file or folder paths>\nOR\n",
		       extract_name_from_path(argv[0]));
		printf("Drag and drop the following onto this EXE:\n");
		printf("1. Files such as .awb, .uasset, and .acb.\n");
		printf("2. Folders that the tool extracted, or folders with 'bgm' in their name for BGM.\n");
		printf("\nImportant: Ensure all required files are present for the tool's usage.\n");
		printf("\nGuides:\n");
		printf("- BGM: https://docs.google.com/document/d/1hjCoHq5XxsIRARTcqUn12roO_SVsuiYhDwmwWXCrDQ0/edit?tab=t.0\n");
		printf("- Voices & Sound Effects: https://docs.google.com/document/d/1hjCoHq5XxsIRARTcqUn12roO_SVsuiYhDwmwWXCrDQ0/edit?tab=t.qg24fpgvrtbx\n");
		printf("\nLatest Downloads:\n");
		printf("- https://gamebanana.com/tools/18312\n");
		printf("- https://github.com/Lostlmbecile/Sparking-Zero-Audio-Modding-Tool/releases/latest\n");
		printf("\nNote: if running in scripts, pass --cmd to avoid hangs.");
		printf("\nAbsolute paths to call the tool are preferred.");

		printf("\nPress Enter to exit...");
		getchar();
		return 1;
	}

	// Parse flags
	bool is_cmd_mode = false;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--cmd") == 0) {
			is_cmd_mode = true;
			break; // remove the break if you want to add more
		}
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
		fprintf(stderr,
		        "You have special characters in your path that my tool can't support!\n"\
		        "Move it to a folder with only English/ASCII characters.\n");
		pause_for_user(is_cmd_mode, "Press Enter to exit...");

		return 1;
	}

	// Initialise everything
	if (initialise_program(program_location) != 0) {
		pause_for_user(is_cmd_mode, "Initialisation failed. Press Enter to exit...");
		return 1;
	}

	// set global
	app_data.is_cmd_mode = is_cmd_mode;


	// Process arguments
	char** filtered_argv = preprocess_argv(&argc, argv);
	process_files(filtered_argv, argc);
	process_and_package_folders(filtered_argv, argc);

	// Clean up
	free_filtered_argv(filtered_argv);
	pause_for_user(app_data.is_cmd_mode,
	               "Processing complete. Press Enter to exit...");
	return 0;
}
