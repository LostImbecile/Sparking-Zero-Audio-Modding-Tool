#include "file_extractor.h"
#include "audio_converter.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool find_acb_file(const char* input_file, char* acb_path, size_t path_size) {
	const char* dot = strrchr(input_file, '.');
	if (!dot) {
		printf("Error: Input file has no extension\n");
		return false;
	}

	// Get the base name without extension
	size_t base_len = dot - input_file;
	if (base_len >= path_size - 5) { // 5 = length of ".awb\0"
		printf("Error: Path buffer too small\n");
		return false;
	}

	// Construct the .acb filename
	strncpy(acb_path, input_file, base_len);
	acb_path[base_len] = '\0';
	strcat(acb_path, ".acb");

	// Check if the file exists
	FILE* file = fopen(acb_path, "r");
	if (!file) {
		printf("Error: Could not find .acb file: %s\n",
		       extract_name_from_path(acb_path));
		return false;
	}
	fclose(file);
	return true;
}

int run_acb_editor(const char* filepath) {
	char command[MAX_PATH * 2];

	// Construct the command with proper quoting
	snprintf(command, sizeof(command), "\"\"%s\" \"%s\"\"", acb_editor_path,
	         filepath);

	// Execute the command
	int result = system(command);
	printf("\n");
	if (result != 0) {
		printf("Error: ACBEditor failed with return code %d\n", result);
		return 1;
	}

	return 0;
}

int extract_and_process(const char* input_file) {
	char acb_path[MAX_PATH];

	// Find the corresponding .awb file
	if (!find_acb_file(input_file, acb_path, sizeof(acb_path))) {
		return 1;
	}

	printf("Extracting files from %s\n", extract_name_from_path(acb_path));
	if (run_acb_editor(acb_path) != 0) {
		return 1;
	}

	if (config.Convert_HCA_Into_WAV ) {
        printf("Converting HCAs into WAV in different CMD.\n");
		char folder_path[MAX_PATH];
		strcpy(folder_path, get_parent_directory(input_file));
		strcat(folder_path, "\\");
		strcat(folder_path, get_basename(input_file));
		if (process_hca_files(folder_path) != 0) {
			printf("Error extracting HCAs from %s\n",
			       extract_name_from_path(get_basename(input_file)));
		}
	}

	return 0;
}
