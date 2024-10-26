#pragma once
#ifndef FILE_PACKER_H
#define FILE_PACKER_H

#include <stdbool.h>
#include "config.h"

extern char acb_editor_path[];
extern Config config;
extern char program_directory[];

bool was_folder_processed(void);

const char* get_mod_name();

int package_combined_mod(const char* mod_name);

/**
 * @brief Packs files from a specified folder using ACBEditor
 * @param foldername Path to the folder containing files to pack
 * @return 0 on success, non-zero on failure
 */
int pack_files(const char* foldername);

/**
 * @brief Runs ACBEditor on the specified folder
 * @param folderpath Path to the folder to process
 * @return 0 on success, non-zero on failure
 */
int run_acb_editor_pack(const char* folderpath);

#endif // FILE_PACKER_H
