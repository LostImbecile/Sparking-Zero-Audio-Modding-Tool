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

static long find_table_start(FILE* file, long header_pos) {
	uint8_t buffer[32];  // Enough to contain the header

	if (fseek(file, header_pos, SEEK_SET) != 0) {
		return -1;
	}

	if (fread(buffer, 1, sizeof(buffer), file) != sizeof(buffer)) {
		return -1;
	}

	// Look for the sequence 20 00 C0 85
	for (size_t i = 0; i < sizeof(buffer) - 3; i++) {
		if (buffer[i] == 0x20 &&
		        buffer[i + 1] == 0x00 &&
		        buffer[i + 2] == 0xC0 &&
		        buffer[i + 3] == 0x85) {
			return header_pos + i + 4;  // Return position after the sequence
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

bool update_offset_range(const char* awb_path, const char* uasset_path,
                         const HCAHeader* headers, int start_idx, int end_idx,
                         int file_end_offset) {
	FILE* awb = fopen(awb_path, "rb+");
	FILE* uasset = fopen(uasset_path, "rb+");

	if (!awb || !uasset) {
		if (awb) fclose(awb);
		if (uasset) fclose(uasset);
		return false;
	}

	fseek(uasset, 0, SEEK_END);
	long uasset_size = ftell(uasset);

	// Search for first header in last 2KB of file
	long search_start = uasset_size - 2048;
	long first_header_pos = find_afs2_header(uasset, 0, search_start,
	                        uasset_size);
	if (first_header_pos == -1) {
		printf("Failed to find first AFS2 header\n");
		fclose(awb);
		fclose(uasset);
		return false;
	}

	// Search for second header after the first one
	long second_header_pos = find_afs2_header(uasset, 1, first_header_pos,
	                         uasset_size);
	if (second_header_pos == -1) {
		printf("Failed to find second AFS2 header\n");
		fclose(awb);
		fclose(uasset);
		return false;
	}

	// Find the actual table positions
	long first_table_pos = find_table_start(uasset, first_header_pos);
	long second_table_pos = find_table_start(uasset, second_header_pos);
	/*	long awb_table_pos = find_table_start(awb, 0);*/

	if (first_table_pos == -1 || second_table_pos == -1) {
		printf("Failed to find table start positions\n");
		fclose(awb);
		fclose(uasset);
		return false;
	}

	int deduct = 0;

	if (end_idx < 82) {
		end_idx += 82;
		deduct = 82;
	}

	const int FIRST_TABLE_SIZE = 82 * 4;  // Size of first offset table
	const int SECOND_TABLE_SIZE = 38 * 4; // Size of second offset table

	// Process offsets
	for (int i = start_idx; i <= end_idx; i++) {
		uint32_t offset = (i < end_idx) ? headers[i - deduct].offset :
		                  file_end_offset;

		long skip_bytes;
		if (i < 82 || (i == end_idx && i == 82)) {
			skip_bytes = FIRST_TABLE_SIZE + (i * 4);
		} else {
			skip_bytes = SECOND_TABLE_SIZE + ((i - deduct) * 4);
		}

		/*
		    // Update AWB's header as well
		    // This is commented out because it breaks things for some reason
		    long awb_write_pos = awb_table_pos + skip_bytes; // AWB has fixed 16-byte header

		    if (write_le32(awb, awb_write_pos, offset) != 4) {
		        printf("Failed to write to AWB at index %d\n", i);
		        continue;
		    }
		*/


		// Calculate UASSET write position
		long uasset_write_pos;
		if (i < 82 || (i == end_idx && i == 82)) {
			uasset_write_pos = first_table_pos + skip_bytes;
		} else {
			uasset_write_pos = second_table_pos + skip_bytes;
		}

		if (uasset_write_pos + 4 > uasset_size) {
			printf("UASSET write position out of bounds: %ld (size: %ld)\n",
			       uasset_write_pos, uasset_size);
			printf("%s is corrupted, please replace it with the original version.\n",
			       extract_name_from_path(awb_path));
			return false;
		}

		if (write_le32(uasset, uasset_write_pos, offset) != 4) {
			printf("Failed to write to UASSET at index %d\n", i);
			return false;
		}
	}

	fclose(awb);
	fclose(uasset);
	return true;
}

bool update_offset_range_with_padding(const char* awb_path, const char* uasset_path,
                                      const HCAHeader* headers,
                                      int start_idx, int end_idx, int file_end_offset,
                                      long size_difference, int padding_count) {

/*	FILE* awb = fopen(awb_path, "rb+");

	if (!awb) {
		if (awb) fclose(awb);
		return false;
	}

	long awb_table_pos = find_table_start(awb, 0);*/

	FILE* uasset = fopen(uasset_path, "rb+");

	fseek(uasset, 0, SEEK_END);
	long uasset_size = ftell(uasset);

	// Search for first header in last 2KB of file
	long search_start = uasset_size - 2048;
	long first_header_pos = find_afs2_header(uasset, 0, search_start,
	                        uasset_size);
	if (first_header_pos == -1) {
		printf("Failed to find first AFS2 header\n");
		fclose(uasset);
		return false;
	}

	// Search for second header after the first one
	long second_header_pos = find_afs2_header(uasset, 1, first_header_pos,
	                         uasset_size);
	if (second_header_pos == -1) {
		printf("Failed to find second AFS2 header\n");
		fclose(uasset);
		return false;
	}

	// Find the actual table positions
	long first_table_pos = find_table_start(uasset, first_header_pos);
	long second_table_pos = find_table_start(uasset, second_header_pos);

	if (first_table_pos == -1 || second_table_pos == -1) {
		printf("Failed to find table start positions\n");
		fclose(uasset);
		return false;
	}

	int deduct = 0;
	if (end_idx < 82) {
		end_idx += 82;
		deduct = 82;
	}

	const int FIRST_TABLE_SIZE = 82 * 4;
	const int SECOND_TABLE_SIZE = 38 * 4;

	for (int i = start_idx; i <= end_idx; i++) {
		uint32_t offset;
		long skip_bytes;

		if (i < 82 || (i == end_idx && i == 82)) {
			skip_bytes = FIRST_TABLE_SIZE + (i * 4);
		} else {
			skip_bytes = SECOND_TABLE_SIZE + ((i - deduct) * 4);
		}


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
			long read_pos = i < 82
			                ? first_table_pos + skip_bytes
			                : second_table_pos + skip_bytes;
			uint32_t original_offset = read_le32(uasset, read_pos);
			// Apply only the size difference to subsequent files
			offset = original_offset + size_difference;
		}


		// Update AWB's header as well
		// This is commented out because it breaks things for some reason
/*		long awb_write_pos = awb_table_pos + skip_bytes; // AWB has fixed 16-byte header

		if (write_le32(awb, awb_write_pos, offset) != 4) {
		    printf("Failed to write to AWB at index %d\n", i);
		    continue;
		}*/


		// Write the modified offset back as usual
		long uasset_write_pos = (i < 82 || (i == end_idx && i == 82))
		                        ? first_table_pos + skip_bytes
		                        : second_table_pos + skip_bytes;

		if (uasset_write_pos + 4 > uasset_size) {
			printf("UASSET write position out of bounds: %ld (size: %ld)\n",
			       uasset_write_pos, uasset_size);
			return false;
		}

		if (write_le32(uasset, uasset_write_pos, offset) != 4) {
			printf("Failed to write to UASSET at index %d\n", i);
			return false;
		}

	}

	fclose(uasset);
	return true;
}
