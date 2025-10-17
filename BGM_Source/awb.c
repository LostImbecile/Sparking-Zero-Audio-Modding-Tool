#include "awb.h"
#include "bgm_data.h"
#include "utils.h"
#include <inttypes.h>

#define READ_BUFFER_SIZE (1024 * 1024) // 1MB buffer

int process_uasset_file(const char* uasset_path) {
	FILE* file = fopen(uasset_path, "rb");
	if (!file) {
		fprintf(stderr, "Error: Could not open UASSET/ACB file: %s\n", uasset_path);
		return 1;
	}

	UAssetHeader* headers = NULL;
	int header_count = 0;
	long current_pos = 0;
	size_t bytes_read;
	uint8_t buffer[READ_BUFFER_SIZE];

	// 1. Determine the corresponding AWB path
	char awb_name[MAX_PATH] = {0};
	int highest_port = -1;
	for (int i = 0; i < csv_data.acb_mapping_count; i++) {
		if (strcasecmp(extract_name_from_path(uasset_path),
		               csv_data.acb_mappings[i].acbName) == 0) {
			// Find the mapping with the highest port number (last port)
			if (csv_data.acb_mappings[i].portNo > highest_port) {
				highest_port = csv_data.acb_mappings[i].portNo;
				strcpy(awb_name, csv_data.acb_mappings[i].awbName);
			}
		}
	}

	// 2. Get the starting index for the last port of this ACB
	int add_to_index = get_file_index_start(awb_name);
	int port1_tracks = get_port1_track_count(extract_name_from_path(uasset_path));

	while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
		for (size_t i = 0; i <= bytes_read - sizeof(hca_signature); ++i) {
			if (memcmp(buffer + i, hca_signature, sizeof(hca_signature)) == 0) {
				headers = realloc(headers, (header_count + 1) * sizeof(UAssetHeader));
				if (!headers) {
					fclose(file);
					perror("realloc failed");
					return 1;
				}

				// Use the calculated starting index or deduct
				if (header_count >= port1_tracks) {
					headers[header_count].index = header_count - port1_tracks;
				} else {
					headers[header_count].index = header_count + add_to_index;
				}

				headers[header_count].offset = current_pos + i;

				long bytes_left_in_buffer = bytes_read - i;
				size_t bytes_to_copy = (bytes_left_in_buffer >= HCA_HEADER_SIZE) ?
				                       HCA_HEADER_SIZE : bytes_left_in_buffer;
				memcpy(headers[header_count].header, buffer + i, bytes_to_copy);

				if (bytes_to_copy < HCA_HEADER_SIZE) {
					size_t remaining_bytes = HCA_HEADER_SIZE - bytes_to_copy;
					fseek(file, current_pos + i + bytes_to_copy, SEEK_SET);
					if (fread(headers[header_count].header + bytes_to_copy, 1, remaining_bytes,
					          file) != remaining_bytes) {
						fclose(file);
						fprintf(stderr, "Error: Could not read complete header at offset %ld\n",
						        headers[header_count].offset);
						free(headers);
						return 1;
					}
					fseek(file, current_pos + bytes_read, SEEK_SET);
				}

				header_count++;
				i += sizeof(hca_signature) - 1; // Optimization
			}
		}
		current_pos += bytes_read;
	}

	fclose(file);

	if (header_count > 0) {
		char output_filename[MAX_PATH];
		snprintf(output_filename, sizeof(output_filename), "%s_uasset_headers.csv",
		         get_basename(uasset_path));
		write_header_csv(output_filename, (HCAHeader*)headers, header_count);
	}

	free(headers);
	return 0;
}

