#include "track_info_utils.h"

// Generate a .txtm file for when the acb manages more than one awb
int generate_txtm(const char* inputfile) {
	const char* txtm_filename = ".txtm";
	FILE* txtm_file = fopen(txtm_filename, "w");
	if (txtm_file == NULL) {
		fprintf(stderr, "Error opening .txtm file for writing\n");
		return -1;
	}

	// Example content (modify as needed)
	if (strstr(inputfile, "bgm_main") != NULL) {
		fprintf(txtm_file, "bgm_main.awb: bgm_main.acb\n");
		fprintf(txtm_file, "bgm_main_Cnk_00.awb: bgm_main.acb\n");
	} else if (strstr(inputfile, "ADVIF_CV_0000_JP") != NULL) {
		fprintf(txtm_file, "ADVIF_CV_0000_JP.awb: ADVIF_CV_0000_JP.acb\n");
		fprintf(txtm_file, "ADVIF_CV_0000_JP_Cnk_00.awb: ADVIF_CV_0000_JP.acb\n");
	} else if (strstr(inputfile, "ADVIF_CV_0000_US") != NULL) {
		fprintf(txtm_file, "ADVIF_CV_0000_US.awb: ADVIF_CV_0000_US.acb\n");
		fprintf(txtm_file, "ADVIF_CV_0000_US_Cnk_00.awb: ADVIF_CV_0000_US.acb\n");
	}

	fclose(txtm_file);
	return 0;
}

// Runs vgmstream to get metadata info on awb+acb pairs
int run_vgmstream(const char* input_file, StreamData* data) {
	char command[MAX_PATH * 8];
	char temp_output_filename[MAX_PATH];

	// Create a temporary filename
	snprintf(temp_output_filename, sizeof(temp_output_filename),
	         "track_info.txt");

	// Construct command
	int command_length = snprintf(command, sizeof(command),
	                              "\"\"%s\" -m -S 0 -i \"%s\" > \"%s\"\"", vgmstream_path, input_file,
	                              temp_output_filename);
	if (command_length < 0 || command_length >= sizeof(command)) {
		fprintf(stderr,
		        "Error: vgmstream command construction failed or too long.\n");
		remove(".txtm");
		return -1;
	}

	// Execute command
	int result = system(command);
	if (result != 0) {
		fprintf(stderr, "Error running vgmstream command. Return code: %d\n", result);
		remove(".txtm");
		return -1;
	}

	// Open output file
	FILE* output_file = fopen(temp_output_filename, "r");
	if (output_file == NULL) {
		fprintf(stderr, "Error opening vgmstream output file\n");
		remove(".txtm");
		return -1;
	}

	// Parse output
	result = parse_vgmstream_output(output_file, data);

	// Cleanup
	fclose(output_file);
	if (remove(temp_output_filename) != 0) {
		fprintf(stderr, "Warning: Error deleting temporary file.\n");
	}

	remove(".txtm");
	return result;
}

// Function to parse vgmstream output
int parse_vgmstream_output(FILE* output_file, StreamData* data) {
	char line[1024];
	int stream_count_found = 0;
	uint16_t stream_count = 0;

	data->num_records = 0;
	data->records = NULL;

	while (fgets(line, sizeof(line), output_file) != NULL) {
		// Parse "stream count" (only once)
		if (!stream_count_found && strstr(line, "stream count:") == line) {
			if (sscanf(line, "stream count: %hu", &stream_count) == 1) {
				stream_count_found = 1;

				// Allocate memory for records based on stream count + 1 for null terminator
				data->records = (StreamInfo*)malloc((stream_count + 1) * sizeof(StreamInfo));
				if (data->records == NULL) {
					fprintf(stderr, "Error allocating memory for records\n");
					return -1;
				}
				data->num_records = stream_count;

				// Initialize all allocated records to 0
				memset(data->records, 0, (stream_count + 1) * sizeof(StreamInfo));

				// Add a null pointer at the end
				data->records[stream_count].stream_name[0] = '\0';
				data->records[stream_count].cue_id[0] = '\0';
			} else {
				fprintf(stderr, "Error parsing stream count.\n");
			}
		}
		// Parse "stream name" and "cue id"
		else if (stream_count_found && data->num_records > 0) {
			int current_record = 0;
			for (int i = 0; i < data->num_records; i++) {
				if (strlen(data->records[i].cue_id) == 0) {
					current_record = i;
					break;
				}
			}

			char temp_stream_name[100];
			if (sscanf(line, "stream name: %[^\n]", temp_stream_name) == 1) {
				// Found stream name, now look for cue id
				while (fgets(line, sizeof(line), output_file) != NULL) {
					if (sscanf(line, "cue id: %[^\n]",
					           data->records[current_record].cue_id) == 1) {
						// Copy the stream name after finding the cue id
						strncpy(data->records[current_record].stream_name, temp_stream_name,
						        sizeof(data->records[current_record].stream_name) - 1);
						data->records[current_record].stream_name[sizeof(
						            data->records[current_record].stream_name) - 1] =
						                '\0'; // Ensure null-termination
						break; // Exit cue id search loop
					}
				}
			}
		}
	}

	// Check if stream count was found and records were allocated
	if (!stream_count_found || data->records == NULL) {
		fprintf(stderr,
		        "Error: Stream count not found or memory allocation failed.\n");
		return -1; // Indicate an error if stream count was not found
	}

	return 0;
}
