#include "pak_extractor.h"
#include "utils.h"
#include "initialization.h"
#include "bgm_processor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

int process_pak_file(const char* file_path) {
	char cmd[MAX_PATH * 8];
	char output_dir[MAX_PATH];
	char* base_name = get_basename(file_path);
	const char* parent_dir = get_parent_directory(file_path);

	snprintf(output_dir, sizeof(output_dir), "%s\\%s", parent_dir, base_name);

	// unreal(un)pak command
	snprintf(cmd, sizeof(cmd),
	         "\"\"%s\" \"%s\" -extract \"%s\"",
	         app_data.unrealpak_exe_path, file_path, output_dir);

	int result = system(cmd);
	if (result != 0) {
		printf("Failed to extract PAK.\n");
		free(base_name);
		return 1;
	}

	printf("PAK file extracted to: %s\n", output_dir);

	// Scan the output directory for any .awb files
	DIR *dir;
	struct dirent *ent;
	char awb_files[256][MAX_PATH]; // Assuming a maximum of 256 .awb files
	int awb_count = 0;

	if ((dir = opendir(output_dir)) != NULL) {
		while ((ent = readdir(dir)) != NULL && awb_count < 256) {
			const char* ext = get_file_extension(ent->d_name);
			if (ext && strcasecmp(ext, "awb") == 0) {
				snprintf(awb_files[awb_count], MAX_PATH, "%s\\%s", output_dir, ent->d_name);
				awb_count++;
			}
		}
		closedir(dir);
	} else {
		printf("Could not open extracted directory to scan for .awb files.\n");
	}

	// Extract the awbs, to allow users to reuse other mods
	if (awb_count > 0) {
		printf("\nFound %d .awb file(s) in the extracted folder. Do you want to extract their contents? (y/n): ",
		       awb_count);
		char response[10];
		if (fgets(response, sizeof(response), stdin)) {
			if (response[0] == 'y' || response[0] == 'Y') {
				printf("Processing .awb files...\n");
				for (int i = 0; i < awb_count; ++i) {
					process_bgm_input(awb_files[i]);
				}
			} else {
				printf("Skipping .awb file processing.\n");
			}
		}
		printf("\nNotes:");
		printf("\n- If you want to rename the files to \"streaming\", there's a .bat file included to do that");
		printf("\n- You should get the original uasset and awb first, extract them, ");
		printf("then drop the extracted WAVs in that folder, that's how you reuse them\n");
	}

	free(base_name);
	return 0;
}