int extract_hca_files(const char* filename, HCAHeader* headers,
                      int header_count, int add_to_index) {
	// Create a directory with the basename of the file
	char dir_name[MAX_PATH];
	snprintf(dir_name, sizeof(dir_name), "%s", get_basename(filename));
	mkdir(dir_name);

	// Reopen the file to read and extract segments
	FILE* file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Error: Could not reopen AWB file: %s\n", filename);
		return false;
	}

	// Iterate over each header to extract HCA segments
	for (int i = 0; i < header_count; ++i) {
		long start_offset = headers[i].offset;
		long end_offset = (i + 1 < header_count) ? headers[i + 1].offset : 0;

		// Calculate segment size
		if (end_offset == 0) { // Last segment continues to EOF
			fseek(file, 0, SEEK_END);
			end_offset = ftell(file);
		}
		long segment_size = end_offset - start_offset;

		// Read segment data
		fseek(file, start_offset, SEEK_SET);
		uint8_t* segment_data = malloc(segment_size);
		if (!segment_data
		        || fread(segment_data, 1, segment_size, file) != segment_size) {
			fprintf(stderr, "Error: Could not read segment %d\n", i);
			free(segment_data);
			fclose(file);
			return false;
		}

		// Write the segment as "index.hca" in the directory
		char hca_filename[MAX_PATH];
		snprintf(hca_filename, sizeof(hca_filename), "%s\\%d.hca", dir_name,
		         i + add_to_index);
		FILE* hca_file = fopen(hca_filename, "wb");
		if (!hca_file) {
			fprintf(stderr, "Error: Could not create HCA file: %s\n", hca_filename);
			free(segment_data);
			fclose(file);
			return false;
		}
		fwrite(segment_data, 1, segment_size, hca_file);
		fclose(hca_file);
		free(segment_data);
	}

	fclose(file);
	return true;
}

extern int extract_flag;
int process_awb_file(const char* filename) {
	FILE* file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Error: Could not open AWB file: %s\n", filename);
		return false;
	}

	int add_to_index = get_file_index_start(filename);
	if (add_to_index == -1) {
		fprintf(stderr, "Warning: %s is not a recognised BGM file.\n",
		        extract_name_from_path(filename));
		add_to_index = 0;
	}

	uint8_t buffer[READ_BUFFER_SIZE];
	HCAHeader* headers = NULL;
	int header_count = 0;
	long current_pos = 0;
	size_t bytes_read;

	while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
		for (size_t i = 0; i <= bytes_read - sizeof(hca_signature); ++i) {
			if (memcmp(buffer + i, hca_signature, sizeof(hca_signature)) == 0) {
				headers = realloc(headers, (header_count + 1) * sizeof(HCAHeader));
				if (!headers) {
					fclose(file);
					perror("realloc failed"); // Use perror for more detailed error info
					return false;
				}

				headers[header_count].index = header_count + add_to_index; // Set the index
				headers[header_count].offset = current_pos + i;

				// Handle partial header reads (same as before):
				long bytes_left_in_buffer = bytes_read - i;
				size_t bytes_to_copy = (bytes_left_in_buffer >= HCA_HEADER_SIZE) ?
				                       HCA_HEADER_SIZE : bytes_left_in_buffer;
				memcpy(headers[header_count].header, buffer + i, bytes_to_copy);

				if (bytes_to_copy < HCA_HEADER_SIZE) {
					size_t remaining_bytes = HCA_HEADER_SIZE - bytes_to_copy;
					fseek(file, current_pos + i + bytes_to_copy, SEEK_SET);
					if (fread(headers[header_count].header + bytes_to_copy, 1, remaining_bytes,
					          file) != remaining_bytes) {
						fclose(file);
						fprintf(stderr, "Error: Could not read complete header at offset %ld\n",
						        headers[header_count].offset);
						free(headers);
						return false;
					}
					fseek(file, current_pos + bytes_read, SEEK_SET);
				}

				header_count++;
				i += sizeof(hca_signature) - 1;
			}
		}
		current_pos += bytes_read;
	}


	fclose(file);


	// Write headers to CSV
	if (header_count > 0) {
		char output_filename[MAX_PATH];
		snprintf(output_filename, sizeof(output_filename), "%s_headers.csv",
		         get_basename(filename));
		write_header_csv(output_filename, headers, header_count);
	}

	if (extract_flag) {
		if (!extract_hca_files(filename, headers, header_count, add_to_index)) {
			fprintf(stderr, "Error: HCA extraction failed\n");
		}
	}

	free(headers);
	return 0;
}

