#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// Bear in mind titles have no max length and you could crash windows explorer with a big enough title, or if this code runs incorrectly
// This use RIFF's Info Tags, see here to add more: https://exiftool.org/TagNames/RIFF.html#Info

// 4-byte chunk ID
typedef struct {
    char id[4];
} ChunkID;

// Write a 4-byte chunk ID
void writeChunkID(FILE* file, const char* id) {
    fwrite(id, 1, 4, file);
}

// Write a 32-bit integer in little-endian format
void writeUInt32LE(FILE* file, unsigned int value) {
    fputc((value >> 0) & 0xFF, file);
    fputc((value >> 8) & 0xFF, file);
    fputc((value >> 16) & 0xFF, file);
    fputc((value >> 24) & 0xFF, file);
}

// Write a string (with null terminator and padding)
void writeString(FILE* file, const char* str) {
    int len = strlen(str);
    fwrite(str, 1, len, file);
    fputc(0x00, file); // Write null terminator

    // Add padding if necessary to make the chunk size even
    if ((len + 1) % 2 != 0) {
        fputc(0x00, file);
    }
}

// Calculate the size of a string, including null terminator and padding
int calculatePaddedStringSize(const char* str) {
    int len = strlen(str);
    // Add 1 for null terminator, then round up to the nearest even number
    return (len + 1 + (len + 1) % 2) & ~1;
}

// Read a 4-byte chunk ID
ChunkID readChunkID(FILE* file) {
    ChunkID chunkID;
    fread(chunkID.id, 1, 4, file);
    return chunkID;
}

// Read a 32-bit integer in little-endian format
unsigned int readUInt32LE(FILE* file) {
    unsigned int value = 0;
    value |= fgetc(file) << 0;
    value |= fgetc(file) << 8;
    value |= fgetc(file) << 16;
    value |= fgetc(file) << 24;
    return value;
}

// Remove existing RIFF metadata (LIST INFO chunk)
void removeMetadata(FILE* file) {
    long fileSize = 0;
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);

    // Limit the search to the last 200 bytes
    long searchStart = fileSize - 200;
    if (searchStart < 0) {
        searchStart = 0; // If the file is smaller than 200 bytes, start from the beginning
    }

    // Search for LIST INFO within the limited range
    for (long i = searchStart; i < fileSize; ++i) {
        fseek(file, i, SEEK_SET);

        ChunkID listChunkID = readChunkID(file);
        if (strncmp(listChunkID.id, "LIST", 4) == 0) {
            // Read the LIST type (should be "INFO")
            ChunkID listType = readChunkID(file);

            if (strncmp(listType.id, "INFO", 4) == 0) {
                ftruncate(fileno(file), i); // Truncate the file before the LIST chunk
                fseek(file, 0, SEEK_END); // Reset file pointer to the end
                break;
            }
        }
    }
}

// Add RIFF metadata (LIST INFO with INAM, IPRD, IART, IGNR, and ITRK)
void addRIFFMetadata(FILE* file, const char* title, const char* album, const char* artist, const char* genre, const char* trackNumber) {
    // LIST INFO chunk
    writeChunkID(file, "LIST");

    // Calculate the size of the LIST chunk (including null terminators and padding)
    int inamSize = 8 + calculatePaddedStringSize(title);        // 8 for INAM chunk ID and size
    int iprdSize = 8 + calculatePaddedStringSize(album);      // 8 for IPRD chunk ID and size (album)
    int iartSize = 8 + calculatePaddedStringSize(artist);     // 8 for IART chunk ID and size
    int ignrSize = 8 + calculatePaddedStringSize(genre);      // 8 for IGNR chunk ID and size
    int itrkSize = 8 + calculatePaddedStringSize(trackNumber); // 8 for ITRK chunk ID and size
    int listChunkSize = 4 + inamSize + iprdSize + iartSize + ignrSize + itrkSize; // 4 for "INFO" ID

    writeUInt32LE(file, listChunkSize);
    writeChunkID(file, "INFO");

    // INAM chunk (Title)
    writeChunkID(file, "INAM");
    writeUInt32LE(file, calculatePaddedStringSize(title));
    writeString(file, title);

    // IPRD chunk (Album)
    writeChunkID(file, "IPRD");
    writeUInt32LE(file, calculatePaddedStringSize(album));
    writeString(file, album);

    // IART chunk (Artist)
    writeChunkID(file, "IART");
    writeUInt32LE(file, calculatePaddedStringSize(artist));
    writeString(file, artist);

    // IGNR chunk (Genre)
    writeChunkID(file, "IGNR");
    writeUInt32LE(file, calculatePaddedStringSize(genre));
    writeString(file, genre);

    // ITRK chunk (Track Number)
    writeChunkID(file, "ITRK");
    writeUInt32LE(file, calculatePaddedStringSize(trackNumber));
    writeString(file, trackNumber);
}

int main(int argc, char* argv[]) {
    if (argc != 7) {
        fprintf(stderr, "Usage: %s <filename> <title> <album> <artist> <genre> <track_number>\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];
    const char* title = argv[2];
    const char* album = argv[3];
    const char* artist = argv[4];
    const char* genre = argv[5];
    const char* trackNumber = argv[6];

    FILE* file = fopen(filename, "rb+"); // Open in binary read and write mode
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    // Remove existing metadata (only LIST INFO, limited search)
    removeMetadata(file);

    // Add RIFF metadata to the end of the file
    addRIFFMetadata(file, title, album, artist, genre, trackNumber);

    fclose(file);

    printf("Metadata added to '%s'\n", filename);

    return 0;
}
