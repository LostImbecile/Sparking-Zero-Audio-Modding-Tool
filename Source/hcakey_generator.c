#include "hcakey_generator.h"
#include "utils.h"
#include <ctype.h>
#include <sys/stat.h>

KeyEntry keyEntries[MAX_ENTRIES];
int numEntries = 0;

int read_csv(const char* full_path)
{
	FILE* file = fopen(full_path, "r");
	if (!file)
	{
		return 0;
	}

	char line[MAX_FILENAME + MAX_KEY_LENGTH];
	while (fgets(line, sizeof(line), file) && numEntries < MAX_ENTRIES)
	{
		char* filename = strtok(line, ",");
		char* key = strtok(NULL, "\n");
		if (filename && key)
		{
			remove_suffix(filename);
			strncpy(keyEntries[numEntries].filename, filename, MAX_FILENAME - 1);
			strncpy(keyEntries[numEntries].key, key, MAX_KEY_LENGTH - 1);
			numEntries++;
		}
	}
	fclose(file);
	return 1;
}

void remove_suffix(char* filename)
{
	char* underscore = strrchr(filename, '_');
	if (underscore)
	{
		if (strlen(underscore) <= 3 && (isupper(underscore[1])
		                                || isdigit(underscore[1])))
		{
			*underscore = '\0';
		}
		else if (strlen(underscore) > 3 && underscore[1] == '*'
		         && isupper(underscore[2]))
		{
			*underscore = '\0';
		}
	}
}

const char* find_key(const char* filename)
{
    char base_filename[MAX_FILENAME];
    strncpy(base_filename, filename, MAX_FILENAME - 1);
    base_filename[MAX_FILENAME - 1] = '\0';

    // Remove extension
    char* dot = strrchr(base_filename, '.');
    if (dot) *dot = '\0';

    remove_suffix(base_filename);

    for (int i = 0; i < numEntries; i++)
    {
        char csv_filename[MAX_FILENAME];
        strncpy(csv_filename, keyEntries[i].filename, MAX_FILENAME - 1);
        csv_filename[MAX_FILENAME - 1] = '\0';

        // Use strstr instead of strcmp to check if csv_filename is contained within base_filename
        if (strstr(base_filename, csv_filename) != NULL)
        {
            return keyEntries[i].key;
        }
    }
    return NULL;
}

void write_binary_key(const char* key, const char* folder)
{
	char hcakey_path[MAX_PATH];
	snprintf(hcakey_path, sizeof(hcakey_path), "%s/.hcakey", folder);

	FILE* file = fopen(hcakey_path, "wb");
	if (!file)
	{
		perror("Error creating .hcakey file");
		return;
	}

	uint64_t binary_key = strtoull(key, NULL, 10);

	for (int i = 7; i >= 0; i--)
	{
		uint8_t byte = (binary_key >> (i * 8)) & 0xFF;
		fwrite(&byte, 1, 1, file);
	}

	fclose(file);
}

void generate_hcakey(const char* filepath)
{
	if (!csv_loaded)
	{
		fprintf(stderr, "\"keys.csv\" not found. Cannot generate .hcakey.\n");
		return; // Stop if the CSV couldn't be loaded.
	}

	const char* filename = filepath;
	filename = get_basename(filename);

	const char* key = find_key(filename);

	if (key)
	{
		char* directory = get_parent_directory(filepath);
		char folder_name[MAX_PATH];
		snprintf(folder_name, sizeof(folder_name), "%s\\%.*s", directory,
		         (int)(strrchr(filename, '.') - filename), filename);

		mkdir(folder_name);

		write_binary_key(key, folder_name);
		printf("Generated .hcakey file for %s in Folder: %s\n", filename,
		       extract_name_from_path(folder_name));
		free(directory);
	}
	else
	{
		fprintf(stderr, "No key found for %s\n", filename);
	}
}

