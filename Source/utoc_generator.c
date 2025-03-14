#include "utoc_generator.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>

static int verify_utoc_generation(const char* game_dir, const char* mod_name) {
    char file_path[MAX_PATH];
    const char* extensions[] = {".utoc", ".pak", ".ucas"};
    int success = 1;  // Start optimistic, set to 0 if any check fails

    // Check existence and size of each required file
    for (int i = 0; i < 3; i++) {
        snprintf(file_path, MAX_PATH, "%s\\~mods\\%s%s", game_dir, mod_name, extensions[i]);

        // Check if file exists
        struct stat file_stat;
        if (stat(file_path, &file_stat) != 0) {
            printf("Error: Missing required file: %s\n", file_path);
            success = 0;
            continue;
        }

        // For .ucas file, check if size is greater than 1KB
        if (strcmp(extensions[i], ".ucas") == 0) {
            if (file_stat.st_size < 1024) {  // 1KB
                success = 0;
            }
        }
    }

    if (!success) {
        printf("UTOC generation failed: some files are missing or invalid.\n");
        printf("Ensure that your file exists in the game, or check common errors in the guide.\n");
        getchar();
    }

    return success;
}

static int copy_oo2core() {
    char source_path[MAX_PATH];
    char dest_path[MAX_PATH];

    snprintf(source_path, MAX_PATH, "%s\\oo2core_9_win64.dll", get_parent_directory(unrealrezen_path));
    snprintf(dest_path, MAX_PATH, "oo2core_9_win64.dll");


    if (copy_file(source_path, dest_path) != 0) {
        return 1;
    }
    return 0;
}

