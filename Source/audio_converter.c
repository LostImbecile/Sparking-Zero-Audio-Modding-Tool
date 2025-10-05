#include "audio_converter.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>

uint64_t extract_hca_key(const char* folder) {
	char hcakey_path[MAX_PATH];
	snprintf(hcakey_path, sizeof(hcakey_path), "%s/.hcakey", folder);
	FILE* file = fopen(hcakey_path, "rb");
	if (!file) {
		perror("Error opening .hcakey file");
		return 0;
	}

	uint64_t key = 0;
	uint8_t byte;
	// Read bytes in reverse order
	for (int i = 7; i >= 0; i--) {
		if (fread(&byte, 1, 1, file) != 1) {
			perror("Error reading from .hcakey file");
			fclose(file);
			return 0;
		}
		key |= ((uint64_t)byte << (i * 8));
	}
	fclose(file);
	return key;
}

int encrypt_hcas(const char* folder, uint64_t hcakey) {
    DIR* dir = opendir(folder);
    if (!dir) {
        return -1;
    }

    printf("Checking if any HCA is in need of encryption...\n");

    struct dirent* entry;
    char input_path[MAX_PATH];
    char output_path[MAX_PATH];
    char command[MAX_PATH * 8];
    char output[1024] = { 0 };
    int success = 0;
    FILE* output_file;

    while ((entry = readdir(dir)) != NULL) {
        const char* ext = get_file_extension(entry->d_name);
        if (ext != NULL && strcasecmp(ext, "hca") == 0) {
            // Construct full input path
            snprintf(input_path, sizeof(input_path), "%s\\%s", folder, entry->d_name);

            // Check if HCA is encrypted by reading the first 3 bytes
            FILE* hca_file = fopen(input_path, "rb");
            if (!hca_file) {
                continue;
            }

            char header[3];
            fread(header, 1, 3, hca_file);
            fclose(hca_file);

            if (memcmp(header, "HCA", 3) != 0) {
                // HCA is likely already encrypted, skip it
                continue;
            }

            // Construct path for the corresponding .wav file using your replace_extension
            const char* wav_path = replace_extension(input_path, "wav");
            if (!wav_path) {
                continue;
            }

            // Check if the .wav file exists using your is_path_exists
            if (is_path_exists(wav_path)) {
                continue;
            }

            // Construct full output path (no change here)
            snprintf(output_path, sizeof(output_path), "%s\\%s", folder, entry->d_name);

            // Create a temporary file for output
            char temp_output_path[MAX_PATH];
            snprintf(temp_output_path, sizeof(temp_output_path), "%s\\temp_output_%d.txt",
                folder, rand());

            // Construct command with output redirected to temp file
            snprintf(command, sizeof(command),
                "\"\"%s\" -i \"%s\" \"%s\" --keycode %" PRIu64 " >%s 2>&1\"",
                app_data.vgaudio_cli_path, input_path, output_path, hcakey, temp_output_path);

            // Execute command
            system(command);

            // Check output file
            output_file = fopen(temp_output_path, "r");
            if (output_file) {
                if (fgets(output, sizeof(output), output_file) != NULL) {
                   success++;
                }
                fclose(output_file);
            }

            // Remove temporary output file
            remove(temp_output_path);
        }
    }

    closedir(dir);
    return success;
}

