#include "awb.h"
#include "hca_tool.h"
#include "utils.h"
#include <inttypes.h>

#define READ_BUFFER_SIZE (1024 * 1024) // 1MB buffer

int process_uasset_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Could not open UASSET file: %s\n", filename);
        return 1;
    }

    UAssetHeader* headers = NULL;
    int header_count = 0;
    long current_pos = 0;
    size_t bytes_read;
    uint8_t buffer[READ_BUFFER_SIZE];

    int add_to_index = 0;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        for (size_t i = 0; i <= bytes_read - sizeof(hca_signature); ++i) {
            if (memcmp(buffer + i, hca_signature, sizeof(hca_signature)) == 0) {
                headers = realloc(headers, (header_count + 1) * sizeof(UAssetHeader));
                if (!headers) {
                    fclose(file);
                    perror("realloc failed");
                    return 1;
                }

                if(header_count > 37)
                    add_to_index = -38;
                else
                    add_to_index = 82;
                headers[header_count].index = header_count + add_to_index;
                headers[header_count].offset = current_pos + i;

                long bytes_left_in_buffer = bytes_read - i;
                size_t bytes_to_copy = (bytes_left_in_buffer >= HCA_HEADER_SIZE) ?
                                       HCA_HEADER_SIZE : bytes_left_in_buffer;
                memcpy(headers[header_count].header, buffer + i, bytes_to_copy);

                if (bytes_to_copy < HCA_HEADER_SIZE) {
                    size_t remaining_bytes = HCA_HEADER_SIZE - bytes_to_copy;
                    fseek(file, current_pos + i + bytes_to_copy, SEEK_SET);
                    if (fread(headers[header_count].header + bytes_to_copy, 1, remaining_bytes, file) != remaining_bytes) {
                        fclose(file);
                        fprintf(stderr, "Error: Could not read complete header at offset %ld\n", headers[header_count].offset);
                        free(headers);
                        return 1;
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

    if (header_count > 0) {
        char output_filename[MAX_PATH];
        snprintf(output_filename, sizeof(output_filename), "%s_uasset_headers.csv", get_basename(filename));
        write_header_csv(output_filename, (HCAHeader *)headers, header_count);
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
		snprintf(hca_filename, sizeof(hca_filename), "%s/%d.hca", dir_name, i + add_to_index);
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

	int add_to_index = 0;
	if (strstr(filename, "Cnk_00") != NULL) {
		add_to_index = 82;
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
	snprintf(header_file, sizeof(header_file), "%s%s", program_directory,
	         extract_name_from_path( filename));
	FILE* file = fopen(header_file, "wb");
	if (!file) {
		return -1;
	}

	// Write header line (as bytes)
	fwrite("Index,Offset,Header\n", 1, strlen("Index,Offset,Header\n"), file);

	for (int i = 0; i < count; i++) {
		char line_buffer[512];
		int bytes_written = snprintf(line_buffer, sizeof(line_buffer),
		                             "%d,0x%" PRIx64 ",", headers[i].index,
		                             (uint64_t)headers[i].offset); // Use PRIx64 for long
		fwrite(line_buffer, 1, bytes_written, file);
		fwrite(headers[i].header, 1, HCA_HEADER_SIZE,
		       file); // Write header bytes directly
		fwrite("\n", 1, 1, file);
	}
	fclose(file);
	return 0;
}

int read_header_csv(const char* filename, HCAHeader** headers, int* count) {
	char header_file[MAX_PATH];
	snprintf(header_file, sizeof(header_file), "%s%s", program_directory,
	         extract_name_from_path( filename));
	FILE* file = fopen(header_file, "rb");  // Open in binary read mode
	if (!file) {
		fprintf(stderr, "Error: Could not open header CSV file: %s\n", header_file);
		return -1;
	}

	char line[1024];
	*count = 0;
	*headers = NULL;

	// Skip header line (read as bytes)
	if (fgets(line, sizeof(line), file) == NULL) {
		fclose(file);
		return -1;
	}


	while (fgets(line, sizeof(line), file)) {
		*headers = realloc(*headers, (*count + 1) * sizeof(HCAHeader));
		if (!*headers) {
			fclose(file);
			return -1;
		}

		char* header_start = strchr(line, ','); // Find start of header bytes
		if (!header_start) continue; // Skip lines with no header
		header_start = strchr(header_start + 1,
		                      ',') + 1; // Move to after second comma


		if (sscanf(line, "%d,0x%lx", &(*headers)[*count].index,
		           &(*headers)[*count].offset) == 2) {
			memcpy((*headers)[*count].header, header_start,
			       HCA_HEADER_SIZE);  // Directly copy the bytes
			(*count)++;
		}
	}

	fclose(file);
	return 0;
}
