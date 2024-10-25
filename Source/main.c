#include "initialization.h"
#include "file_processor.h"
#include "file_preprocessor.h"
#include "utils.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Usage: %s <file or folder paths>\n", extract_name_from_path(argv[0]));
		printf("Press Enter to exit...");
		getchar();
		return 1;
	}


	char program_location[MAX_PATH] = {0};
	if (strstr(argv[0], ":") == NULL) {
		strcpy(program_location, ".\\");
		strcat(program_location, argv[0]);
	} else
		strcpy(program_location, argv[0]);

	// Initialize everything
	if (initialize_program(program_location) != 0) {
		printf("Initialization failed. Press Enter to exit...");
		getchar();
		return 1;
	}

	// Process arguments
	char** filtered_argv = preprocess_argv(&argc, argv);
	for (int i = 1; i < argc; i++) {
		if (process_input(filtered_argv[i]) != 0) {
			printf("Error processing input: %s\n",
			       extract_name_from_path(filtered_argv[i]));
		}
		printf("\n");
	}

	// Clean up
	free_filtered_argv(filtered_argv);
	printf("Processing complete. Press Enter to exit...");
	getchar();
	return 0;
}
