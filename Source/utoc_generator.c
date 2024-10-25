#include "utoc_generator.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>

static int check_existing_files(const char* game_dir, const char* mod_name) {
	char file_path[MAX_PATH];
	const char* extensions[] = {".utoc", ".pak", ".ucas"};
	int files_exist = 0;

	// Check each file type
	for (int i = 0; i < 3; i++) {
		snprintf(file_path, MAX_PATH, "%s\\~mods\\%s%s", game_dir, mod_name,
		         extensions[i]);
		FILE* file = fopen(file_path, "r");
		if (file != NULL) {
			fclose(file);
			files_exist = 1;
			printf("Found existing file: %s\n", file_path);
		}
	}

	// If files exist, ask user for permission to delete
	if (files_exist) {
		char response;
		printf("Existing mod files found. Delete them? (y/n): ");
		scanf(" %c", &response);

		if (tolower(response) == 'y') {
			// Delete each file type
			for (int i = 0; i < 3; i++) {
				snprintf(file_path, MAX_PATH, "%s\\~mods\\%s%s", game_dir, mod_name,
				         extensions[i]);
				if (remove(file_path) != 0) {
					printf("Warning: Failed to delete %s\n", file_path);
				}
			}
			printf("\n");
		} else {
			printf("Operation cancelled by user.\n");
			getchar();
			return 1;
		}
	}

	return 0;
}

static void cleanup(const char* folder) {
	if (remove_directory_recursive(folder) != 0
	        || remove("oo2core_9_win64.dll") != 0) {
		printf("Warning: Failed to clean up temporary files.\n");
	}
}

static void to_lower(char* str) {
	for (int i = 0; str[i]; i++) {
		str[i] = tolower(str[i]);
	}
}

int utoc_generate(const char* file_path, const char* mod_name) {
	char full_path[MAX_PATH];
	char mods_folder[MAX_PATH];
	char mod_folder[MAX_PATH];
	char cmd[1024];

	// Check for existing files before proceeding
	if (check_existing_files(config.Game_Directory, mod_name) != 0) {
		return 1;
	}

	snprintf(mod_folder, MAX_PATH, "%s%s", program_directory, mod_name);

	const char* subfolder = "";
	char lowercase_filename[MAX_PATH];
	strncpy(lowercase_filename, extract_name_from_path(file_path), MAX_PATH);
	to_lower(lowercase_filename);

	if (strstr(lowercase_filename, "bgm")) {
		printf("BGM files are not supported.\n");
		return 1;
	} else if (strstr(lowercase_filename, "btlcv")) {
		subfolder = strstr(lowercase_filename,
		                   "_jp") ? "\\Battle_VOICE\\JP" : "\\Battle_VOICE\\US";
	} else if (strstr(lowercase_filename, "btlse")
	           || strstr(lowercase_filename, "se_battle"))
		subfolder = "\\Battle_SE";
	else if (strstr(lowercase_filename,
	                "advif_cv")) subfolder = "\\ADV_VOICE\\_AtomCueSheet";
	else if (strstr(lowercase_filename,
	                "se_advif")) subfolder = "\\ADV_SE\\_AtomCueSheet";
	else if (strstr(lowercase_filename,
	                "voice_gallery")) subfolder = "\\GALLARY_VOICE";
	else if (strstr(lowercase_filename, "se_ui")) subfolder = "\\UI_SE";
	else if (strstr(lowercase_filename, "shop_item")) subfolder = "\\SHOP_ITEM";

	snprintf(full_path, MAX_PATH, "%s%s\\SparkingZERO\\Content\\SS\\Sounds%s",
	         program_directory, mod_name, subfolder);
	printf("Creating directory structure: %s\n", strstr(full_path,
	        "SparkingZERO\\Content"));

	if (create_directory_recursive(full_path) != 0) {
		cleanup(mod_folder);
		return 1;
	}

	snprintf(mods_folder, MAX_PATH, "%s\\~mods", config.Game_Directory);
	if (create_directory(mods_folder) != 0) {
		printf("Failed to create mods folder: %s\n", mods_folder);
		cleanup(mod_folder);
		return 1;
	}

	char dest_path[MAX_PATH];
	snprintf(dest_path, MAX_PATH, "%s\\%s", full_path,
	         extract_name_from_path(file_path));
	if (copy_file(file_path, dest_path) != 0) {
		printf("Failed to copy .uasset file\n");
		cleanup(mod_folder);
		return 1;
	}

	snprintf(cmd, sizeof(cmd),
	         "start \"\" /wait cmd /C  \"\"%s\" --content-path \"%s\\%s\" --compression-format Zlib "
	         "--engine-version GAME_UE5_1 --aes-key "
	         "0xb2407c45ea7c528738a94c0a25ea8f419de4377628eb30c0ae6a80dd9a9f3ef0 "
	         "--game-dir \"%s\" --output-path \"%s\\~mods\\%s.utoc\" && pause && exit\"",
	         unrealrezen_path, program_directory, mod_name,
	         config.Game_Directory, config.Game_Directory, mod_name);

	int result = system(cmd);
	if (result != 0) {
		printf("Failed to generate UTOC.\n");
		cleanup(mod_folder);
		return 1;
	}

	cleanup(mod_folder);
	printf("UTOC generation successful.\n");
	return 0;
}
