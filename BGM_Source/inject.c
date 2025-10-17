#include "inject.h"
#include "utils.h"
#include "awb.h"
#include "offset_updater.h"

bool create_backup(const char* filename) {
	char backup_path[MAX_PATH];
	snprintf(backup_path, sizeof(backup_path), "%s.bak", filename);

	// Check if backup already exists
	if (access(backup_path, F_OK) != -1) {
		return true;  // Backup already exists
	}

	FILE* source = fopen(filename, "rb");
	FILE* backup = fopen(backup_path, "wb");

	if (!source || !backup) {
		if (source) fclose(source);
		if (backup) fclose(backup);
		return false;
	}

	char buffer[8192];
	size_t bytes;

	while ((bytes = fread(buffer, 1, sizeof(buffer), source)) > 0) {
		if (fwrite(buffer, 1, bytes, backup) != bytes) {
			fclose(source);
			fclose(backup);
			return false;
		}
	}

	fclose(source);
	fclose(backup);
	return true;
}

void recalculate_subsequent_offsets(HCAHeader* headers, int header_count,
                                    int current_index, long size_difference) {
	// Adjust all offsets that come after the replaced file
	for (int i = current_index + 1; i < header_count; i++) {
		headers[i].offset += size_difference;
	}
}

// Fixed version with additional safety checks and proper error handling
#define SAFE_BUFFER_SIZE (1024 * 128)  // 128KB chunks

long replaceFileContent(FILE* target, FILE* new_hca,
                        HCAHeader* target_headers,
                        int j, int target_header_count, long file_end_offset, int *padding_added) {
	*padding_added = 0; // Initialize padding to zero

	// Get content boundaries
	long content_start = target_headers[j].offset;
	long content_end = (j < target_header_count - 1) ?
	                   target_headers[j + 1].offset : file_end_offset;

	// Validate positions and parameters
	if (!target || !new_hca || !target_headers || j < 0 ||
	        content_start < 0 || content_end < 0 || content_start >= content_end) {
		return -1;
	}

	// Get sizes and validate file positions
	if (fseek(new_hca, 0, SEEK_END) != 0) return -1;
	long new_size = ftell(new_hca);
	if (new_size < 0) return -1;

	clearerr(new_hca);
	rewind(new_hca);

	if (fseek(target, 0, SEEK_END) != 0) return -1;
	long total_size = ftell(target);
	if (total_size < 0 || total_size < content_end) return -1;

	long original_size = content_end - content_start;

	// Calculate padding before calculating size difference
	long padding = 0;
	if (content_end != file_end_offset && j < target_header_count - 1) {
		long next_file_offset = content_start + new_size;
		if (next_file_offset % 32 != 0) {
			padding = 32 - (next_file_offset % 32);
			*padding_added = padding;
		}
	}

	// Calculate total size difference including padding
	long size_diff = (new_size + padding) - original_size;
	long remaining_data = total_size - content_end;

	// Allocate buffer with error handling
	char* buffer = malloc(SAFE_BUFFER_SIZE);
	if (!buffer) return -1;

	// Handle data movement if size changes
	if (size_diff != 0 && remaining_data > 0) {
		if (size_diff > 0) {
			// Move data forward (working backwards)
			for (long pos = remaining_data; pos > 0; pos -= SAFE_BUFFER_SIZE) {
				long chunk_size = (pos > SAFE_BUFFER_SIZE) ? SAFE_BUFFER_SIZE : pos;
				long read_pos = content_end + pos - chunk_size;
				long write_pos = read_pos + size_diff;

				if (fseek(target, read_pos, SEEK_SET) != 0) {
					free(buffer);
					return -1;
				}

				size_t bytes_read = fread(buffer, 1, chunk_size, target);
				if (bytes_read != chunk_size) {
					free(buffer);
					return -1;
				}

				if (fseek(target, write_pos, SEEK_SET) != 0) {
					free(buffer);
					return -1;
				}

				if (fwrite(buffer, 1, bytes_read, target) != bytes_read) {
					free(buffer);
					return -1;
				}
			}
		} else {
			// Move data backward (working forwards)
			for (long pos = 0; pos < remaining_data; pos += SAFE_BUFFER_SIZE) {
				long chunk_size = (remaining_data - pos > SAFE_BUFFER_SIZE) ?
				                  SAFE_BUFFER_SIZE : remaining_data - pos;

				if (fseek(target, content_end + pos, SEEK_SET) != 0) {
					free(buffer);
					return -1;
				}

				size_t bytes_read = fread(buffer, 1, chunk_size, target);
				if (bytes_read != chunk_size) {
					free(buffer);
					return -1;
				}

				if (fseek(target, content_start + new_size + padding + pos, SEEK_SET) != 0) {
					free(buffer);
					return -1;
				}

				if (fwrite(buffer, 1, bytes_read, target) != bytes_read) {
					free(buffer);
					return -1;
				}
			}

			// Truncate immediately after moving data for shrinking case
			if (ftruncate(fileno(target), total_size + size_diff) != 0) {
				free(buffer);
				return -1;
			}
		}
	}

	// Write new content
	if (fseek(target, content_start, SEEK_SET) != 0) {
		free(buffer);
		return -1;
	}

	// Copy new content
	size_t total_written = 0;
	while (total_written < (size_t)new_size) {
		size_t remaining = new_size - total_written;
		size_t chunk_size = (remaining > SAFE_BUFFER_SIZE) ? SAFE_BUFFER_SIZE :
		                    remaining;

		size_t bytes_read = fread(buffer, 1, chunk_size, new_hca);
		if (bytes_read == 0) {
			free(buffer);
			return -1;
		}

		if (fwrite(buffer, 1, bytes_read, target) != bytes_read) {
			free(buffer);
			return -1;
		}

		total_written += bytes_read;
	}

	// Write padding bytes
	if (padding > 0) {
		char zero_buffer[32] = {0};
		if (fwrite(zero_buffer, 1, padding, target) != padding) {
			free(buffer);
			return -1;
		}
		fflush(target);
	}

	// Final size verification and adjustment
	if (fseek(target, 0, SEEK_END) != 0) {
		free(buffer);
		return -1;
	}

	long final_size = ftell(target);
	if (final_size != total_size + size_diff) {
		if (ftruncate(fileno(target), total_size + size_diff) != 0) {
			free(buffer);
			return -1;
		}
	}

	free(buffer);
	return 0;
}

