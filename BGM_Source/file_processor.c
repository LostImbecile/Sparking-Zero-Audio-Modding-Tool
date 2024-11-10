#include "file_processor.h"
#include "utils.h"
#include "awb.h"
#include "directory.h"
#include "inject.h"
#include <stdio.h>
#include <string.h>


int bgm_process_input(const char* input) {
	if (is_directory(input)) {
		return bgm_process_directory(input);
	} else { // Assuming it's a file at this point
		char* ext = get_file_extension(input);
		if (!ext) {
			printf("Invalid file: %s\n", extract_name_from_path(input));
			return 1;
		}

		if (strcasecmp(ext, "awb") == 0) {
			if (strstr(input, "DLC") != NULL || strstr(input, "dlc") != NULL ) {
				printf("DLC is not supported");
				return 1;
			}
			return bgm_process_awb_file(input);
		} else {
			printf("Unsupported file type: %s\n", ext);
			return 1;
		}
	}
}

int bgm_process_directory(const char* dir_path) {
	if (bgm_entry_count == 0) {
		fprintf(stderr, "Error reading BGM dictionary\n");
		return 1;
	}

	if (process_directory(dir_path)) {
		return 1;
	}
	return 0;
}

int bgm_process_awb_file(const char* file_path) {
	printf("Processing AWB file: %s\n", file_path);
	return process_awb_file(file_path);
}
