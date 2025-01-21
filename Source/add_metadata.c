#include "add_metadata.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>

// When there's a link between Cue Name and genre, I'll update this
const char* get_genre(const char* filename) {
	if (strstr(filename, "BTLCV") != NULL) {
		return "Battle";
	} else if (strstr(filename, "bgm") != NULL) {
		return "BGM";
	} else if (strstr(filename, "BTLSE") != NULL) {
		return "Sound Effects";
	} else if (strstr(filename, "ADVIF") != NULL) {
		return "Story";
	} else {
		return extract_name_from_path(filename);
	}
}

void rename_files_back(const char* foldername) {
	int is_bgm = (strstr(foldername, "BGM") != NULL
	              || strstr(foldername, "bgm") != NULL );
	DIR* dir;
	struct dirent* ent;

	if ((dir = opendir(foldername)) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			const char* filename = ent->d_name;

			// Match all files starting with "Cue_" (both formats)
			if (strncmp(filename, "Cue_", 4) == 0 && strstr(filename, ".hca")) {
				int original_num;
				// Extract number from both formats:
				// "Cue_123 - Name.ext" and "Cue_123.ext"
				if (sscanf(filename + 4, "%d", &original_num) != 1) {
					continue;  // Skip invalid format
				}

				// Determine new name
				char new_name[MAX_PATH];
				if (is_bgm) {
					snprintf(new_name, sizeof(new_name), "%d.hca", original_num);
				} else {
					snprintf(new_name, sizeof(new_name), "%05d_streaming.hca", original_num);
				}

				// Delete existing file with target name first
				char target_path[MAX_PATH];
				snprintf(target_path, sizeof(target_path), "%s\\%s", foldername, new_name);
				if (access(target_path, F_OK) == 0) {
					remove(target_path);
				}

				// Rename the file
				char old_path[MAX_PATH];
				snprintf(old_path, sizeof(old_path), "%s\\%s", foldername, filename);
				rename(old_path, target_path);
			}
		}
		closedir(dir);
	}
}

int add_metadata(const char* input_file) {
	char awb_path[MAX_PATH];

	// Acbs would show all their cues, has to be the awb file
	strcpy(awb_path, replace_extension(input_file, "awb"));

	int is_bgm = 0;
	int deduct = 0;
	if (strstr(input_file, "bgm_") != NULL) {
		is_bgm = 1;
		if (strstr(input_file, "Cnk_00"))
			deduct = 82;
		else if (strstr(input_file, "DLC_01"))
			deduct = 82 + 38;
		else if (strstr(input_file, "DLC_02"))
			deduct = 82 + 38 + 13;
	}

	// Get folder path
	char folder_path[MAX_PATH];
	strcpy(folder_path, get_parent_directory(input_file));
	strcat(folder_path, "\\");
	strcat(folder_path, get_basename(input_file));

	printf("Getting file metadata for %s\n", extract_name_from_path(awb_path));
	StreamData streamData;
	if (run_vgmstream(awb_path, &streamData) != 0) {
		fprintf(stderr, "Error running or parsing vgmstream output.\n");
		free(streamData.records);
		streamData.records = NULL;
		return 1;
	}

	// Create path for metadata batch file
	char metadata_batch_path[MAX_PATH];
	snprintf(metadata_batch_path, sizeof(metadata_batch_path),
	         "%s\\add_metadata.bat", folder_path);

	// Create a batch file for adding metadata
	FILE* metadata_batch_file = fopen(metadata_batch_path, "w");
	if (!metadata_batch_file) {
		perror("Error creating metadata batch file");
		free(streamData.records);
		streamData.records = NULL;
		return 1;
	}

	// Write batch file header
	fprintf(metadata_batch_file, "@echo off\n");
	fprintf(metadata_batch_file, "echo Adding metadata to WAV files...\n");

	// Iterate through files in the folder
	DIR* dir;
	struct dirent* ent;
	if ((dir = opendir(folder_path)) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			char* filename = ent->d_name;
			// Check if filename matches the pattern (looking for .hca files, conditionally checking for _streaming)
			if ((is_bgm || strstr(filename, "_streaming") != NULL) &&
			        strstr(filename, ".hca") != NULL && isdigit(*filename)) {

				// Metadata is for .wav files since .hcas are structured differently
				char wav_file_path[MAX_PATH];
				snprintf(wav_file_path, sizeof(wav_file_path), "%s\\%s", folder_path,
				         filename);
				strcpy(wav_file_path, replace_extension(wav_file_path, "wav"));

				int fileindex;

				// Extract index from filename
				char* endptr = filename;
				int original_num = strtol(filename, &endptr, 10);
				fileindex = original_num - deduct;

				if (endptr == filename) {
					continue;
				}

				// Find corresponding record in streamData using index
				if (fileindex >= streamData.num_records)
					continue;

				const char* genre = get_genre(awb_path);

				// Write the command to the batch file
				fprintf(metadata_batch_file,
				        "\"%s\" \"%s\" \"Cue: %s\" \"%s\" \"CueID: %s\" \"%s\" \"%d\"\n",
				        metadata_tool_path, wav_file_path, // Now with .wav extension
				        streamData.records[fileindex].stream_name, extract_name_from_path(awb_path),
				        streamData.records[fileindex].cue_id, genre, fileindex + 1);

				// Add rename command if Use_Cue_Names is enabled
				if (config.Use_Cue_Names) {
					const char* cue_name = streamData.records[fileindex].stream_name;
					if (!cue_name || strlen(cue_name) == 0) {
						cue_name = "null";
					}
					char sanitized_name[MAX_PATH] = {0};
					sanitize_filename(cue_name, sanitized_name);

					// Generate new name
					char new_name[MAX_PATH];
					snprintf(new_name, sizeof(new_name), "Cue_%d - %s.wav", original_num,
					         sanitized_name);

					// Write rename command to batch file
					fprintf(metadata_batch_file, "ren \"%s\" \"%s\"\n", wav_file_path, new_name);
				}
			}
		}
		closedir(dir);
	} else {
		fprintf(stderr, "Error: Could not open directory: %s\n", folder_path);
		free(streamData.records);
		streamData.records = NULL;
		return 1;
	}

	// Add completion message and cleanup to the batch file
	fprintf(metadata_batch_file, "echo Metadata addition complete!\n");
	fprintf(metadata_batch_file, "del \"%s\"\n", metadata_batch_path);
	fclose(metadata_batch_file);

	free(streamData.records);
	streamData.records = NULL;
	return 0;
}

