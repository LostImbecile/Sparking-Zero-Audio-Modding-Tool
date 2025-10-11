#include "initialization.h"
#include "hcakey_generator.h"
#include <stdio.h>
#include <string.h>

// global struct
AppData app_data;

static int initialize_tool_paths(void) {
	char tools_path[MAX_PATH];
	get_program_file_path("Tools\\", tools_path, sizeof(tools_path));

	// Initialize paths using the struct members
	snprintf(app_data.vgaudio_cli_path, MAX_PATH, "%sVGAudioCli.exe", tools_path);
	snprintf(app_data.acb_editor_path, MAX_PATH, "%sAcbEditor.exe", tools_path);
	snprintf(app_data.vgmstream_path, MAX_PATH, "%svgmstream-cli\\vgmstream-cli.exe",
	         tools_path);
	snprintf(app_data.unrealrezen_path, MAX_PATH, "%sUnrealReZen\\UnrealReZen.exe",
	         tools_path);
	snprintf(app_data.unrealpak_path, MAX_PATH,
	         "%sUnrealPak\\UnrealPak-With-Compression.bat", tools_path);
	snprintf(app_data.unrealpak_path_no_compression, MAX_PATH,
	         "%sUnrealPak\\UnrealPak-Without-Compression.bat", tools_path);
	snprintf(app_data.unrealpak_exe_path, MAX_PATH,
	         "%sUnrealPak\\UnrealPak.exe", tools_path);
	snprintf(app_data.bgm_tool_path, MAX_PATH, "%sBgmModdingTool.exe", tools_path);
	snprintf(app_data.metadata_tool_path, MAX_PATH, "%sAddWavMetadata.exe", tools_path);

	// Verify required executables exist
	FILE* vgaudio_test = fopen(app_data.vgaudio_cli_path, "r");
	if (!vgaudio_test) {
		fprintf(stderr, "Error: VGAudioCli.exe not found in Tools directory\n");
		return 1;
	}
	fclose(vgaudio_test);

	FILE* acbeditor_test = fopen(app_data.acb_editor_path, "r");
	if (!acbeditor_test) {
		fprintf(stderr, "Error: AcbEditor.exe not found in Tools directory\n");
		return 1;
	}
	fclose(acbeditor_test);

	FILE* bgmtool_test = fopen(app_data.bgm_tool_path, "r");
	if (!bgmtool_test) {
		fprintf(stderr, "Error: BgmModdingTool.exe not found in Tools directory\n");
	}
	fclose(bgmtool_test);

	return 0;
}

char* get_program_file_path(const char* filename, char* buffer,
                            size_t buffer_size) {
	snprintf(buffer, buffer_size, "%s%s", app_data.program_directory, filename);
	return buffer;
}

int initialise_program(const char* program_path) {
	// Set program directory
	strncpy(app_data.program_directory, program_path, MAX_PATH - 1);
	app_data.program_directory[MAX_PATH - 1] = '\0';

	char* last_backslash = strrchr(app_data.program_directory, '\\');
	if (last_backslash) {
		*(last_backslash + 1) = '\0';
	}

	// Initialize tool paths
	if (initialize_tool_paths() != 0) {
		fprintf(stderr, "Error: Failed to initialize tool paths\n");
		return 1;
	}

	char mapping_path[MAX_PATH];
	get_program_file_path("Tools\\acb_mapping.csv", mapping_path, sizeof(mapping_path));
	if (!read_acb_mapping(mapping_path, &app_data.acb_mapping_data)) {
		fprintf(stderr, "Warning: Could not read acb_mapping.csv. Metadata may be incorrect.\n");
	}

	// Load config file
	char config_path[MAX_PATH];
	get_program_file_path("config.ini", config_path, sizeof(config_path));
	if (!config_load(&app_data.config, config_path)) {
		fprintf(stderr, "Error: Failed to load configuration\n");
		return 1;
	}

	strcpy(app_data.config.Game_Directory, sanitize_path(app_data.config.Game_Directory));

	if (!is_path_exists(app_data.config.Game_Directory)
	        || strstr(app_data.config.Game_Directory, "\\Paks") == NULL) {
		fprintf(stderr,
		        "Your Game Path is incorrect.\nChange it in config.ini to point to the \\Paks folder.\n\n"\
		        "If you're sure it's correct, then it might have special characters that my tool does not support\n");
		return 1;
	}

	return 0;
}
