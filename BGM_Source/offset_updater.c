#include "offset_updater.h"
#include "utils.h"
#include <stdio.h>
#include <stdint.h>

static size_t write_le32(FILE* file, long position, uint32_t value) {
	uint8_t bytes[4];
	bytes[0] = (value & 0xFF);
	bytes[1] = ((value >> 8) & 0xFF);
	bytes[2] = ((value >> 16) & 0xFF);
	bytes[3] = ((value >> 24) & 0xFF);

	long current_pos = ftell(file);
	if (current_pos == -1) return 0;

	if (fseek(file, position, SEEK_SET) != 0) return 0;
	size_t written = fwrite(bytes, 1, 4, file);
	fseek(file, current_pos, SEEK_SET);

	return written;
}

static long find_afs2_header(FILE* file, int target_index, long start_pos,
                             long file_size) {
	if (file == NULL || start_pos < 0 || start_pos >= file_size) {
		return -1;
	}

	uint8_t buffer[4096];
	int found_count = 0;
	long current_pos = start_pos;

	if (fseek(file, start_pos, SEEK_SET) != 0) {
		return -1;
	}

	while (current_pos < file_size) {
		size_t bytes_to_read = sizeof(buffer);
		if (current_pos + bytes_to_read > file_size) {
			bytes_to_read = file_size - current_pos;
		}

		size_t bytes_read = fread(buffer, 1, bytes_to_read, file);
		if (bytes_read < 4) break;

		for (size_t i = 0; i < bytes_read - 3; i++) {
			if (buffer[i] == 0x41 &&     // 'A'
			        buffer[i + 1] == 0x46 && // 'F'
			        buffer[i + 2] == 0x53 && // 'S'
			        buffer[i + 3] == 0x32) { // '2'

				if (found_count == target_index) {
					return current_pos + i;
				}
				found_count++;
			}
		}

		current_pos += bytes_read - 3;
		if (fseek(file, current_pos, SEEK_SET) != 0) {
			return -1;
		}
	}

	return -1;
}

static uint32_t read_le32(FILE* file, long position) {
	uint8_t bytes[4];
	if (fseek(file, position, SEEK_SET) != 0) return 0;
	if (fread(bytes, 1, 4, file) != 4) return 0;

	return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
}

// Helper function to get the PortNo for a given AWB name
int get_awb_port(const char* awb_name) {
	for (int i = 0; i < acb_mapping_count; i++) {
		if (strcasecmp(acb_mappings[i].awbName,
		               extract_name_from_path(awb_name)) == 0) {
			return acb_mappings[i].portNo;
		}
	}
	return 0; // Default to 0 if not found
}

int get_table_size(const char* awb_name) {
	for (int i = 0; i < acb_mapping_count; i++) {
		if (strcasecmp(acb_mappings[i].awbName,
		               extract_name_from_path(awb_name)) == 0) {
			return acb_mappings[i].tracks * 4;
		}
	}
	return -1; // Indicate table size not found (error)
}

// Helper function to find the header position for a specific port
long find_header_for_port(FILE* uasset, long search_start, long uasset_size,
                          int target_port) {
	long header_pos = -1;
	for (int port = 0; port <= target_port; port++) {
		header_pos = find_afs2_header(uasset, port, search_start, uasset_size);
		if (header_pos == -1) {
			printf("Failed to find AFS2 header for port %d\n", port);
			return -1; // Return -1 to indicate failure
		}
		if (port == target_port) {
			return header_pos;
		}
		// Update search_start to look for the next header after the current one
		search_start = header_pos;
	}
	return -1;
}