int write_header_csv(const char* filename, HCAHeader* headers, int count) {
	char header_file[MAX_PATH];
	snprintf(header_file, sizeof(header_file), "%s%s", app_config.program_directory,
	         extract_name_from_path(filename));
	FILE* file = fopen(header_file, "wb");
	if (!file) {
		return -1;
	}

	// Write header line (as bytes)
	fwrite("Index,Offset,Header\n", 1, strlen("Index,Offset,Header\n"), file);

	for (int i = 0; i < count; i++) {
		char line_buffer[256]; // Increased buffer to handle potential larger indexes
		int bytes_written = snprintf(line_buffer, sizeof(line_buffer),
		                             "%d,0x%" PRIx64 ",", headers[i].index,
		                             (uint64_t)headers[i].offset);
		if (bytes_written < 0 || bytes_written >= sizeof(line_buffer)) {
			fprintf(stderr,
			        "Error: snprintf failed or truncated in write_header_csv at header %d.\n", i);
			fclose(file);
			return -1; // Indicate error
		}


		fwrite(line_buffer, 1, bytes_written, file);
		fwrite(headers[i].header, 1, HCA_HEADER_SIZE,
		       file); // Write header bytes directly
		fwrite("\n", 1, 1, file);

	}
	fclose(file);
	return 0;
}

// Helper function to read an index from a CSV file
long read_index(FILE* file) {
    char buffer[256];
    int i = 0;
    while (i < sizeof(buffer) - 1) {
        int c = fgetc(file);
        if (c == EOF || c == ',') {
            buffer[i] = '\0';
            if (c == EOF && i == 0) {
                // Correctly handle EOF before reading anything
                return -1; // Indicates EOF
            }
            char* endptr;
            errno = 0;
            long index = strtol(buffer, &endptr, 10);
            if (errno != 0 || *endptr != '\0' || index < INT_MIN || index > INT_MAX) {
                fprintf(stderr, "Error: Invalid index format.\n");
                return -2; // Error in parsing
            }
            if (c == ',') {
                // Comma is expected and handled, no need to ungetc
            }
            return index;
        } else {
            buffer[i++] = (char)c;
        }
    }
    fprintf(stderr, "Error: Index too long.\n");
    return -2; // Error
}

// Helper function to read an offset from a CSV file
long read_offset(FILE* file) {
    char buffer[256];
    int i = 0;

    // Read characters into the buffer until a comma, newline, or EOF is encountered
    while (i < sizeof(buffer) - 1) {
        int c = fgetc(file);
        if (c == EOF) {
            if (i == 0) {
                // If we hit EOF before reading anything, it's an error
                fprintf(stderr, "Error: Unexpected EOF while reading offset.\n");
                return -1;
            } else {
                // Otherwise, we've read something and it's the end of the file
                break;
            }
        } else if (c == ',' || c == '\n' || c == '\r') { // Handle CR, LF, or CRLF
            buffer[i] = '\0';
            if (strncmp(buffer, "0x", 2) != 0) {
                fprintf(stderr, "Error: Offset does not start with '0x'.\n");
                return -1;
            }
            char* endptr;
            errno = 0;
            unsigned long offset = strtoul(buffer + 2, &endptr, 16);
            if (errno != 0 || (*endptr != '\0' && *endptr != '\r') || offset > LONG_MAX) {
                fprintf(stderr, "Error: Invalid offset format or out of range.\n");
                return -1; // Error
            }
            if (c == '\n') {
                // LF is handled
            } else if (c == '\r') {
                // Consume the following LF for CRLF
                int next_c = fgetc(file);
                if (next_c != '\n') {
                    ungetc(next_c, file); // Put back if not LF
                }
            }
            return (long)offset; // Return the parsed offset
        } else {
            buffer[i++] = (char)c;
        }
    }

    if (i == sizeof(buffer) - 1) {
        fprintf(stderr, "Error: Offset too long.\n");
        return -1; // Error
    }

    // Handle the case where the offset is at the end of the file
    buffer[i] = '\0';
    if (strncmp(buffer, "0x", 2) != 0) {
        fprintf(stderr, "Error: Offset does not start with '0x'.\n");
        return -1;
    }
    char* endptr;
    errno = 0;
    unsigned long offset = strtoul(buffer + 2, &endptr, 16);
    if (errno != 0 || (*endptr != '\0' && *endptr != '\r') || offset > LONG_MAX) {
        fprintf(stderr, "Error: Invalid offset format or out of range.\n");
        return -1; // Error
    }
    return (long)offset; // Return the parsed offset
}

