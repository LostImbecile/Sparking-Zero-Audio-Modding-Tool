#include "file_packer.h"
#include "audio_converter.h"
#include "uasset_extractor.h"
#include "uasset_injector.h"
#include "utoc_generator.h"
#include "pak_generator.h"
#include "utils.h"
#include <stdio.h>

int pack_files(const char* foldername) {
	printf("Packaging files from folder: %s\n",
	       extract_name_from_path(foldername));

	// Get parent directory for file operations
	char* parent_dir = get_parent_directory(foldername);

	// Step 1: Extract HCA key
	uint64_t hca_key = extract_hca_key(foldername);
	if (hca_key == 0) {
		printf("Failed to extract HCA key\n");
		free(parent_dir);
		return -1;
	}
	printf("Extracted HCA key: %" PRIu64 "\n", hca_key);

	// Step 2: Process all WAV files in the folder
	int conversion_result = process_wav_files(foldername, hca_key);
	if (conversion_result != 0) {
		printf("Error during WAV to HCA conversion\n");
		free(parent_dir);
		return -1;
	}

	// Step 3: Run ACBEditor on the folder
	int acb_result = run_acb_editor_pack(foldername);
	if (acb_result != 0) {
		free(parent_dir);
		return -1;
	}

	// Step 4: Run the inject process
	if (inject_process_file(foldername) != 0) {
		free(parent_dir);
		return -1;
	}

	char uasset_path[MAX_PATH];
	build_uasset_path(foldername, uasset_path, sizeof(uasset_path));

	char mod_name[MAX_PATH] = "Mod_P";
	printf("\nEnter mod name ('_P' recommended to suffix) or press Enter for default 'Mod_P': ");
	char input[MAX_PATH];
	if (fgets(input, sizeof(input), stdin)) {
		input[strcspn(input, "\n")] = 0;
		if (strlen(input) > 0) {
			strncpy(mod_name, input, MAX_PATH - 1);
		}
	}

	// Generate utoc & ucas in mods folder
	if (utoc_generate(uasset_path, mod_name) != 0) {
		free(parent_dir);
		return -1;
	}

	printf("\n");

	// Generate and replace Pak
	if (pak_generate(replace_extension(uasset_path, "awb"), mod_name) != 0) {
		free(parent_dir);
		return -1;
	}

	free(parent_dir);
	return 0;
}

int run_acb_editor_pack(const char* folderpath) {
	char command[MAX_PATH * 2];

	snprintf(command, sizeof(command), "\"\"%s\" \"%s\"\"", acb_editor_path,
	         folderpath);

	int result = system(command);
	if (result != 0) {
		printf("Error: ACBEditor packaging failed with return code %d\n", result);
		return 1;
	}
	return 0;
}
