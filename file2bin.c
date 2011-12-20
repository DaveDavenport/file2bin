/* file2bin.c: Converts file(s) to a binary stream starting with size (32bit unsigned int)
 * and padded to whole words (32 bit). 
 *
 * Copyright (C) 2011 Martijn Koedam 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * This program was written for use with the CompSoC platform (TU/e, TUd).
 */

/***
 * Output format:
 * 32bit unsigned int with size of binary data (excluding itself).
 * <file 1> padded to 4bytes (32 bits, one word)
 * <file ...> 
 * <file n> padded to 4bytes (32 bits, one word)
 */

#include <stdio.h>

// Exit status
#include <stdlib.h>

// Error reporting
#include <errno.h>
#include <string.h>

// File stat
#include <sys/types.h>
#include <sys/stat.h>

// File reading
#include <fcntl.h>
#include <unistd.h>

// uint32_t and endian conversion
#include <stdint.h>
#include <endian.h>

// The maximum size we support. Currently 16MB should be sufficient.
const unsigned int MAX_SIZE = 1024*1024*16;


// The output is rounded up to complete words.
// Padding is added as zeros. 
// On the target platform memory can only be accessed in words.
const unsigned int WORD_SIZE = 4;


int main ( int argc , char **argv )
{
	// Total size we are going transmit (without size header)
	uint32_t total_size = 0;
	unsigned int number_files = argc;

	// Check number of arguments.
	if(argc < 2) {
		fprintf(stderr, "%s: requires 1 input file.\n", argv[0]);
		return EXIT_FAILURE;
	}

	for(unsigned int current_file = 1; current_file < number_files;current_file++)
	{
		// Get file size 
		struct stat file_stat;
		int retv = stat(argv[current_file], &file_stat);

		// Check help message.
		if(strcmp(argv[current_file], "-h") == 0 || strcmp(argv[current_file], "--help")==0)
		{
			fprintf(stderr, "Usage: %s <input files>\n\n", argv[0]);
			fprintf(stderr, "%s packs all given input files into one binary stream. Each file is padded to be word aligned.\n", argv[0]);
			fprintf(stderr, "Prepended is the size of the stream in a 32bit little endian unsigned int.\n");
			return EXIT_SUCCESS;
		}

		// Error
		if(retv != 0)
		{
			fprintf(stderr, "%s: Failed to open input file: %s, reason: %s\n", 
					argv[0],
					argv[current_file],
					strerror(errno));
			return EXIT_FAILURE;
		}
		// Check read permission.
		// size
		if(file_stat.st_size > MAX_SIZE) 
		{
			fprintf(stderr, "%s: File '%s' size is to big: %u bytes (limit %u bytes)\n", 
					argv[0],
					argv[current_file],
					(unsigned int) file_stat.st_size,
					MAX_SIZE);
			return EXIT_FAILURE;
		}
		uint32_t file_size = file_stat.st_size;
		uint32_t padding = file_size%WORD_SIZE;
		// Calculate padding 

		padding = (padding > 0)?(WORD_SIZE-padding):0;
		total_size += file_size+padding;

		fprintf(stderr, "File size: %u, padding: %u, total: %u\n", file_size, padding, total_size);
	}
	// Check total size
	if(total_size > MAX_SIZE) {
			fprintf(stderr, "%s: File size is to big: %u bytes (limit %u bytes)\n", 
					argv[0],
					(unsigned int)total_size, 
					MAX_SIZE);
			return EXIT_FAILURE;
	}

	// Explicitely convert to little endian.	
	uint32_t le_tsz = htole32(total_size);
	// Write out total file size.
	fwrite((void*)&le_tsz, sizeof(uint32_t),1,stdout);

	for(unsigned int current_file = 1; current_file < number_files;current_file++)
	{
		// Get file size 
		struct stat file_stat;
		int retv = stat(argv[current_file], &file_stat);

		// Error
		if(retv != 0)
		{
			fprintf(stderr, "%s: Failed to open input file: %s, reason: %s\n", 
					argv[0],
					argv[current_file],
					strerror(errno));
			return EXIT_FAILURE;
		}
		// Check read permission.
		uint32_t file_size = file_stat.st_size;
		uint32_t padding = file_size%WORD_SIZE;
		// Calculate padding 
		padding = (padding > 0)?(WORD_SIZE-padding):0;

		// Read the file.
		int fd = open(argv[current_file], O_RDONLY);

		// Read bytewise.
		unsigned char d;
		while(read(fd, &d,1) == 1)
		{ 
			putc(d, stdout);
			file_size--;
		}

		// Check if complete file was written.
		if(file_size > 0)
		{
			// Read data does not match up.
			fprintf(stderr, "%s:  Failed to read complete file, %u bytes remaining.\n", argv[0], file_size); 
			close(fd);	
			return EXIT_FAILURE;
		}

		// Write padding.
		while(padding) {
			putc(0, stdout);
			padding--;
		}

		// Close
		close(fd);
	}
	// Return success
	return EXIT_SUCCESS;
}
