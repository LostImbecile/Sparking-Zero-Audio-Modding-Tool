#include "bgm_processor.h"
#include "file_packer.h"
#include "hcakey_generator.h"
#include "utils.h"
#include "file_processor.h"
#include "audio_converter.h"
#include "utoc_generator.h"
#include "pak_generator.h"
#include <stdio.h>
#include <string.h>

extern Config config;

int process_bgm_input(const char* input) {
	if (is_directory(input)) {
		return process_bgm_directory(input);
	} else {
		char* ext = get_file_extension(input);
		if (ext != NULL && strcasecmp(ext, "awb") == 0) {
			return process_bgm_awb_file(input);
		} else {
			printf("Error: .%s is not recognised for BGM files.", ext);
		}
	}
	return 0;
}

int get_last_mod_time(const char *file_path, time_t *mod_time) {
	struct stat file_stat;
	if (stat(file_path, &file_stat) != 0) {
		return -1;
	}
	*mod_time = file_stat.st_mtime;
	return 0;
}


static int rename_temp_folder(const char* temp_name, const char* mod_name) {
	char old_path[MAX_PATH];
	char new_path[MAX_PATH];

	// Build the full paths using program directory
	snprintf(old_path, MAX_PATH, "%s%s", program_directory, temp_name);
	snprintf(new_path, MAX_PATH, "%s%s", program_directory, mod_name);

	if (rename(old_path, new_path) != 0) {
		printf("Error: Failed to rename %s to %s\n", temp_name, mod_name);
		return -1;
	}

	return 0;
}

int process_bgm_directory(const char* dir_path) {
	const char* parent_dir = get_parent_directory(dir_path);
	char awb_path1[MAX_PATH];
	char awb_path2[MAX_PATH];
	char uasset_path[MAX_PATH];
	char command[MAX_PATH * 3];

	snprintf(awb_path1, MAX_PATH, "%s\\bgm_main.awb", parent_dir);
	snprintf(awb_path2, MAX_PATH, "%s\\bgm_main_Cnk_00.awb", parent_dir);
	snprintf(uasset_path, MAX_PATH, "%s\\bgm_main.uasset", parent_dir);

	int awb1_exists = 1, awb2_exists = 1;

	// Repack modified awb files if they exist
	if (!check_pair_exists(awb_path1, "awb")) {
		printf("Warning: bgm_main.awb not found.\n");
		awb1_exists = 0;
	}
	if (!check_pair_exists(awb_path2, "awb")) {
		if (awb1_exists) {
			printf("Warning: bgm_main_Cnk_00.awb not found.\n");
			awb2_exists = 0;
		} else {
			printf("Error: no bgm file is not present, no processing will be done.\n");
			return -1;
		}
	}

	if (!check_pair_exists(uasset_path, "uasset")) {
		printf("Error: bgm_main.uasset is not present, no processing will be done.\n");
		return -1;
	}

	uint64_t hca_key = 1013324708210664545;

	// Process all WAV files in the folder
	int conversion_result = process_wav_files(dir_path, hca_key, 1);
	if (conversion_result != 0) {
		printf("Error during WAV to HCA conversion\n");
		return -1;
	}

	time_t initial_mod_time_awb1, initial_mod_time_awb2;
	get_last_mod_time(awb_path1, &initial_mod_time_awb1);
	get_last_mod_time(awb_path2, &initial_mod_time_awb2);

	char arguments[20] = {0};
	strcpy(arguments, "--cmd");
	if (config.Fixed_Size_BGM)
		strcat(arguments, " --fixed-size");

	if (awb1_exists && awb2_exists) {
		snprintf(command, sizeof(command), "\"\"%s\" %s \"%s\" \"%s\" \"%s\"\"",
		         bgm_tool_path, arguments, awb_path1, awb_path2, dir_path);
	} else if (awb1_exists) {
		snprintf(command, sizeof(command), "\"\"%s\" %s \"%s\" \"%s\"\"",
		         bgm_tool_path, arguments, awb_path1, dir_path);
	} else {
		snprintf(command, sizeof(command), "\"\"%s\" %s \"%s\" \"%s\"\"",
		         bgm_tool_path, arguments, awb_path2, dir_path);
	}

	int result = system(command);
	if (result != 0) {
		printf("Error: BGM tool failed with return code %d\n", result);
		return 1;
	}

	if (config.Generate_Paks_And_Utocs) {
		int pak_awb1 = 0;
		int pak_awb2 = 0;

		if (awb1_exists) {
			time_t post_mod_time_awb1;
			if (get_last_mod_time(awb_path1, &post_mod_time_awb1) == 0 &&
			        post_mod_time_awb1 != initial_mod_time_awb1) {
				pak_awb1 = 1;
			} else
				printf("Note: bgm_main.awb was not modified and will be ignored.\n");
		}
		if (awb2_exists) {
			time_t post_mod_time_awb2;
			if (get_last_mod_time(awb_path2, &post_mod_time_awb2) == 0 &&
			        post_mod_time_awb2 != initial_mod_time_awb2) {
				pak_awb2 = 1;
			} else
				printf("Note: bgm_main_Cnk_00.awb was not modified and will be ignored.\n");
		}

		if (!pak_awb1 && !pak_awb2) {
			printf("Nothing to pack.\n");
			return 0;
		}

		const char* mod_name = get_mod_name();

		// Generate utoc & ucas in mods folder
		if (utoc_generate(uasset_path, mod_name) != 0) {
			return -1;
		}

		printf("\n");

		if (pak_awb1 && pak_create_structure(awb_path1, "temp_pak")) {
			return -1;
		}

		if (pak_awb2 && pak_create_structure(awb_path2, "temp_pak")) {
			return -1;
		}


		if (rename_temp_folder("temp_pak", mod_name) != 0) {
			return -1;
		}

		// Process the renamed pak folder
		if (pak_package_and_cleanup(mod_name) != 0) {
			return -1;
		}
	}

	return 0;
}

int process_bgm_awb_file(const char* file_path) {
	char answer;

	printf("Do you want to extract and convert %s to WAV? (y/n): ",
	       extract_name_from_path(file_path));
	scanf(" %c", &answer);

	if (answer == 'y' || answer == 'Y') {
		char folder_path[MAX_PATH];
		strcpy(folder_path, get_parent_directory(file_path));
		strcat(folder_path, "\\");
		strcat(folder_path, get_basename(file_path)); // Build the folder path

		char command[MAX_PATH * 2];
		snprintf(command, sizeof(command), "\"\"%s\" --extract --cmd \"%s\"\"",
		         bgm_tool_path, file_path); // extract flag

		int result = system(command);
		if (result != 0) {
			printf("Error: BGM tool failed with return code %d\n", result);
			return 1;
		}


		// Generate HCA key using the uasset in the created folder
		char uasset_path[MAX_PATH];
		snprintf(uasset_path, MAX_PATH, "%s/bgm_main.uasset",
		         get_parent_directory(file_path)); // Corrected path
		generate_hcakey_dir(uasset_path, folder_path);

		// Process HCA files in the folder
		if (process_hca_files(folder_path) != 0) {
			printf("Error extracting HCAs\n");
			return 1;
		}

		return 0;

	}
	return 0;
}

int pack_bgm_files(const char* foldername) {
	return pack_files(foldername);
}
