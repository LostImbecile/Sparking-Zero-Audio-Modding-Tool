#include "utils.h"
#include <dirent.h>
#include <libgen.h>
#include <errno.h>

void press_enter_to_exit() {
	printf("Press Enter to exit...");
	getchar();
}

void hex_to_bytes(const char* hex_str, uint8_t* bytes, size_t length) {
    for (size_t i = 0; i < length; i++) {
        char hex_byte[3] = {hex_str[i*2], hex_str[i*2 + 1], '\0'};
        int value;
        sscanf(hex_byte, "%x", &value);
        bytes[i] = (uint8_t)value;
    }
}

/**
 * @brief Checks if a path points to a directory
 *
 * @param path Path to check
 * @return true if path is a directory, false otherwise
 */
bool is_directory(const char* path) {
	struct stat path_stat;
	if (stat(path, &path_stat) == 0) {
		return S_ISDIR(path_stat.st_mode);
	}
	return false;
}

const char* get_parent_directory(const char* path) {
	char* path_copy = strdup(path);
	if (path_copy == NULL) {
		return "";
	}
	char* parent = dirname(path_copy);
	const char* result = strdup(parent);
	free(path_copy);
	return (result == NULL) ? "" : result;
}

const char* extract_name_from_path(const char* path) {
	const char* last_slash = strrchr(path, '/');
	const char* last_backslash = strrchr(path, '\\');
	const char* last_separator = (last_slash > last_backslash) ? last_slash : last_backslash;
	const char* name = last_separator ? last_separator + 1 : path;
	return name;
}

const char* get_file_extension(const char* filename) {
	const char* dot = strrchr(filename, '.');
	if (!dot || dot == filename) return "";
	return dot + 1;
}

int create_directory(const char* path) {

	if (strcmp(path, "C:") == 0 || strcmp(path, ".") == 0
	        || strcmp(path, "..") == 0)
		return 0;

	if (mkdir(path) != 0 && errno != EEXIST)
		return 1;

	return 0;
}

int remove_directory_recursive(const char* path) {
	DIR *dir = opendir(path);
	if (!dir) {
		return rmdir(path);
	}

	struct dirent *entry;
	char full_path[MAX_PATH];

	while ((entry = readdir(dir))) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		snprintf(full_path, MAX_PATH, "%s/%s", path, entry->d_name);

		struct stat statbuf;
		if (stat(full_path, &statbuf) == 0) {
			if (S_ISDIR(statbuf.st_mode)) {
				remove_directory_recursive(full_path);
			} else {
				remove(full_path);
			}
		}
	}

	closedir(dir);
	return rmdir(path);
}

int copy_file(const char* src_path, const char* dest_path) {
	FILE *source = fopen(src_path, "rb");
	if (!source) {
		printf("Failed to open source file: %s\n", src_path);
		return 1;
	}

	FILE *dest = fopen(dest_path, "wb");
	if (!dest) {
		fclose(source);
		printf("Failed to open destination file: %s\n", dest_path);
		return 1;
	}

	char buffer[8192];
	size_t bytes;
	while ((bytes = fread(buffer, 1, sizeof(buffer), source)) > 0) {
		if (fwrite(buffer, 1, bytes, dest) != bytes) {
			printf("Failed to write to destination file\n");
			fclose(source);
			fclose(dest);
			return 1;
		}
	}

	fclose(source);
	fclose(dest);
	return 0;
}

int create_directory_recursive(const char* path) {
	char tmp[MAX_PATH];
	char* p = NULL;
	size_t len;

	strncpy(tmp, path, MAX_PATH - 1);
	len = strlen(tmp);
	if (tmp[len - 1] == '\\')
		tmp[len - 1] = 0;

	for (p = tmp + 1; *p; p++) {
		if (*p == '\\') {
			*p = 0;
			if (create_directory(tmp) != 0) {
				printf("Failed to create directory: %s\n", tmp);
				return 1;
			}
			*p = '\\';
		}
	}
	if (create_directory(tmp) != 0) {
		printf("Failed to create directory: %s\n", tmp);
		return 1;
	}
	return 0;
}

/**
 * @brief Extracts the base name without extension from a file path
 *
 * @param path Full path to the file
 * @return const char* Newly allocated string containing the base name
 */
const char* get_basename(const char* path) {
    const char* last_slash = strrchr(path, '\\');
    const char* filename = last_slash ? last_slash + 1 : path;

    // Copy the filename portion
    char* filename_copy = strdup(filename);
    if (filename_copy == NULL) {
        return NULL; // Handle allocation failure
    }

    // Find the dot in the copy
    char* dot = strrchr(filename_copy, '.');
    if (dot) {
        *dot = '\0'; // Truncate the copy at the dot
    }

    return filename_copy;
}

const char* replace_extension(const char* filename, const char* new_extension) {
	const char* dot = strrchr(filename, '.');
	if (!dot) {
		size_t filename_len = strlen(filename);
		size_t ext_len = strlen(new_extension);
		char* result = malloc(filename_len + ext_len + 2);
		if (result == NULL) {
			return "";
		}
		strcpy(result, filename);
		strcat(result, ".");
		strcat(result, new_extension);
		return result;
	}

	size_t len = dot - filename;
	size_t ext_len = strlen(new_extension);
	char* result = malloc(len + ext_len + 2);
	if (result == NULL) {
		return "";
	}
	strncpy(result, filename, len);
	result[len] = '\0';
	strcat(result, ".");
	strcat(result, new_extension);
	return result;
}
