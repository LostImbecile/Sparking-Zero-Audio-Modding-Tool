#include "initialization.h"
#include "file_processor.h"
#include "config.h"
#include "utils.h"
#include <stdio.h>
#include <dirent.h>

extern Config config;
int extract_flag = 0;
int fixed_size = 0;
int cmd = 0;

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Usage: %s <file or folder paths>\n", extract_name_from_path(argv[0]));
		press_enter_to_exit();
		return 1;
	}

	char program_location[MAX_PATH] = {0};
	if (strstr(argv[0], ":") == NULL) {
		strcpy(program_location, ".\\");
		strcat(program_location, argv[0]);
	} else {
		strcpy(program_location, argv[0]);
	}

	if (initialize_program(program_location) != 0) {
		printf("Initialization failed.\n");
		press_enter_to_exit();
		return 1;
	}

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--extract") == 0) {
			extract_flag = 1;
		} else if (strcmp(argv[i], "--cmd") == 0) {
			cmd = 1;
		} else if (strcmp(argv[i], "--fixed-size") == 0) {
			fixed_size = 1;
		}
	}

	int return_value = 0;
	for (int i = 1; i < argc; i++) {
		if (strncmp(argv[i], "--", 2) == 0) continue;

		if (bgm_process_input(argv[i]) != 0) {
			printf("Error processing: %s\n", extract_name_from_path(argv[i]));
			return_value = 1;
		}
		printf("\n");
	}

	if (!cmd) {
		printf("Processing complete.\n");
		press_enter_to_exit();
	}
	return return_value;
}
