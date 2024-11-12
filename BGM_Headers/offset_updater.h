#ifndef OFFSET_UPDATER_H_INCLUDED
#define OFFSET_UPDATER_H_INCLUDED

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "awb.h"

bool update_offset_range(const char* awb_path, const char* uasset_path,
                         const HCAHeader* headers, int start_idx, int end_idx, int file_end_offset);
bool update_offset_range_with_padding(const char* awb_path,const char* uasset_path,
                                      const HCAHeader* headers,
                                      int start_idx, int end_idx, int file_end_offset,
                                      long size_difference, int padding_count);


#endif // OFFSET_UPDATER_H_INCLUDED
