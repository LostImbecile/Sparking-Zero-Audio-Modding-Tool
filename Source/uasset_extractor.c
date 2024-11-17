#include "uasset_extractor.h"
#include <string.h>

int process_uasset(const char* uasset_path) {
    char* output_path = replace_extension(uasset_path, "acb");

    FILE* input = fopen(uasset_path, "rb");
    if (!input) {
        perror("Failed to open input file");
        return 1;
    }

    long utf_pos = find_utf_marker(input);
    if (utf_pos == -1) {
        printf("No @UTF found in %s\n", extract_name_from_path(uasset_path));
        fclose(input);
        return 1;
    }

    fseek(input, utf_pos, SEEK_SET);
    FILE* output = fopen(output_path, "wb");
    if (!output) {
        perror("Failed to open output file");
        fclose(input);
        return 1;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, input)) > 0) {
        fwrite(buffer, 1, bytes_read, output);
    }

    fclose(input);
    fclose(output);
    printf("Extracted %s from .uasset file.\n", extract_name_from_path(output_path));
    return 0;
}

long find_utf_marker(FILE *fp) {
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    const char *marker = "@UTF";
    long position = -1;

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
        for (size_t i = 0; i < bytes_read - 3; ++i) {
            if (memcmp(&buffer[i], marker, 4) == 0) {
                position = ftell(fp) - bytes_read + i;
                return position;
            }
        }
    }
    return -1;  // Not found
}