void sanitize_filename(const char* input, char* output) {
	for (int i = 0; input[i] != '\0' && i < MAX_PATH - 1; i++) {
		char c = input[i];
		if (strchr("\\/:*?\"<>|", c)) {
			output[i] = '_';
		} else {
			output[i] = c;
		}
	}
	output[MAX_PATH - 1] = '\0';
}

int rename_hcas(const char* input_file) {
	char awb_path[MAX_PATH];
	strcpy(awb_path, replace_extension(input_file, "awb"));

	int deduct = 0;
	int is_bgm = 0;
	if (strstr(input_file, "bgm_") != NULL) {
		is_bgm = 1;
		if (strstr(input_file, "Cnk_00"))
			deduct = 82;
		else if (strstr(input_file, "DLC_01"))
			deduct = 82 + 38;
		else if (strstr(input_file, "DLC_02"))
			deduct = 82 + 38 + 13;
	}

	// Get folder path
	char folder_path[MAX_PATH];
	strcpy(folder_path, get_parent_directory(input_file));
	strcat(folder_path, "\\");
	strcat(folder_path, get_basename(input_file));

	printf("Getting file metadata for %s\n", extract_name_from_path(awb_path));
	StreamData streamData;
	if (run_vgmstream(awb_path, &streamData) != 0) {
		fprintf(stderr, "Error running or parsing vgmstream output.\n");
		free(streamData.records);
		streamData.records = NULL;
		return 1;
	}

	// Create path for rename batch file
	char rename_batch_path[MAX_PATH];
	snprintf(rename_batch_path, sizeof(rename_batch_path), "%s\\rename_hcas.bat",
	         folder_path);

	// Create a batch file for renaming HCAs
	FILE* batch_file = fopen(rename_batch_path, "w");
	if (!batch_file) {
		perror("Error creating rename batch file");
		free(streamData.records);
		streamData.records = NULL;
		return 1;
	}

	// Write batch file header
	fprintf(batch_file, "@echo off\n");
	fprintf(batch_file, "echo Renaming HCA files...\n");

	// Iterate through files in the folder
	DIR* dir;
	struct dirent* ent;
	if ((dir = opendir(folder_path)) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			char* filename = ent->d_name;
			if ((is_bgm || strstr(filename, "_streaming") != NULL)
			        && strstr(filename, ".hca") != NULL && isdigit(*filename)) {
				// Extract index from filename
				char* endptr = filename;
				int original_num = strtol(filename, &endptr, 10);
				int fileindex = original_num - deduct;

				if (endptr == filename || fileindex >= streamData.num_records) {
					continue;
				}

				const char* cue_name = streamData.records[fileindex].stream_name;
				if (!cue_name || strlen(cue_name) == 0) {
					cue_name = "null";
				}
				char sanitized_name[MAX_PATH] = {0};
				sanitize_filename(cue_name, sanitized_name);

				// Write rename command to batch file
				fprintf(batch_file, "ren \"%s\\%s\" \"Cue_%d - %s.hca\"\n", folder_path,
				        filename, original_num,
				        sanitized_name);
			}
		}
		closedir(dir);
	} else {
		fprintf(stderr, "Error: Could not open directory: %s\n",
		        extract_name_from_path(folder_path));
		free(streamData.records);
		streamData.records = NULL;
		return 1;
	}

	// Add completion message and cleanup to the batch file
	fprintf(batch_file, "echo HCA renaming complete!\n");
	fprintf(batch_file, "del \"%s\"\n", rename_batch_path);
	fclose(batch_file);

	free(streamData.records);
	streamData.records = NULL;
	return 0;
}
