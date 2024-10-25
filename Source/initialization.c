#include "initialization.h"
#include "config.h"
#include "hcakey_generator.h"
#include <stdio.h>
#include <string.h>

#define MAX_PATH 1024

char program_directory[MAX_PATH] = {0};
char vgaudio_cli_path[MAX_PATH] = {0};
char acb_editor_path[MAX_PATH] = {0};
char unrealrezen_path[MAX_PATH] = {0};
char unrealpak_path[MAX_PATH] = {0};
char vgmstream_path[MAX_PATH] = {0};
int csv_loaded = 0;
Config config;

static int initialize_tool_paths(void) {
    char tools_path[MAX_PATH];
    get_program_file_path("Tools\\", tools_path, sizeof(tools_path));

    // Initialize paths
    snprintf(vgaudio_cli_path, MAX_PATH, "%sVGAudioCli.exe", tools_path);
    snprintf(acb_editor_path, MAX_PATH, "%sAcbEditor.exe", tools_path);
    snprintf(vgmstream_path, MAX_PATH, "%svgmstream-cli\\vgmstream-cli.exe", tools_path);
    snprintf(unrealrezen_path, MAX_PATH, "%sUnrealReZen\\UnrealReZen.exe", tools_path);
    snprintf(unrealpak_path, MAX_PATH, "%sUnrealPak\\UnrealPak-With-Compression.bat", tools_path);

    // Verify required executables exist
    FILE* vgaudio_test = fopen(vgaudio_cli_path, "r");
    if (!vgaudio_test) {
        fprintf(stderr, "Error: VGAudioCli.exe not found in Tools directory\n");
        return 1;
    }
    fclose(vgaudio_test);

    FILE* acbeditor_test = fopen(acb_editor_path, "r");
    if (!acbeditor_test) {
        fprintf(stderr, "Error: AcbEditor.exe not found in Tools directory\n");
        return 1;
    }
    fclose(acbeditor_test);

    return 0;
}

char* get_program_file_path(const char* filename, char* buffer, size_t buffer_size) {
    snprintf(buffer, buffer_size, "%s%s", program_directory, filename);
    return buffer;
}

void load_csv(void) {
    char keys_path[MAX_PATH];
    get_program_file_path("keys.csv", keys_path, sizeof(keys_path));
    csv_loaded = read_csv(keys_path);
    if (!csv_loaded) {
        fprintf(stderr, "Note: Failed to load keys.csv from program directory.\n");
    }
}

int initialize_program(const char* program_path) {
    // Set program directory
    strncpy(program_directory, program_path, MAX_PATH - 1);
    program_directory[MAX_PATH - 1] = '\0';

    char* last_backslash = strrchr(program_directory, '\\');
    if (last_backslash) {
        *(last_backslash + 1) = '\0';
    }

    // Initialize tool paths
    if (initialize_tool_paths() != 0) {
        fprintf(stderr, "Error: Failed to initialize tool paths\n");
        return 1;
    }

    // Load config file
    char config_path[MAX_PATH];
    get_program_file_path("config.ini", config_path, sizeof(config_path));
    if (!config_load(&config, config_path)) {
        fprintf(stderr, "Error: Failed to load configuration\n");
        return 1;
    }

    // Load CSV data
    load_csv();

    return 0;
}
