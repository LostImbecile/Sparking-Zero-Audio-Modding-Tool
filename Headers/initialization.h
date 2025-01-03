#pragma once
#ifndef INITIALIZATION_H
#define INITIALIZATION_H

#include <stdbool.h>
#include <stdlib.h>


// Global paths and states
extern char program_directory[];
extern char vgaudio_cli_path[];
extern char acb_editor_path[];
extern char unrealrezen_path[];
extern char unrealpak_path[];
extern char unrealpak_exe_path[];
extern char unrealpak_path_no_compression[];
extern char vgmstream_path[];
extern char metadata_tool_path[];
extern int csv_loaded;

// Initialization functions
int initialise_program(const char* program_path);
char* get_program_file_path(const char* filename, char* buffer, size_t buffer_size);
void load_csv(void);

#endif // INITIALIZATION_H