int process_wav_files(const char* folder, uint64_t hca_key,
                      int set_looping_points) {

	if (set_looping_points)
		printf("Checking if any WAVs need conversion...\n");
	DIR* dir = opendir(folder);
	if (!dir) {
		perror("Error opening directory");
		return -1;
	}

	// Create a batch file for all conversions
	char batch_path[MAX_PATH];
	snprintf(batch_path, sizeof(batch_path), "%s\\convert_wav.bat", folder);
	FILE* batch_file = fopen(batch_path, "w");
	if (!batch_file) {
		perror("Error creating batch file");
		closedir(dir);
		return -1;
	}


	// Write batch file header
	fprintf(batch_file, "@chcp 65001 >nul\n@echo off\n");
	fprintf(batch_file, "echo Starting WAV to HCA conversion...\n");

	struct dirent* entry;
	char wav_path[MAX_PATH];
	char hca_path[MAX_PATH];
	char temp_file[MAX_PATH];
	char command[MAX_PATH * 8];
	int has_files = 0;

	while ((entry = readdir(dir)) != NULL) {
		const char* ext = get_file_extension(entry->d_name);
		if (ext != NULL && strcasecmp(ext, "wav") == 0) {
			has_files = 1;
			// Construct full paths
			snprintf(wav_path, sizeof(wav_path), "%s\\%s", folder, entry->d_name);
			const char* basename = get_basename(entry->d_name);
			snprintf(hca_path, sizeof(hca_path), "%s\\%s.hca", folder, basename);
			if (set_looping_points) {
				snprintf(temp_file, sizeof(temp_file), "%s\\temp_samples.txt", folder);

				// Get total samples and sample rate using vgmstream
				snprintf(command, sizeof(command), "\"\"%s\" -m \"%s\" > \"%s\"\"",
				         app_data.vgmstream_path, wav_path, temp_file);
				system(command);
				int samples = 0;
				int sample_rate = 0;
				FILE* temp = fopen(temp_file, "r");
				if (temp) {
					char buffer[1024] = {0};
					while (fgets(buffer, sizeof(buffer), temp)) {
						if (strstr(buffer, "sample rate:") != NULL) {
							char* sample_rate_str = strstr(buffer, "sample rate:") + 12;
							sample_rate = atoi(sample_rate_str);

						} else if (strstr(buffer, "stream total samples:") != NULL) {
							char* samples_str = strstr(buffer, "samples:") + 8;
							samples = atoi(samples_str);
							break; // No need to continue if we have both values
						}
					}
					fclose(temp);
					remove(temp_file);

					if (sample_rate != 48000) {
						fprintf(stderr,
						        "Warning: File '%s' has a different sampling rate: %dHz, 48KHz is preferred\n",
						        extract_name_from_path(wav_path), sample_rate);
					}

					if (samples > 0) {
						fprintf(batch_file, "echo Converting %s to HCA (adding loop points 0-%d)\n",
						        basename, samples);
						fprintf(batch_file, "\"%s\" \"%s\" \"%s\" --keycode %" PRIu64
						        " --out-format hca -l 0-%d\n",
						        app_data.vgaudio_cli_path, wav_path, hca_path, hca_key, samples);
					}
				}
			} else {
				fprintf(batch_file, "echo Converting %s to HCA\n", basename);
				fprintf(batch_file, "\"%s\" \"%s\" \"%s\" --keycode %" PRIu64
				        " --out-format hca\n",
				        app_data.vgaudio_cli_path, wav_path, hca_path, hca_key);
			}
		}
	}

	// Add completion message and auto-exit
	fprintf(batch_file, "echo Conversion complete!\n");
	fprintf(batch_file, "del \"%s\\convert_wav.bat\"\n", folder);
	fprintf(batch_file, "exit\n");
	fclose(batch_file);
	closedir(dir);

	if (!has_files) {
		remove(batch_path);
		return 0;
	}
	if (set_looping_points) {
		printf("Note: Looping points were set from start to end for converted HCAs\n");
	}

	// Execute the batch file in a new window and wait for completion
	snprintf(command, sizeof(command),
	         "start \"WAV to HCA Conversion\" /wait cmd /C \"chcp 65001 >nul && \"%s\"\"",
	         batch_path);
	return system(command);
}

int process_hca_files(const char* folder) {
	DIR* dir = opendir(folder);
	if (!dir) {
		perror("Error opening directory");
		return -1;
	}

	// Create a batch file for all conversions
	char batch_path[MAX_PATH];
	snprintf(batch_path, sizeof(batch_path), "%s\\convert_hca.bat", folder);
	FILE* batch_file = fopen(batch_path, "w");
	if (!batch_file) {
		perror("Error creating batch file");
		closedir(dir);
		return -1;
	}

	// Write batch file header
	fprintf(batch_file, "@chcp 65001 >nul\n@echo off\n");
	fprintf(batch_file, "echo Starting HCA to WAV conversion...\n");

	struct dirent* entry;
	char hca_path[MAX_PATH];
	char wav_path[MAX_PATH];
	int has_files = 0;

	while ((entry = readdir(dir)) != NULL) {
		const char* ext = get_file_extension(entry->d_name);
		if (ext != NULL && strcasecmp(ext, "hca") == 0
		        && strcmp(entry->d_name, "00000.hca") != 0) {
			has_files = 1;
			// Construct full paths
			snprintf(hca_path, sizeof(hca_path), "%s\\%s", folder, entry->d_name);
			const char* basename = get_basename(entry->d_name);
			snprintf(wav_path, sizeof(wav_path), "%s\\%s.wav", folder, basename);

			// Write conversion and deletion logic to batch file
			fprintf(batch_file, "echo Converting %s.hca to WAV\n", basename);
			fprintf(batch_file, "\"%s\" -i \"%s\" -o \"%s\"\n", app_data.vgmstream_path, hca_path,
			        wav_path);

			// Check if conversion was successful and delete HCA if it was
			fprintf(batch_file, "if %%errorlevel%% equ 0 (\n");
			fprintf(batch_file, "    echo Conversion successful - deleting %s.hca\n",
			        basename);
			fprintf(batch_file, "    del \"%s\"\n", hca_path);
			fprintf(batch_file, ") else (\n");
			fprintf(batch_file,
			        "    echo Conversion failed for %s.hca - keeping original file\n", basename);
			fprintf(batch_file, ")\n");
		}
	}

	// Create path for metadata batch file
	char metadata_batch_path[MAX_PATH];
	snprintf(metadata_batch_path, sizeof(metadata_batch_path),
	         "%s\\add_metadata.bat", folder);

	// Add a line to run the metadata batch file after conversion
	fprintf(batch_file, "echo Conversion complete!\n");
	fprintf(batch_file, "if exist \"%s\" (\n", metadata_batch_path);
	fprintf(batch_file, "    call \"%s\"\n", metadata_batch_path);
	fprintf(batch_file, ")\n");
	fprintf(batch_file, "del \"%s\\convert_hca.bat\" && exit\n", folder);
	fclose(batch_file);
	closedir(dir);

	if (!has_files) {
		remove(batch_path);
		return 0;
	}

	// Execute the batch file in a new window
	char command[MAX_PATH * 8];
	snprintf(command, sizeof(command),
	         "start \"HCA to WAV Conversion\" cmd /C \"chcp 65001 >nul && \"%s\"\"",
	         batch_path);
	system(command);

	return 0;
}
