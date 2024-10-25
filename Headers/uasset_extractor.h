#ifndef UASSET_EXTRACTOR_H
#define UASSET_EXTRACTOR_H

#include <stdio.h>

#define MAX_PATH 1024
#define BUFFER_SIZE 4096

int process_uasset(const char* uasset_path);
long find_utf_marker(FILE *fp);

#endif // UASSET_EXTRACTOR_H
