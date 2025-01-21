#include "hcakey_generator.h"
#include <stdio.h>
#include <sys/stat.h>

// Writes a 64-bit key to .hcakey file in the specified folder
static void write_binary_key(uint64_t key, const char* folder) {
    char hcakey_path[MAX_PATH];
    snprintf(hcakey_path, sizeof(hcakey_path), "%s\\.hcakey", folder);

    FILE* file = fopen(hcakey_path, "wb");
    if (!file) {
        perror("Error creating .hcakey file");
        return;
    }

    // Write key in big-endian format
    for (int i = 7; i >= 0; i--) {
        uint8_t byte = (key >> (i * 8)) & 0xFF;
        fwrite(&byte, 1, 1, file);
    }
    fclose(file);
}

// Generates .hcakey in a folder named after the input file
void generate_hcakey(const char* filepath) {
    const char* filename = get_basename(filepath);
    const char* directory = get_parent_directory(filepath);

    // Create folder name from input file (without extension)
    char folder_name[MAX_PATH];
    snprintf(folder_name, sizeof(folder_name), "%s\\%.*s",
             directory,
             (int)(strrchr(filename, '.') - filename),
             filename);

    // Generate .hcakey in the new folder
    generate_hcakey_dir(filepath, folder_name);
}

// Generates .hcakey in the specified directory
void generate_hcakey_dir(const char* filepath, const char* directory) {
    // Extract key from the file
    uint64_t key = get_key(filepath);
    if (key == (uint64_t)-1) {
        fprintf(stderr, "Failed to extract key for: %s\n", filepath);
        return;
    }

    // Create output directory if it doesn't exist
    mkdir(directory);

    // Write the key to .hcakey
    write_binary_key(key, directory);
    printf("Generated .hcakey for %s\n", get_basename(filepath));
}

static const uint64_t MAIN_KEY = 13238534807163085345ULL;

uint64_t get_key(const char *filepath) {
    FILE *file = fopen(replace_extension(filepath,"awb"), "rb");
    if (!file) return -1;

    // Verify AFS2 header
    unsigned char header[4];
    if (fread(header, 1, 4, file) != 4) {
        fclose(file);
        return -1;
    }

    if (header[0] != 0x41 || header[1] != 0x46 ||
        header[2] != 0x53 || header[3] != 0x32) {
        fclose(file);
        return -1;
    }

    // Read AwbHash from 0x0E
    if (fseek(file, 0x0E, SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }

    unsigned char awb_hash_bytes[2];
    if (fread(awb_hash_bytes, 1, 2, file) != 2) {
        fclose(file);
        return -1;
    }

    fclose(file);

    // Convert to little-endian ushort
    uint16_t awb_hash = (uint16_t)(awb_hash_bytes[1] << 8) | awb_hash_bytes[0];

    // Calculate key
    return MAIN_KEY * (
        ((uint64_t)awb_hash << 16) |
        (uint16_t)(~awb_hash + 2)
    );
}
