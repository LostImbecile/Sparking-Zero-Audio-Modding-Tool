#pragma once
#ifndef INITIALIZATION_H
#define INITIALIZATION_H

#include <stdbool.h>
#include <stdlib.h>


// Initialization functions
int initialize_program(const char* program_path);
char* get_program_file_path(const char* filename, char* buffer, size_t buffer_size);

#endif // INITIALIZATION_H
