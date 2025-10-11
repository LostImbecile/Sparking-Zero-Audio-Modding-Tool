#include "bgm_processor.h"
#include "file_packer.h"
#include "hcakey_generator.h"
#include "file_processor.h"
#include "audio_converter.h"
#include "utoc_generator.h"
#include "pak_generator.h"
#include "add_metadata.h"
#include "uasset_extractor.h"
#include <stdio.h>
#include <string.h>

int process_bgm_input(const char* input) {
	if (is_directory(input)) {
		return process_bgm_directory(input);
	} else {
		const char* ext = get_file_extension(input);
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
	snprintf(old_path, MAX_PATH, "%s%s", app_data.program_directory, temp_name);
	snprintf(new_path, MAX_PATH, "%s%s", app_data.program_directory, mod_name);

	if (rename(old_path, new_path) != 0) {
		printf("Error: Failed to rename %s to %s\n", temp_name, mod_name);
		return -1;
	}

	return 0;
}
typedef struct {
	char base_name[32]; // Store the base name (e.g., "bgm_main")
	char awb_path[MAX_PATH];
	char uasset_path[MAX_PATH];
	bool awb_exists;
	time_t initial_mod_time_awb;
	uint64_t hca_key;
} BGMFile;

// Function to construct full paths
void construct_paths(BGMFile *bgm, const char *parent_dir) {
	snprintf(bgm->awb_path, MAX_PATH, "%s\\%s.awb", parent_dir, bgm->base_name);
	snprintf(bgm->uasset_path, MAX_PATH, "%s\\%s.uasset", parent_dir,
	         bgm->base_name);
}

int process_bgm_directory(const char* dir_path) {
	const char* parent_dir = get_parent_directory(dir_path);
	char command[MAX_PATH * 8];

	BGMFile bgm_files[] = {
		{"bgm_main", "", "", false, 0, 0},
		{"bgm_main_Cnk_00", "", "", false, 0, 0},
		{"bgm_DLC_01", "", "", false, 0, 0},
		{"bgm_DLC_02", "", "", false, 0, 0}
	};
	const int num_bgm_files = sizeof(bgm_files) / sizeof(bgm_files[0]);

	const char* dir_basename = extract_name_from_path(dir_path);
	int bgm_index = -1;

	if (strcmp(dir_basename, "bgm_DLC_01") == 0)      bgm_index = 2;
	else if (strcmp(dir_basename, "bgm_DLC_02") == 0) bgm_index = 3;
	else bgm_index = 0; // Default to bgm_main

	uint64_t hca_key_to_use = 0;

	// Initialize and check existence
	for (int i = 0; i < num_bgm_files; ++i) {
		if (bgm_index == 0 && (i == 0
		                       || i == 1)) construct_paths(&bgm_files[i], parent_dir);
		else if (bgm_index > 0
		         && i == bgm_index) construct_paths(&bgm_files[i], parent_dir);

		if ((bgm_index == 0 && (i == 0 || i == 1)) || (bgm_index > 0
		        && i == bgm_index)) {
			bgm_files[i].awb_exists = check_pair_exists(bgm_files[i].awb_path, "awb");
			if (bgm_files[i].awb_exists) {
				get_last_mod_time(bgm_files[i].awb_path, &bgm_files[i].initial_mod_time_awb);
				uint64_t key = get_key(bgm_files[i].awb_path);
				if (key != (uint64_t) -1) {
					bgm_files[i].hca_key = key;
					if (hca_key_to_use == 0) hca_key_to_use = bgm_files[i].hca_key;
				} else {
					printf("Error: Could not find HCA key for %s\n", bgm_files[i].awb_path);
					return -1;
				}
			}
		}
	}

	// Check if relevant AWB(s) exist
	if (bgm_index == 0 && (!bgm_files[0].awb_exists
	                       && !bgm_files[1].awb_exists)) {
		printf("Error: bgm_main.awb and bgm_main_Cnk_00.awb are not present, no processing will be done.\n");
		return -1;
	} else if (bgm_index > 0 && !bgm_files[bgm_index].awb_exists) {
		printf("Error: %s is not present, no processing will be done.\n",
		       bgm_files[bgm_index].awb_path);
		return -1;
	}

	// Process WAVs and encrypt HCAs
	if (process_wav_files(dir_path, hca_key_to_use, !app_data.config.Disable_Looping) != 0) {
		printf("Error during WAV to HCA conversion\n");
		return -1;
	}
	int encryption_successes = encrypt_hcas(dir_path, hca_key_to_use);
	if (encryption_successes > 0) printf("%d HCAs were encrypted\n",
		                                     encryption_successes);

	rename_files_back(dir_path);

	// Prepare bgm_tool arguments
	char arguments[20] = "--cmd";
	if (app_data.config.Fixed_Size_BGM) strcat(arguments, " --fixed-size");

	// Execute bgm_tool command
	if (bgm_index == 0) {
		snprintf(command, sizeof(command), "\"\"%s\" %s \"%s\" \"%s\" \"%s\"\"",
		         app_data.bgm_tool_path, arguments, bgm_files[0].awb_path,
		         bgm_files[1].awb_path,
		         dir_path);
	} else {
		snprintf(command, sizeof(command), "\"\"%s\" %s \"%s\" \"%s\"\"",
		         app_data.bgm_tool_path, arguments, bgm_files[bgm_index].awb_path, dir_path);
	}

	int result = system(command);
	if (result != 0) {
		printf("Error: BGM tool failed with return code %d\n", result);
		return 1;
	}

	// Handle pak generation
	if (app_data.config.Generate_Paks_And_Utocs) {
		const char* mod_name = get_mod_name();
		bool any_modified = false;

		// Check for modified files
		for (int i = 0; i < num_bgm_files; ++i) {
			if ((bgm_index == 0 && (i == 0 || i == 1)) || (bgm_index > 0
			        && i == bgm_index)) {
				if (bgm_files[i].awb_exists) {
					time_t current_mod_time;
					if (get_last_mod_time(bgm_files[i].awb_path, &current_mod_time) == 0) {
						if (current_mod_time != bgm_files[i].initial_mod_time_awb) {
							any_modified = true;
							printf("Note: %s was modified and will be packed.\n",
							       extract_name_from_path(bgm_files[i].awb_path));
						}
					}
				}
			}
		}

		if (!any_modified) {
			printf("Note: No BGM files were modified. Skipping utoc and pak generation.\n");
			return 0;
		}

		// Generate utoc for each modified file
		int utoc_result = -1;
		if (bgm_index == 0)
			utoc_result = utoc_generate(bgm_files[0].uasset_path, mod_name);
		else
			utoc_result = utoc_generate(bgm_files[bgm_index].uasset_path, mod_name);

		if (utoc_result != 0)
			return -1;

		// Create pak structure for modified files
		for (int i = 0; i < num_bgm_files; ++i) {
			if ((bgm_index == 0 && (i == 0 || i == 1)) || (bgm_index > 0
			        && i == bgm_index)) {
				if (bgm_files[i].awb_exists) {
					time_t current_mod_time;
					if (get_last_mod_time(bgm_files[i].awb_path, &current_mod_time) == 0 &&
					        current_mod_time != bgm_files[i].initial_mod_time_awb) {
						if (pak_create_structure(bgm_files[i].awb_path, "temp_pak") != 0) return -1;
					}
				}
			}
		}

		if (rename_temp_folder("temp_pak", mod_name) != 0
		        || pak_package_and_cleanup(mod_name) != 0)
			return -1;
	}

	return 0;
}

int process_bgm_awb_file(const char* file_path) {

	printf("Extracting %s and converting files to WAV...\n",
	       extract_name_from_path(file_path));
	char folder_path[MAX_PATH];
	strcpy(folder_path, get_parent_directory(file_path));
	strcat(folder_path, "\\");
	strcat(folder_path, get_basename(file_path)); // Build the folder path

	// extract to get the HCAs, started in the relevant directory
	char command[MAX_PATH * 8];
	snprintf(command, sizeof(command),
	         "cd /d \"%s\" && \"%s\" --extract --cmd \"%s\"",
	         get_parent_directory(file_path), app_data.bgm_tool_path, file_path);

	int result = system(command);
	if (result != 0) {
		printf("Error: BGM tool failed with return code %d\n", result);
		return 1;
	}


	// Generate HCA key using the uasset in the created folder
	char uasset_path[MAX_PATH];
	if (strstr(file_path, "bgm_main") != NULL)
		snprintf(uasset_path, MAX_PATH, "%s\\bgm_main.uasset",
		         get_parent_directory(file_path)); // Corrected path
	else {
		snprintf(uasset_path, MAX_PATH, "%s\\%s",
		         get_parent_directory(file_path),
		         replace_extension(extract_name_from_path(file_path), "uasset"));
	}
	generate_hcakey_dir(uasset_path, folder_path);

	if (!app_data.config.Disable_Metadata && process_uasset(uasset_path) == 0) {
		generate_txtm(file_path);
		add_metadata(file_path);
	}
	// Process HCA files in the folder
	if (process_hca_files(folder_path) != 0) {
		printf("Error extracting HCAs\n");
		return 1;
	}

	return 0;
}

int pack_bgm_files(const char* foldername) {
	return pack_files(foldername);
}
