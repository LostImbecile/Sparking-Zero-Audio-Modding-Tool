#include "config.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define DEFAULT_CONFIG_CONTENT \
"# Configuration file for the application\n\n" \
"# If true, turns all hcas into wavs on extraction (To listen to them) and converts them back on packaging.\n" \
"# Note: operations will take longer.\n" \
"Convert_HCA_Into_WAV=false\n\n" \
"# If false, all processed sound folders will be combined into a single mod package.\n" \
"# Use true to create separate mods for each folder.\n" \
"# Use false to combine all folders into one mod.\n" \
"Create_Separate_Mods=true\n\n" \
"# Generate .pak and .utoc files with UnrealRezen and UnrealPak. Skips the process if false.\n" \
"# Disabling is useful if you want to add non-audio parts to your package.\n" \
"Generate_Paks_&_Utocs=true\n\n" \
"# Game directory to move the output paks into\n" \
"# It should point to your \\Paks folder\n" \
"Game_Directory=\"C:\\Program Files (x86)\\Steam\\steamapps\\common\\DRAGON BALL Sparking! ZERO\\SparkingZERO\\Content\\Paks\"\n\n" \
"# Make BGM size fixed, this will make any BGM bugs disappear but HCA size will be constrained\n" \
"# Use this if you care about Menu music, but you might have to reduce your HCA's size\n" \
"Fixed_Size_BGM=false\n\n"

// Initialize config with default values
void config_init(Config* config) {
	config->Convert_HCA_Into_WAV = true;
	config->Create_Separate_Mods = true;
	config->Generate_Paks_And_Utocs = true;
	config->Fixed_Size_BGM = false;
	strcpy(config->Game_Directory,
	       "C:\\Program Files (x86)\\Steam\\steamapps\\common\\DRAGON BALL Sparking! ZERO\\SparkingZERO\\Content\\Paks");
}

// Helper functions
static void trim(char* str) {
	char* start = str;
	char* end = str + strlen(str) - 1;
	while (isspace((unsigned char)*start)) start++;
	while (end > start && isspace((unsigned char)*end)) end--;
	end[1] = '\0';
	memmove(str, start, end - start + 2);
}

static void remove_quotes(char* str) {
	size_t len = strlen(str);
	if (len >= 2 && str[0] == '"' && str[len - 1] == '"') {
		memmove(str, str + 1, len - 2);
		str[len - 2] = '\0';
	}
}

// Parse a single config line
static void parse_line(Config* config, char* line) {
	char* key = strtok(line, "=");
	char* value = strtok(NULL, "\n");
	if (!key || !value) return;

	trim(key);
	trim(value);
	remove_quotes(value);

	char key_lower[256] = {0};
	strcpy(key_lower, key);
	strlwr(key_lower);

	if (strcmp(key_lower, "convert_hca_into_wav") == 0) {
		config->Convert_HCA_Into_WAV = (strcasecmp(value, "true") == 0);
	} else if (strcmp(key_lower, "create_separate_mods") == 0) {
		config->Create_Separate_Mods = (strcasecmp(value, "true") == 0);
	} else if (strcmp(key_lower, "game_directory") == 0) {
		strcpy(config->Game_Directory, value);
	} else if (strcmp(key_lower, "generate_paks_&_utocs") == 0) {
		config->Generate_Paks_And_Utocs = (strcasecmp(value, "true") == 0);
	}else if (strcmp(key_lower, "fixed_size_bgm") == 0) {
		config->Fixed_Size_BGM = (strcasecmp(value, "true") == 0);
	}
}

// Load or create config file
	bool config_load(Config * config, const char* filename) {
		config_init(config);  // Initialize with defaults

		FILE* file = fopen(filename, "r");
		if (!file) {
			// Create default config file if it doesn't exist
			file = fopen(filename, "w");
			if (!file) return false;
			fprintf(file, "%s", DEFAULT_CONFIG_CONTENT);
			fclose(file);
			return true;
		}

		char line[1024];
		while (fgets(line, sizeof(line), file)) {
			if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
			parse_line(config, line);
		}
		fclose(file);
		return true;
	}

// Helper function for quoted paths
	const char* config_get_quoted_path(const char* path, char* buffer,
	                                   size_t buffer_size) {
		if (path[0] == '"') {
			strncpy(buffer, path, buffer_size - 1);
			buffer[buffer_size - 1] = '\0';
		} else {
			snprintf(buffer, buffer_size, "\"%s\"", path);
		}
		return buffer;
	}