static int check_existing_files(const char* game_dir, const char* mod_name) {
    char mods_path[MAX_PATH];
    char file_path[MAX_PATH];
    const char* extensions[] = {".utoc", ".pak", ".ucas"};
    int files_exist = 0;
    DIR* dir;
    struct dirent* entry;


    char mod_name_clean[MAX_PATH];
    strncpy(mod_name_clean, mod_name, MAX_PATH - 1);
    mod_name_clean[MAX_PATH - 1] = '\0';
    size_t mod_len = strlen(mod_name_clean);

    // Remove _p if present
    if (mod_len > 2 && strcasecmp(mod_name_clean + mod_len - 2, "_p") == 0) {
        mod_name_clean[mod_len - 2] = '\0';
    }

    snprintf(mods_path, MAX_PATH, "%s\\~mods", game_dir);

    dir = opendir(mods_path);
    if (dir == NULL) {
        printf("Could not open mods directory.\n");
        return 1;
    }

    // Read all files in mods folder
    while ((entry = readdir(dir)) != NULL) {
        char* dot_pos = strrchr(entry->d_name, '.');
        if (!dot_pos) continue;

        size_t name_len = dot_pos - entry->d_name;
        char filename[MAX_PATH];
        strncpy(filename, entry->d_name, name_len);
        filename[name_len] = '\0';

        size_t file_len = strlen(filename);
        // Remove _p from name (just in case)
        if (file_len > 2 && strcasecmp(filename + file_len - 2, "_p") == 0) {
            filename[file_len - 2] = '\0';
        }

        // Check if any mod of the same name more or less exists
        if (strcasecmp(filename, mod_name_clean) == 0) {
            for (int i = 0; i < 3; i++) {
                if (strcasecmp(dot_pos, extensions[i]) == 0) {
                    files_exist = 1;
                    snprintf(file_path, MAX_PATH, "%s\\~mods\\%s", game_dir, entry->d_name);
                    printf("Found existing file: %s\n", file_path);
                    break;
                }
            }
        }
    }
    closedir(dir);

    // Ask user to delete or keep
    if (files_exist) {
        char response;
        printf("Existing mod files found. Delete them? (y/n): ");
        scanf(" %c", &response);

        if (tolower(response) == 'y') {
            dir = opendir(mods_path);
            if (dir != NULL) {
                while ((entry = readdir(dir)) != NULL) {
                    char* dot_pos = strrchr(entry->d_name, '.');
                    if (!dot_pos) continue;

                    size_t name_len = dot_pos - entry->d_name;
                    char filename[MAX_PATH];
                    strncpy(filename, entry->d_name, name_len);
                    filename[name_len] = '\0';

                    size_t file_len = strlen(filename);
                    if (file_len > 2 && strcasecmp(filename + file_len - 2, "_p") == 0) {
                        filename[file_len - 2] = '\0';
                    }

                    if (strcasecmp(filename, mod_name_clean) == 0) {
                        for (int i = 0; i < 3; i++) {
                            if (strcasecmp(dot_pos, extensions[i]) == 0) {
                                snprintf(file_path, MAX_PATH, "%s/~mods/%s", game_dir, entry->d_name);
                                if (remove(file_path) != 0) {
                                    printf("Warning: Failed to delete %s\n", file_path);
                                }
                                break;
                            }
                        }
                    }
                }
                closedir(dir);
                printf("\n");
            }
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
	if (utoc_create_structure(file_path, mod_name) != 0) {
		return 1;
	}

	return utoc_package_and_cleanup(mod_name);
}

int utoc_create_structure(const char* file_path, const char* mod_name) {
	char full_path[MAX_PATH];
	char mods_folder[MAX_PATH];
	char mod_folder[MAX_PATH];

	// Check for existing files before proceeding
	if (check_existing_files(config.Game_Directory, mod_name) != 0) {
		return 1;
	}

	snprintf(mod_folder, MAX_PATH, "%s%s", program_directory, mod_name);

	const char* subfolder = "";
	char lowercase_filename[MAX_PATH];
	strncpy(lowercase_filename, extract_name_from_path(file_path), MAX_PATH);
	to_lower(lowercase_filename);

	// Determine subfolder based on filename
	if (strstr(lowercase_filename, "bgm")
	        && strstr(lowercase_filename, "dlc") == NULL) {
	            subfolder = "\\BGM";
	}else if (strstr(lowercase_filename, "btlcv")) {
		subfolder = strstr(lowercase_filename,
		                   "_jp") ? "\\Battle_VOICE\\JP" : "\\Battle_VOICE\\US";
	} else if (strstr(lowercase_filename, "btlse")
	           || strstr(lowercase_filename, "se_battle"))
		subfolder = "\\Battle_SE";
	else if (strstr(lowercase_filename, "advif_cv"))
		subfolder = "\\ADV_VOICE\\_AtomCueSheet";
	else if (strstr(lowercase_filename, "se_advif"))
		subfolder = "\\ADV_SE\\_AtomCueSheet";
	else if (strstr(lowercase_filename, "voice_gallery"))
		subfolder = "\\GALLARY_VOICE";
	else if (strstr(lowercase_filename, "se_ui"))
		subfolder = "\\UI_SE";
	else if (strstr(lowercase_filename, "shop_item"))
		subfolder = "\\SHOP_ITEM";

	// Create full directory path
	if (strstr(lowercase_filename, "dlc_01")) {
		snprintf(full_path, MAX_PATH,
		         "%s%s\\SparkingZERO\\Plugins\\DLC_AnimeSongsBGMPack1\\Content",
		         program_directory, mod_name);
		printf("Creating directory structure: %s\n", strstr(full_path,
		        "SparkingZERO\\Plugins"));
	} else if (strstr(lowercase_filename, "dlc_02")) {
		snprintf(full_path, MAX_PATH,
		         "%s%s\\SparkingZERO\\Plugins\\DLC_AnimeSongsBGMPack2\\Content",
		         program_directory, mod_name);
		printf("Creating directory structure: %s\n", strstr(full_path,
		        "SparkingZERO\\Plugins"));
	} else {
		snprintf(full_path, MAX_PATH, "%s%s\\SparkingZERO\\Content\\SS\\Sounds%s",
		         program_directory, mod_name, subfolder);
		printf("Creating directory structure: %s\n", strstr(full_path,
		        "SparkingZERO\\Content"));
	}


	// Create directories
	if (create_directory_recursive(full_path) != 0) {
		return 1;
	}

	// Create mods folder
	snprintf(mods_folder, MAX_PATH, "%s\\~mods", config.Game_Directory);
	if (create_directory(mods_folder) != 0) {
		printf("Failed to create mods folder: %s\n", mods_folder);
		return 1;
	}

	// Copy the file
	char dest_path[MAX_PATH];
	snprintf(dest_path, MAX_PATH, "%s\\%s", full_path,
	         extract_name_from_path(file_path));
	if (copy_file(file_path, dest_path) != 0) {
		printf("Failed to copy .uasset file\n");
		return 1;
	}

	return 0;
}

int utoc_package_and_cleanup(const char* mod_name) {
	char cmd[MAX_PATH * 8];
	char mod_folder[MAX_PATH];
	snprintf(mod_folder, MAX_PATH, "%s%s", program_directory, mod_name);

	char game_dir[MAX_PATH];
	if (strstr(config.Game_Directory,"Content\\Paks") != NULL) {
		strcpy(game_dir,get_parent_directory(config.Game_Directory));
	} else
		strcpy(game_dir,config.Game_Directory);

	// Second check for cases where mod name wasn't given when structure was created
    if (check_existing_files(config.Game_Directory, mod_name) != 0) {
		return 1;
	}

    copy_oo2core();

	// Generate UTOC command
	snprintf(cmd, sizeof(cmd),
	         "start \"\" /wait cmd /C \"\"%s\" --content-path \"%s%s\" --compression-format Zlib "
	         "--engine-version GAME_UE5_1 --aes-key "
	         "0xb2407c45ea7c528738a94c0a25ea8f419de4377628eb30c0ae6a80dd9a9f3ef0 "
	         "--game-dir \"%s\" --output-path \"%s\\~mods\\%s.utoc\" && exit\"",
	         unrealrezen_path, program_directory, mod_name,
	         game_dir, config.Game_Directory, mod_name);

	int result = system(cmd);
	if (result != 0) { // Note that I use && exit which trashes the return value
		printf("Failed to generate UTOC.\n");
		cleanup(mod_folder);
		return 1;
	}

	if (!verify_utoc_generation(config.Game_Directory, mod_name)) {
        cleanup(mod_folder);
        return 1;
    }

	cleanup(mod_folder);
	printf("UTOC generation successful.\n");
	return 0;
}
