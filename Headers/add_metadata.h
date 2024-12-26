#ifndef ADD_METADATA_H
#define ADD_METADATA_H

#include "track_info_utils.h"
#include "utils.h"

extern char metadata_tool_path[];

/**
 * @brief Creates a batch file that adds metadata to extracted audio files.
 *
 * @param input_file The path to the original .awb file.
 * @return 0 on success, non-zero on failure.
 */
int add_metadata(const char* input_file);

#endif // ADD_METADATA_H
