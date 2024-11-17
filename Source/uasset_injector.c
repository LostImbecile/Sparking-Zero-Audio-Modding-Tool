#include "uasset_injector.h"

int inject_process_file(const char* input_path) {
	char uasset_path[MAX_PATH];
	char acb_path[MAX_PATH];

	build_uasset_path(input_path, uasset_path, sizeof(uasset_path));
	build_acb_path(input_path, acb_path, sizeof(acb_path));

	struct stat buffer;
	if (stat(uasset_path, &buffer) != 0) {
		printf("%s does not exist, no .acb will be injected.\n",
		       extract_name_from_path(uasset_path));
		return -1;
	}

	create_backup(uasset_path);
	return inject_acb_content(uasset_path, acb_path);;
}

void create_backup(const char* file_path) {
    char backup_path[MAX_PATH];
    snprintf(backup_path, sizeof(backup_path), "%s.bak", file_path);

    // Check if the backup already exists
    FILE* check = fopen(backup_path, "rb");
    if (check) {
        fclose(check);
        printf("Backup already exists: %s\n", extract_name_from_path(backup_path));
        return;
    }

    FILE* src = fopen(file_path, "rb");
    FILE* dest = fopen(backup_path, "wb");

    if (!src || !dest) {
        perror("Failed to create backup");
        if (src) fclose(src);
        if (dest) fclose(dest);
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, src)) > 0) {
        fwrite(buffer, 1, bytes_read, dest);
    }

    fclose(src);
    fclose(dest);
    printf("Backup created: %s\n", extract_name_from_path(backup_path));
}

int inject_acb_content(const char* uasset_path, const char* acb_path) {
	FILE* uasset = fopen(uasset_path, "rb+");
	FILE* acb = fopen(acb_path, "rb");

	if (!uasset || !acb) {
		perror("Failed to open files");
		if (uasset) fclose(uasset);
		if (acb) fclose(acb);
		return -1;
	}

	// Get file sizes
	fseek(uasset, 0, SEEK_END);
	long uasset_size = ftell(uasset);
	fseek(acb, 0, SEEK_END);
	rewind(uasset);
	rewind(acb);

	// Find first @UTF marker
	char buffer[BUFFER_SIZE];
	long utf_pos = -1;
	size_t bytes_read = 0;
	const char* marker = "@UTF";

	while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, uasset)) > 0) {
		for (size_t i = 0; i < bytes_read - 3; ++i) {
			if (memcmp(&buffer[i], marker, 4) == 0) {
				utf_pos = ftell(uasset) - bytes_read + i;
				goto marker_found;
			}
		}
	}

marker_found:
	if (utf_pos == -1) {
		printf("No @UTF marker found in %s\n", extract_name_from_path(uasset_path));
		fclose(uasset);
		fclose(acb);
		return -1;
	}

	// Position file pointer at UTF marker
	fseek(uasset, utf_pos, SEEK_SET);

	// Copy ACB content
	rewind(acb);
	size_t total_written = 0;
	while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, acb)) > 0) {
		fwrite(buffer, 1, bytes_read, uasset);
		total_written += bytes_read;
	}

	// Zero out remaining space after ACB content
	const char zero_buffer[BUFFER_SIZE] = {0};
	long remaining_size = uasset_size - (utf_pos + total_written);

	while (remaining_size > 0) {
		size_t to_write = (remaining_size > BUFFER_SIZE) ? BUFFER_SIZE :
		                  remaining_size;
		fwrite(zero_buffer, 1, to_write, uasset);
		remaining_size -= to_write;
	}

	fclose(uasset);
	fclose(acb);
	printf("Successfully injected .acb into %s (replaced %lu bytes and zeroed %lu remaining bytes)\n",
	       extract_name_from_path(uasset_path),
	       (unsigned long)total_written,
	       (unsigned long)(uasset_size - (utf_pos + total_written)));

	return 0;
}

void build_acb_path(const char* input_path, char* output_path,
                    size_t max_len) {
	snprintf(output_path, max_len, "%s", input_path);
	char* dot = strrchr(output_path, '.');
	if (dot) *dot = '\0';
	strncat(output_path, ".acb", max_len - strlen(output_path) - 1);
}

void build_uasset_path(const char* input_path, char* output_path,
                       size_t max_len) {
	snprintf(output_path, max_len, "%s", input_path);
	char* dot = strrchr(output_path, '.');
	if (dot) *dot = '\0';
	strncat(output_path, ".uasset", max_len - strlen(output_path) - 1);
}
