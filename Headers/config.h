#pragma once
#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stdlib.h>
#include "utils.h"


typedef struct {
    bool Convert_HCA_Into_WAV;
    bool Create_Separate_Mods;
    bool Generate_Paks_And_Utocs;
    bool Fixed_Size_BGM;
    bool Use_Cue_Names;
    bool Dont_Use_Numbers;
    bool Disable_Looping;
    bool Disable_Metadata;
    bool Use_Cue_IDs;
    char Game_Directory[MAX_PATH];
} Config;

void config_init(Config* config);
bool config_load(Config* config, const char* filename);
const char* config_get_quoted_path(const char* path, char* buffer, size_t buffer_size);

#endif // CONFIG_H
