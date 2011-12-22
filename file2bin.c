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

// uint32_t and  conversion
#include <stdint.h>
#include <endian.h>

// The maximum size we support. Currently 16MB should be sufficient.
const unsigned int MAX_SIZE = 1024*1024*16;

// The output is rounded up to complete words.
// Padding is added as zeros. 
// On the target platform memory can only be accessed in words.
// DO NOT CHANGE!!!
const unsigned int WORD_SIZE = 4;

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))
#define FALSE	 0
#define TRUE	 1

typedef struct _file {
	// Pointer to (non-free-able) filename
	const char *filename;
	// Size of file.
	uint32_t    size;
	// Size of padding;
	uint32_t    padding;
} file;

// Free array of file elements and set pointer to NULL
static void free_files(file ***files)
{
	// Check valid  pointer.
	if(files == NULL) return;
	if(*files != NULL)
	{
		for(unsigned int i = 0; (*files)[i] != NULL; i++)
		{
			free((*files)[i]);
			(*files)[i] = NULL;
		}	
		free(*files);
	}
	// Set pointer NULL
	*files = NULL;
}

static void print_help(char **argv)
{
	fprintf(stderr, "Usage: %s <options> <input files>\n\n", argv[0]);
	fprintf(stderr, "%s packs all given input files into one binary stream. Each file is padded to be word aligned.\n", argv[0]);
	fprintf(stderr, "Prepended is the size of the stream in a 32bit big endian unsigned int.\n");
	fprintf(stderr, "Optinally a TOC is added to the start of the bit stream.\n");
	fprintf(stderr, "The maximum output size is currently limited to: %u bytes.\n", MAX_SIZE);
	fprintf(stderr, "\n");
	fprintf(stderr, "\t-t\t--toc\tOutput a Table of content\n");
	fprintf(stderr, "\t-h\t--help\tThis help message\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "TOC spec:\n");
	fprintf(stderr, "\tunsigned int 32bit be:  <Number of files: n>\n");
	fprintf(stderr, "\tunsigned int 32bit be:  <offset file 1>\n");
	fprintf(stderr, "\tunsigned int 32bit be:  <offset file 2>\n");
	fprintf(stderr, "\t.....\n");
	fprintf(stderr, "\tunsigned int 32bit be:  <offset file n>\n");
}

int main ( int argc , char **argv )
{
	// Total size we are going transmit (without size header)
	uint32_t total_size = 0;
	unsigned int number_files = argc;
	file 			**files = NULL;
	unsigned int 	num_files = 0;
	short int		output_toc = FALSE;
	short int		output_total_size = TRUE;


	for(unsigned int current_file = 1; current_file < number_files;current_file++)
	{

		// Commandline option parsing.

		// Check help message.
		if(strcmp(argv[current_file], "-h") == 0 || strcmp(argv[current_file], "--help")==0)
		{
			print_help(argv);
			// Cleanup
			free_files(&files);
			return EXIT_SUCCESS;
		}
		else if(strcmp(argv[current_file], "-t") == 0 || strcmp(argv[current_file], "--toc") == 0)
		{
			output_toc = TRUE;
			continue;
		}
		// No input argument, assume file.

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
			free_files(&files);
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
			free_files(&files);
			return EXIT_FAILURE;
		}
		uint32_t file_size = file_stat.st_size;
		uint32_t padding = file_size%WORD_SIZE;
		// Calculate padding 
		padding = (padding > 0)?(WORD_SIZE-padding):0;
		total_size += file_size+padding;

		// Add element to list
		files 						= realloc(files, (num_files+2)*sizeof(file*));
		files[num_files] 			= malloc(sizeof(file));
		files[num_files]->filename	= argv[current_file];
		files[num_files]->size 		= file_size;
		files[num_files]->padding	= padding;

		// NULL terminated array
		files[num_files+1] = NULL;
		num_files++;

		fprintf(stderr, "File size: %u, padding: %u, total: %u\n", file_size, padding, total_size);
	}
	// Check total size
	if(total_size > MAX_SIZE) {
			fprintf(stderr, "%s: File size is to big: %u bytes (limit %u bytes)\n", 
					argv[0],
					(unsigned int)total_size, 
					MAX_SIZE);
			free_files(&files);
			return EXIT_FAILURE;
	}

	// Include toc in total size.
	// Toc table is 1 word 'num files' and 
	// one offset word for each word.
	if(output_toc)
	{
		total_size += (num_files+1)*4;
	}

	// Prepend total size of image. (excluding this)
	if(output_total_size)
	{
		// Explicitely convert to bit.	
		uint32_t le_tsz = htobe32(total_size);
		// Write out total file size.
		fwrite((void*)&le_tsz, sizeof(uint32_t),1,stdout);
	}

	if(output_toc)
	{
		// OUTPUT a toc.
		// All fields are 32bit unsigned int big endian.
		// <number of entries n>
		// <offset file 1>
		// .....
		// <offset file n>
		uint32_t toc_offset = (num_files+1)*4;
		uint32_t temp_be;
		// Number of entries in the toc.
		temp_be = htobe32(num_files);
		fwrite((void*)&temp_be, sizeof(uint32_t),1,stdout);
		// sizes
		fprintf(stderr, "====== TOC =====\nsize: %u\n", num_files);
		for(unsigned int i = 0; i < num_files; i++)
		{
			// Write file offset.
			temp_be = htobe32(toc_offset);
			fwrite((void*)&temp_be, sizeof(uint32_t),1,stdout);
			fprintf(stderr, "%04d: 0x%08X: %s\n", i+1, toc_offset, files[i]->filename);
			// increment offset.
			toc_offset += files[i]->size+files[i]->padding; 
		}
		fprintf(stderr, "================\n");
	}

	// Write out each file
	for(unsigned int i = 0; i < num_files; i++) 
	{
		// Get file size 
		// Check read permission.
		uint32_t file_size = files[i]->size; 

		// Read the file.
		int fd = open(files[i]->filename, O_RDONLY);
		if(fd < 0)
		{
			fprintf(stderr, "%s: Failed to open input file: %s, reason: %s\n", 
					argv[0],
					files[i]->filename,
					strerror(errno));
			free_files(&files);
			return EXIT_FAILURE;
		}

		// Read bytewise. Spit out integer.
		// This union allows us to quicky read in bytes, 
		// But work on it as a integer.
		union {
			uint32_t d;
			uint8_t  a[WORD_SIZE]; 
		}d;

		// Read complete words.
		while(file_size > 0)
		{
			unsigned int nread = 0;
			// RESET
			d.d =0;
			while(nread != MIN(WORD_SIZE, file_size)){
				nread+=read(fd, &(d.a)[nread], WORD_SIZE-nread);
			}
			// Write
			fwrite(&(d.d), WORD_SIZE, 1, stdout);
			// substract.
			file_size-=MIN(WORD_SIZE,file_size);
		}
		close(fd);
	}
	// Cleanup
	free_files(&files);
	// Return success
	return EXIT_SUCCESS;
}