bool update_offset_range(const char* awb_path, const char* uasset_path,
                         const HCAHeader* headers, int start_idx, int end_idx,
                         int file_end_offset) {
	FILE* uasset = fopen(uasset_path, "rb+");

	if (!uasset) {
		return false;
	}

	fseek(uasset, 0, SEEK_END);
	long uasset_size = ftell(uasset);

	// 1. Get the target port and table size for the AWB
	int target_port = get_awb_port(awb_path);
	int table_size = get_table_size(awb_path);

	if (table_size == -1) {
		printf("Failed to find table size for AWB: %s\n", awb_path);
		fclose(uasset);
		return false;
	}

	// 2. Find the header position for the target port
	long search_start = uasset_size - 2048; // Consider making this more dynamic
	long header_pos = find_header_for_port(uasset, search_start, uasset_size,
	                                       target_port);

	if (header_pos == -1) {
		printf("Failed to find AFS2 header for target port %d\n", target_port);
		fclose(uasset);
		return false;
	}

	// 3. Find the table position corresponding to the header
	long table_pos = header_pos + 16;
	if (table_pos == -1) {
		printf("Failed to find table start position\n");
		fclose(uasset);
		return false;
	}

	// 4. Adjust end_idx and deduct based on the starting index of the AWB file
	int deduct = get_file_index_start(awb_path);
	end_idx += deduct;

	// 5. Validate end_idx against table size
	if ((end_idx - deduct) * 4 > table_size) {
		printf("Error: end_idx exceeds table size for AWB: %s\n", awb_path);
		fclose(uasset);
		return false;
	}

	// 6. Process offsets
	for (int i = start_idx; i <= end_idx; i++) {
		uint32_t offset = (i < end_idx) ? headers[i - deduct].offset :
		                  file_end_offset;

		// Calculate UASSET write position
		long skip_bytes = table_size + (i - deduct) * 4;

		long uasset_write_pos = table_pos + skip_bytes;

		if (write_le32(uasset, uasset_write_pos, offset) != 4) {
			printf("Failed to write to UASSET at index %d\n", i);
			fclose(uasset);
			return false;
		}
	}

	fclose(uasset);
	return true;
}

bool update_offset_range_with_padding(const char* awb_path,
                                      const char* uasset_path,
                                      const HCAHeader* headers,
                                      int start_idx, int end_idx, int file_end_offset,
                                      long size_difference, int padding_count) {
	FILE* uasset = fopen(uasset_path, "rb+");

	if (!uasset) {
		return false;
	}

	fseek(uasset, 0, SEEK_END);
	long uasset_size = ftell(uasset);

	// 1. Get the target port and table size for the AWB
	int target_port = get_awb_port(awb_path);
	int table_size = get_table_size(awb_path);

	if (table_size == -1) {
		printf("Failed to find table size for AWB: %s\n", awb_path);
		fclose(uasset);
		return false;
	}

	// 2. Find the header position for the target port
	long search_start = uasset_size - 2048; // Consider making this more dynamic
	long header_pos = find_header_for_port(uasset, search_start, uasset_size,
	                                       target_port);

	if (header_pos == -1) {
		printf("Failed to find AFS2 header for target port %d\n", target_port);
		fclose(uasset);
		return false;
	}

	// 3. Find the table position corresponding to the header
	long table_pos = header_pos + 16;
	if (table_pos == -1) {
		printf("Failed to find table start position\n");
		fclose(uasset);
		return false;
	}

	// 4. Adjust end_idx and deduct based on the starting index of the AWB file
	int deduct = get_file_index_start(awb_path);
	end_idx += deduct;

	// 5. Validate end_idx against table size
	if ((end_idx - deduct) * 4 > table_size) {
		printf("Error: end_idx exceeds table size for AWB: %s\n", awb_path);
		fclose(uasset);
		return false;
	}

	// 6. Process offsets
	for (int i = start_idx; i <= end_idx; i++) {
		uint32_t offset;

		// Calculate UASSET write position
		long skip_bytes = table_size + (i - deduct) * 4;

		long uasset_write_pos = table_pos + skip_bytes;

		if (i == start_idx) {
			// Do nothing for the start index
			continue;
		} else if (i == end_idx) {
			// Use file_end_offset for the last offset
			offset = file_end_offset;
		} else if (i == start_idx + 1) {
			// Apply padding and size difference ONLY to the next file
			offset = headers[i - deduct].offset - padding_count;
		} else {
			// Apply only the size difference to subsequent files
			uint32_t original_offset = read_le32(uasset, uasset_write_pos);
			offset = original_offset + size_difference;
		}

		if (write_le32(uasset, uasset_write_pos, offset) != 4) {
			printf("Failed to write to UASSET at index %d\n", i);
			fclose(uasset);
			return false;
		}
	}

	fclose(uasset);
	return true;
}


