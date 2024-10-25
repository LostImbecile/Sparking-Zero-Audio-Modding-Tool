#include "config.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define DEFAULT_CONFIG_CONTENT \
"# Configuration file for the application\n\n" \
"# If true, turns all hcas into wavs on extraction and converts them back on packaging. Note: operations will take longer.\n" \
"Convert_HCA_Into_WAV=false\n\n" \
"# Game directory to move the output paks into\n" \
"Game_Directory=\"C:\\Program Files (x86)\\Steam\\steamapps\\common\\DRAGON BALL Sparking! ZERO\\SparkingZERO\\Content\\Paks\"\n"

// Initialize config with default values
void config_init(Config* config) {
    config->Convert_HCA_Into_WAV = false;
    strcpy(config->Game_Directory,
           "C:\\Program Files (x86)\\Steam\\steamapps\\common\\DRAGON BALL Sparking! ZERO\\SparkingZERO\\Content\\Paks");
}

// Helper functions
static void trim(char* str) {
    char* start = str;
    char* end = str + strlen(str) - 1;
    while(isspace((unsigned char)*start)) start++;
    while(end > start && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    memmove(str, start, end - start + 2);
}

static void remove_quotes(char* str) {
    size_t len = strlen(str);
    if (len >= 2 && str[0] == '"' && str[len-1] == '"') {
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
    }
}

// Parse a single config line
static void parse_line(Config* config, char* line) {
    char* key = strtok(line, "=");
    char* value = strtok(NULL, "\n");
    if (!key || !value) return;

    trim(key);
    trim(value);
    remove_quotes(value);

    char key_lower[256] = {0};
    strcpy(key_lower, key);
    strlwr(key_lower);

    if (strcmp(key_lower, "convert_hca_into_wav") == 0) {
        config->Convert_HCA_Into_WAV = (_stricmp(value, "true") == 0);
    }
    else if (strcmp(key_lower, "game_directory") == 0) {
        strcpy(config->Game_Directory, value);
    }
}

// Load or create config file
bool config_load(Config* config, const char* filename) {
    config_init(config);  // Initialize with defaults

    FILE* file = fopen(filename, "r");
    if (!file) {
        // Create default config file if it doesn't exist
        file = fopen(filename, "w");
        if (!file) return false;
        fprintf(file, "%s", DEFAULT_CONFIG_CONTENT);
        fclose(file);
        return true;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        parse_line(config, line);
    }
    fclose(file);
    return true;
}

// Helper function for quoted paths
const char* config_get_quoted_path(const char* path, char* buffer, size_t buffer_size) {
    if (path[0] == '"') {
        strncpy(buffer, path, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    } else {
        snprintf(buffer, buffer_size, "\"%s\"", path);
    }
    return buffer;
}
