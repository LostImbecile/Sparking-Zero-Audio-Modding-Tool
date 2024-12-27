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
	}
	// Get folder path
	char folder_path[MAX_PATH];
	strcpy(folder_path, get_parent_directory(input_file));
	strcat(folder_path, "\\");
	strcat(folder_path, get_basename(input_file));

	printf("Getting file metadata for %s\n",
	       extract_name_from_path(awb_path));
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
				fileindex = strtol(filename, &endptr, 10) - deduct;

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
