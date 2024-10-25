#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stdlib.h>

#define MAX_PATH 1024

typedef struct {
    bool Convert_HCA_Into_WAV;
    char Game_Directory[MAX_PATH];
} Config;

void config_init(Config* config);
bool config_load(Config* config, const char* filename);
const char* config_get_quoted_path(const char* path, char* buffer, size_t buffer_size);

#endif // CONFIG_H