int read_header_csv(const char* filename, HCAHeader** headers, int* count) {
    char header_file[MAX_PATH];
    snprintf(header_file, sizeof(header_file), "%s%s", app_config.program_directory,
             extract_name_from_path(filename));
    FILE* file = fopen(header_file, "rb");
    if (!file) {
        fprintf(stderr, "Error: Could not open header CSV file: %s\n", header_file);
        return -1;
    }

    char line[1024]; // Buffer for reading the header line
    *count = 0;
    *headers = NULL;

    // Skip the header line
    if (!fgets(line, sizeof(line), file)) {
        fprintf(stderr, "Error: Failed to read header line.\n");
        fclose(file);
        return -1;
    }

    while (1) {
        long index = read_index(file);
        if (index == -1) {
            if (feof(file)) {
                break; // End of file reached correctly
            } else {
                fprintf(stderr, "Error: Error: Failed to read index data. Aborting\n");
                goto cleanup;
            }
        } else if (index == -2) {
            fprintf(stderr, "Error: Error: Failed to parse index data. Aborting\n");
            goto cleanup;
        }

        long offset = read_offset(file);
        if (offset == -1) {
            if (feof(file)) {
                break; // End of file reached
            } else {
                fprintf(stderr, "Error: Error: Failed to read offset data. Aborting\n");
                goto cleanup;
            }
        }

        *headers = realloc(*headers, (*count + 1) * sizeof(HCAHeader));
        if (!*headers) {
            fprintf(stderr, "Error: Memory allocation failed.\n");
            goto cleanup;
        }

        (*headers)[*count].index = (int)index;
        (*headers)[*count].offset = offset;

        // Read header data
        size_t bytes_read = fread((*headers)[*count].header, 1, HCA_HEADER_SIZE, file);
        if (bytes_read != HCA_HEADER_SIZE) {
            if (feof(file)) {
                fprintf(stderr, "Info: Reached end of file while reading header data.\n");
                goto cleanup;
            } else {
                fprintf(stderr, "Error: Failed to read header data.\n");
                goto cleanup;
            }
        }

        // Consume the newline or EOF
        int c = fgetc(file);
        if (c == '\n') {
            // Newline is consumed, continue
        } else if (c == EOF) {
            // EOF is valid after header
        } else {
            fprintf(stderr, "Error: Unexpected character after header.\n");
            goto cleanup;
        }

        (*count)++;
        if (c == EOF) {
            break; // Break loop if EOF was encountered
        }
    }

    // Write a temporary CSV file for debugging
/*    char temp_filename[MAX_PATH];
    snprintf(temp_filename, sizeof(temp_filename), "%s_temp.csv", get_basename(header_file));
    write_header_csv(temp_filename, *headers, *count);*/

    fclose(file);
    return 0;

cleanup:
    free(*headers);
    *headers = NULL;
    fclose(file);
    return -1;
}

