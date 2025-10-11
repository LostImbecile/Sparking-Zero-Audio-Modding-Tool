#pragma once
#ifndef ACB_MAPPING_H
#define ACB_MAPPING_H

#include "config.h"
#include <stdbool.h>

#define MAX_ACB_MAPPINGS 100

typedef struct {
    char awbName[MAX_PATH];
    char acbName[MAX_PATH];
    int tracks;
    int portNo;
} AcbMapping;

typedef struct {
    AcbMapping mappings[MAX_ACB_MAPPINGS];
    int mapping_count;
} AcbMappingData;

bool read_acb_mapping(const char* filepath, AcbMappingData* data);

#endif // ACB_MAPPING_H
