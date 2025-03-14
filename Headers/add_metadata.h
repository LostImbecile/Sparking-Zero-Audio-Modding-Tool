#ifndef ADD_METADATA_H
#define ADD_METADATA_H

#include "track_info_utils.h"
#include "utils.h"
#include "config.h"
#include "file_mapping.h"

extern char metadata_tool_path[];
extern Config config;

/**
 * @brief Creates a batch file that adds metadata to extracted audio files.
 *
 * @param input_file The path to the original .awb file.
 * @return 0 on success, non-zero on failure.
 */
int add_metadata(const char* input_file);

char* generate_file_name(const char* sanitized_name, int original_num, const char* extension,
                        FileMappingList* mapping, int config_dont_use_numbers);

void rename_files_back(const char* foldername);
int rename_hcas(const char* input_file);
void sanitize_filename(const char* input, char* output);

#endif // ADD_METADATA_H
