#include "file_packer.h"
#include "audio_converter.h"
#include "uasset_extractor.h"
#include "uasset_injector.h"
#include "utoc_generator.h"
#include "pak_generator.h"
#include "add_metadata.h"
#include <stdio.h>

static bool folder_processed = false;

bool was_folder_processed(void) {
	return folder_processed;
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

int pack_files(const char* foldername) {
	printf("Packaging files from folder: %s\n",
	       extract_name_from_path(foldername));

	// Step 1: Extract HCA key
	uint64_t hca_key = extract_hca_key(foldername);
	if (hca_key == 0) {
		printf("Failed to extract HCA key\n");
		return -1;
	}
	printf("Extracted HCA key: %" PRIu64 "\n", hca_key);

	// Step 2: Process all WAV files in the folder
	int conversion_result = process_wav_files(foldername, hca_key, 0);
	if (conversion_result != 0) {
		printf("Error during WAV to HCA conversion\n");
		return -1;
	}

	rename_files_back(foldername);


	// Step 3: Run ACBEditor on the folder
	int acb_result = run_acb_editor_pack(foldername);
	if (acb_result != 0) {
		return -1;
	}

	// Step 4: Run the inject process
	if (inject_process_file(foldername) != 0) {
		return -1;
	}

	if (config.Generate_Paks_And_Utocs
	        && generate_mod_packages(foldername) != 0) {
		return -1;
	}

	return 0;
}

int generate_mod_packages(const char* foldername) {
	char uasset_path[MAX_PATH];
	build_uasset_path(foldername, uasset_path, sizeof(uasset_path));

	if (config.Create_Separate_Mods) {
		const char* mod_name = get_mod_name();

		// Generate utoc & ucas in mods folder
		if (utoc_generate(uasset_path, mod_name) != 0) {
			return -1;
		}

		printf("\n");

		// Generate and replace Pak
		if (pak_generate(replace_extension(uasset_path, "awb"), mod_name) != 0) {
			return -1;
		}
	} else {
		if (utoc_create_structure(uasset_path, "temp_utoc") != 0) {
			return -1;
		}
		if (pak_create_structure(replace_extension(uasset_path, "awb"), "temp_pak")) {
			return -1;
		}
		folder_processed = true;
	}

	return 0;
}

const char* get_mod_name() {
	static char mod_name[MAX_PATH] =
	    "Mod_P"; // Static to ensure persistence after function exits
	char input[MAX_PATH];

	printf("\nEnter mod name ('_P' will be added by the tool) or press Enter for default 'Mod_P': ");
	if (fgets(input, sizeof(input), stdin)) {
		input[strcspn(input, "\n")] = 0; // Remove the newline character
		if (strlen(input) > 0) {
			strncpy(mod_name, input, MAX_PATH - 1);
			mod_name[MAX_PATH - 1] = '\0'; // Ensure null termination
		}
	}

	// Check if "_P" is already present in the mod_name
	size_t len = strlen(mod_name);
	if (len < 2 || strcasecmp(mod_name + len - 2, "_P") != 0) {
		strncat(mod_name, "_P", MAX_PATH - len -
		        1); // Append "_P" if not already at the end
	}

	return mod_name;
}

int package_combined_mod(const char* mod_name) {
	// First rename temp_utoc to mod_name
	if (rename_temp_folder("temp_utoc", mod_name) != 0) {
		return -1;
	}

	// Process the renamed utoc folder
	if (utoc_package_and_cleanup(mod_name) != 0) {
		return -1;
	}

	// Then rename temp_pak to mod_name
	if (rename_temp_folder("temp_pak", mod_name) != 0) {
		return -1;
	}

	// Process the renamed pak folder
	if (pak_package_and_cleanup(mod_name) != 0) {
		return -1;
	}

	return 0;
}

int run_acb_editor_pack(const char* folderpath) {
	char command[MAX_PATH * 8];

	snprintf(command, sizeof(command), "\"\"%s\" \"%s\"\"", acb_editor_path,
	         folderpath);

	int result = system(command);
	if (result != 0) {
		printf("Error: ACBEditor packaging failed with return code %d\n", result);
		return 1;
	}
	return 0;
}
