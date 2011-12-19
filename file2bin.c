/* file2bin.c: Converts a file to a binary starting with size (32bit) and padded to whole words (32 bit). 
 *
 * Copyright (C) 2011 Martijn Koedam 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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

#include <stdint.h>

// The maximum size.
const int MAX_SIZE = 1024*1024*16;


// The output is rounded up to complete words.
// Padding is added as zeros. 
// On the target platform memory can only be accessed in words.
const int WORD_SIZE = 4;


int main ( int argc , char **argv )
{
	if(argc != 2) {
		fprintf(stderr, "%s: requires 1 input file.\n", argv[0]);
		return EXIT_FAILURE;
	}


	// Get file size 
	struct stat file_stat;
	int retv = stat(argv[1], &file_stat);

	// Error
	if(retv != 0)
	{
		fprintf(stderr, "%s: Failed to open input file: %s, reason: %s\n", 
				argv[0],
				argv[1],
				strerror(errno));
		return EXIT_FAILURE;
	}
	// Check read permission.
	// size
	if(file_stat.st_size > MAX_SIZE) 
	{
		fprintf(stderr, "%s: File size is to big: %u bytes (limit %u bytes)\n", 
				argv[0],
				(unsigned int) file_stat.st_size,
				MAX_SIZE);
		return EXIT_FAILURE;
	}
	uint32_t file_size = file_stat.st_size;
	uint32_t padding = file_size%WORD_SIZE;
	// Calculate padding 

	padding = (padding > 0)?(WORD_SIZE-padding):0;
	uint32_t total_size = file_size+padding;

	fprintf(stderr, "File size: %u, padding: %u, total: %u\n", file_size, padding, total_size);

	// Write out file size.
	fwrite((void*)&total_size, sizeof(uint32_t),1,stdout);

	// Read the file.
	int fd = open(argv[1], O_RDONLY);

	// Read bytewise.
	unsigned char d;
	while(read(fd, &d,1) == 1)
	{ 
		putc(d, stdout);
		file_size--;
	}
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
	close(fd);
	return EXIT_SUCCESS;
}
