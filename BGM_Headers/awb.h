#ifndef AWB_H
#define AWB_H

#include "hca_tool.h"

int process_awb_file(const char* filename);
int process_uasset_file(const char* filename);
int write_header_csv(const char* filename, HCAHeader* headers, int count);
int read_header_csv(const char* filename, HCAHeader** headers, int* count);

typedef HCAHeader UAssetHeader;

#endif