bool inject_hca(const char* container_path, InjectionInfo* injections,
                int injection_count) {

	printf("Processing container: %s\n", extract_name_from_path(container_path));

	FILE* container = fopen(container_path, "r+b");
	if (!container) {
		fprintf(stderr, "Error opening container file\n");
		return false;
	}

	create_backup(container_path);

	// Get target uasset headers - headers CSV should be in same directory as target file
	UAssetHeader* uasset_headers = NULL;
	int uasset_header_count = 0;
	char uasset_header_csv[MAX_PATH];

	// Get directory path of target uasset file
	char target_uasset_dir[MAX_PATH] = {0};
	strcpy(target_uasset_dir, get_parent_directory(container_path));

	snprintf(uasset_header_csv, sizeof(uasset_header_csv),
	         "%s_uasset_headers.csv", get_basename(container_path));

	// Only needed once since containers are grouped
	if (read_header_csv(uasset_header_csv, &uasset_headers,
	                    &uasset_header_count) != 0) {
		fclose(container);
		free(uasset_headers);
		return false;
	}

	// Process each injection
	for (int i = 0; i < injection_count; i++) {
		// Skip injections that were already processed (marked with index -1)
		if (injections[i].index == -1) continue;



		// Get target file headers - headers CSV should be in same directory as target file
		HCAHeader* target_headers = NULL;
		int target_header_count = 0;
		char header_csv[MAX_PATH];

		// Get directory path of target file
		char target_dir[MAX_PATH] = {0};
		strcpy(target_dir, get_parent_directory(injections[i].target_file));

		snprintf(header_csv, sizeof(header_csv), "%s\\%s_headers.csv",
		         target_dir, get_basename(injections[i].target_file));

		// Why read and write each time? I don't want to implement a cache and there
		// are multiple AWBs for the same container
		if (read_header_csv(header_csv, &target_headers, &target_header_count) != 0) {
			fclose(container);
			free(uasset_headers);
			free(target_headers);
			fclose(container);
			return false;
		}

		bool header_found = false;
		// Find matching header in target headers
		for (int j = 0; j < target_header_count; j++) {
			if (target_headers[j].index == injections[i].index) {
				header_found = true;

				printf("Injecting %s at index (%d)\n",
				       extract_name_from_path(injections[i].hca_path), injections[i].index);

				int uasset_header_index = -1;

				for (int k = 0; k < uasset_header_count; k++) {
					if (uasset_headers[k].index == injections[i].index) {
						uasset_header_index = k;
						break;
					}
				}

				if (uasset_header_index < 0) {
					fprintf(stderr, "No matching header found for index %d in uasset\n",
					        injections[i].index);
					continue; // Skip to the next injection
				}

				// Read new HCA file
				FILE* new_hca = fopen(injections[i].hca_path, "rb");
				if (!new_hca) {
					fprintf(stderr, "Error opening new HCA file: %s\n", injections[i].hca_path);
					continue;
				}

				// Read target file
				FILE* target = fopen(injections[i].target_file, "r+b");
				if (!target) {
					fprintf(stderr, "Error opening target file: %s\n", injections[i].target_file);
					fclose(new_hca);
					continue;
				}

				fseek(target, 0, SEEK_END);
				long file_end_offset = ftell(target);

				// Get file sizes
				fseek(new_hca, 0, SEEK_END);
				long new_size = ftell(new_hca);
				fseek(new_hca, 0, SEEK_SET);

				long next_offset;
				if (j < target_header_count - 1)
					next_offset = target_headers[j + 1].offset;
				else
					next_offset = file_end_offset;

				long original_size = next_offset - target_headers[j].offset;

				// Check if the new file is too large to replace the existing content
				if (fixed_size && new_size > original_size) {
					int thousands = original_size / (1024 * 1000);
					int remainder = (original_size / 1024) % 1000;
					if (thousands)
						fprintf(stderr,
						        "-> Error: New HCA file is larger than the original %d,%dKB. File skipped.\n\n",
						        thousands, remainder);
					else
						fprintf(stderr,
						        "-> Error: New HCA file is larger than the original %dKB. File skipped.\n\n",
						        remainder);

					fclose(new_hca);
					fclose(target);
					continue; // Skip this injection and move to the next
				}

				if (uasset_header_index < uasset_header_count - 1)
					next_offset = uasset_headers[uasset_header_index + 1].offset;
				else
					next_offset = uasset_headers[uasset_header_index].offset + HCA_MAX_SIZE;

				if (!replace_header_at_offset(container,
				                              uasset_headers[uasset_header_index].offset, next_offset,
				                              injections[i].new_header, HCA_MAX_SIZE)) {
					fprintf(stderr, "Failed to replace header in uasset for index %d\n",
					        injections[i].index);
					fclose(new_hca);
					fclose(target);
					continue;
				}

				// Perform replacement
				fseek(target, target_headers[j].offset, SEEK_SET);

				if (fixed_size) {
					// Option 1: Replace content and zero out remaining bytes if new content is smaller
					long remaining = original_size - new_size;
					long buffer_size = (remaining > 0) ? original_size : new_size;

					char* buffer = malloc(buffer_size);
					if (buffer) {
						// Write new HCA content
						fread(buffer, 1, new_size, new_hca);
						fwrite(buffer, 1, new_size, target);

						// If new content is smaller, zero out remaining bytes
						if (new_size < original_size) {
							// Zero out the remaining bytes if there's any remaining space
							memset(buffer, 0, remaining);  // Only zero out remaining space
							fwrite(buffer, 1, remaining, target);
						}

						free(buffer);
					} else {
						fprintf(stderr, "Memory allocation failed for buffer\n");
					}

					fclose(target);
					fclose(new_hca);
				} else {
					// Option 2: Delete and shift content
					int padding_count;
					int result = replaceFileContent(target, new_hca, target_headers, j,
					                                target_header_count, file_end_offset, &padding_count);
					if (result != 0) {
						// Handle error
						fprintf(stderr, "Failed to replace file content\n");
						fclose(target);
						fclose(new_hca);
						free(uasset_headers);
						free(target_headers);
						fclose(container);
						return false;
					}

					fclose(target);
					fclose(new_hca);

					long size_difference = new_size - original_size + padding_count;

					recalculate_subsequent_offsets(target_headers, target_header_count,
					                               j, size_difference);

					if (write_header_csv(header_csv, target_headers, target_header_count) != 0) {
						fprintf(stderr, "Failed to update target headers CSV file\n");
						free(uasset_headers);
						free(target_headers);
						fclose(container);
						return false;
					}

					file_end_offset += size_difference;

					// After injection is complete, update offsets
					// injections[i].index OR get_file_index_start(injections[i].target_file)
					if (!update_offset_range(injections[i].target_file, container_path,
					                         target_headers,
					                         get_file_index_start(injections[i].target_file), target_header_count, file_end_offset)) {
						fprintf(stderr, "Failed to update offsets, your uasset is corrupted.\n");
						free(uasset_headers);
						free(target_headers);
						fclose(container);
						return false;
					}
					// This one subtracts padding 0s as the game originally does, to a T
					// Yet it for some reason doesn't like it
					/*if (!update_offset_range_with_padding(injections[i].target_file, container_path,
					                                      target_headers,
					                                      get_file_index_start(injections[i].target_file),
					                                      target_header_count, file_end_offset, size_difference, padding_count)) {
						fprintf(stderr, "Failed to update offsets, your uasset is corrupted.\n");
						free(uasset_headers);
						free(target_headers);
						fclose(container);
						return false;
					}*/
				}
				break;
			}
		}

		if (!header_found) {
			fprintf(stderr, "No matching header found for index %d\n",
			        injections[i].index);
		}

		free(target_headers);
	}

	return true;
}

bool replace_header_at_offset(FILE * file, long offset, long next_offset,
                              const uint8_t* new_header, long new_header_size) {
	// Calculate the available space between offsets
	long available_space = next_offset - offset;
	// Ensure we don't write beyond the available space
	long bytes_to_write = (new_header_size <= available_space) ? new_header_size :
	                      available_space;
	// Seek to the offset position and write the new header within the space limit
	fseek(file, offset, SEEK_SET);
	fwrite(new_header, 1, bytes_to_write, file);
	// If the new header is smaller than the available space, zero out the remaining space
	if (bytes_to_write < available_space) {
		long bytes_to_zero = available_space - bytes_to_write;
		uint8_t zero_buffer[1024] = {0};
		// Zero out the remaining space until next_offset
		while (bytes_to_zero > 0) {
			long zero_chunk = (bytes_to_zero > sizeof(zero_buffer)) ? sizeof(
			                      zero_buffer) : bytes_to_zero;
			fwrite(zero_buffer, 1, zero_chunk, file);
			bytes_to_zero -= zero_chunk;
		}
	}
	return true;
}
