#include "pak_generator.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>

static void cleanup(const char* folder) {
	if (remove_directory_recursive(folder) != 0) {
		printf("Warning: Failed to clean up temporary directory: %s\n", folder);
	}
}

// Function to move file to mods folder
static int move_to_mods_folder(const char* source_path, const char* mod_name,
                               const char* game_directory) {
	char mods_folder[MAX_PATH];
	char dest_path[MAX_PATH];

	// Create mods folder path
	snprintf(mods_folder, MAX_PATH, "%s\\~mods", game_directory);

	// Create mods folder if it doesn't exist
	if (create_directory(mods_folder) != 0) {
		printf("Failed to create mods folder: %s\n", mods_folder);
		return 1;
	}

	// Create destination path
	snprintf(dest_path, MAX_PATH, "%s\\%s.pak", mods_folder, mod_name);

	remove(dest_path);
	if (rename(source_path, dest_path) != 0) {
		printf("Failed to move PAK file to mods folder\n");
		return 1;
	}

	return 0;
}

int pak_generate(const char* file_path, const char* mod_name) {
	char full_path[MAX_PATH];
	char mod_folder[MAX_PATH];
	char cmd[1024];
	char pak_path[MAX_PATH];

	snprintf(mod_folder, MAX_PATH, "%s%s", program_directory, mod_name);

	// Create the CriWareData directory path
	snprintf(full_path, MAX_PATH, "%s%s\\SparkingZERO\\Content\\CriWareData",
	         program_directory, mod_name);

	printf("Creating directory structure: %s\n", strstr(full_path,
	        "SparkingZERO\\Content"));

	// Create directory structure
	if (create_directory_recursive(full_path) != 0) {
		return 1;
	}

	// Copy the input file to the mod directory
	char dest_path[MAX_PATH];
	snprintf(dest_path, MAX_PATH, "%s\\%s", full_path,
	         extract_name_from_path(file_path));
	if (copy_file(file_path, dest_path) != 0) {
		printf("Failed to copy AWB file\n");
		cleanup(mod_folder);
		return 1;
	}

	// Create pak path
	snprintf(pak_path, MAX_PATH, "%s\\%s.pak", program_directory, mod_name);

	// Create unrealpak command
	snprintf(cmd, sizeof(cmd),
	         "start \"\" /wait cmd /C \"\"%s\" \"%s%s\" -compress && exit\"",
	         unrealpak_path, program_directory, mod_name);

	// Execute the command
	int result = system(cmd);
	if (result != 0) {
		printf("Failed to generate PAK.\n");
		cleanup(mod_folder);
		getchar();
		return 1;
	}

	// Move the generated PAK file to mods folder
	if (move_to_mods_folder(pak_path, mod_name, config.Game_Directory) != 0) {
		printf("Failed to move generated PAK to mods folder. Please do so manually.\n");
		cleanup(mod_folder);
		getchar();
		return 1;
	}

	cleanup(mod_folder);
	printf("PAK generation successful.\n");
	return 0;
}
