#pragma once
#ifndef INITIALIZATION_H
#define INITIALIZATION_H

#include <stdbool.h>
#include <stdlib.h>
#include "config.h" // defines the Config struct and MAX_PATH

// all global variables
typedef struct {
	char program_directory[MAX_PATH];
	char vgaudio_cli_path[MAX_PATH];
	char acb_editor_path[MAX_PATH];
	char unrealrezen_path[MAX_PATH];
	char unrealpak_path[MAX_PATH];
	char unrealpak_path_no_compression[MAX_PATH];
	char unrealpak_exe_path[MAX_PATH];
	char vgmstream_path[MAX_PATH];
	char bgm_tool_path[MAX_PATH];
	char metadata_tool_path[MAX_PATH];
	bool is_cmd_mode;
	Config config;
} AppData;

extern AppData app_data;

int initialise_program(const char* program_path);
char* get_program_file_path(const char* filename, char* buffer, size_t buffer_size);

#endif // INITIALIZATION_H
