#undef _GNU_SOURCE // gets rid of vim warning
#define _GNU_SOURCE

#include "io.h"
#include <math.h>
#include <stdio.h> 
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/*
 * Parses command line arguments.
 */
int parse_args(
		int argc,
	   	char *argv[],
	   	char *filepath,
	   	char *outputfilepath,
	   	sim_params *params,
		bool *ascii_encoding
) {
	int c;
	while((c = getopt(argc, argv, "af:o:n:t:r:e:c:g:v:s:d:m:")) != -1){
		switch(c){
			case 'f':	
				strncpy(filepath, optarg, FILEPATH_MAXLEN);
				break;
			case 'o':
				strncpy(outputfilepath, optarg, FILEPATH_MAXLEN);
				break;
			case 'a':
				*ascii_encoding = true;
				break;
			case 't': params->ttl = atoi(optarg); break;
			case 'n': params->n = atoi(optarg); break;
			case 'r': params->p_radius = atoi(optarg); break;
			case 'e': params->p_enertia = atof(optarg); break;
			case 'c': params->p_capacity = atof(optarg); break;
			case 'g': params->p_gravity = atof(optarg); break;
			case 'v': params->p_evaporation = atof(optarg); break;
			case 's': params->p_erosion = atof(optarg); break;
			case 'd': params->p_deposition = atof(optarg); break;
			case 'm': params->p_min_slope = atof(optarg); break;
			default:
				fprintf(stderr, "Usage: %s\n", argv[0]);
				return 1;
		}	
	}

	return 0;
}

/*
 * Loads *.pgm into buffer `buffer`. `buffer` is dynamically allocated in 
 * load_pgm and should be free'd after use.
 */
int load_pgm(
		const char *filepath,
		image *img,
		int *precision
) {
	FILE	*fp = fopen(filepath, "r");
	char	*token;
	char	*line = NULL;
	char	*magic;
	size_t	len = 0;
	int		data_offset = 0;

	if(fp == NULL)
		return 1;
	
	// read width, height and precision
	// TODO rewrite properly
	if (getline(&magic, &len, fp) == EOF) return 1; // magic
	data_offset += len;
	if (getline(&line, &len, fp) == EOF) return 1; // comment
	data_offset += len;
	if (getline(&line, &len, fp) == EOF) return 1; // width height
	data_offset += len;
	token   = strtok(line, " ");
	img->width  = atoi(token);
	token   = strtok(NULL, " ");
	img->height = atoi(token);
	if (getline(&line, &len, fp) == EOF) return 1; // precision
	data_offset += len;
	*precision = atoi(line);
	
	// Allocate buffer for pixel values
	img->buffer = malloc(sizeof(double) * (img->height) * (img->width));
	double *buffer = (double *) img->buffer;
	if(buffer == NULL)
		return 1;

	// Read pixel values to buffer. 
	// If magic is "P2" then values are ASCII encoded. 
	// If magic is "P5" then values are binary encoded. 
	if(strncmp(magic, "P2", 2) == 0){
		for(int i = 0; getline(&line, &len, fp) != EOF; i++)
			buffer[i] = atof(line) / *precision;
	} else if(strncmp(magic, "P5", 2) == 0) {
		int byte_depth = *precision < 256 ? 1 : 2; 
		char data[byte_depth];
		for(int i = 0; i < img->width * img->height; i++) {
			fread(data, sizeof(char), byte_depth, fp);
			int val = (byte_depth == 2) ? 
				((data[0] << 8) & 0xFF00) | (data[1] & 0x00FF) :
				data[0] & 0xFF;
			buffer[i] = (double) val / *precision;
		}
	}

	// cleanup
	fclose(fp);
	if(line)
		free(line);
	if(magic)
		free(magic);

	return 0;
}

/*
 * Saves buffer `buffer` to a file.
 */
int save_pgm(
		const char *filepath,
	   	image *img,
		int precision,
		bool ascii_encoding
) {
	FILE *fp = fopen(filepath, "w");

	// write "header"
	fputs((ascii_encoding) ? "P2\n" : "P5\n", fp);	
	fputs("# Generated by erodr\n", fp);
	fprintf(fp, "%d %d\n", img->width, img->height);	
	fprintf(fp, "%d\n", precision);	

	double *buffer = (double *)img->buffer;
	int byte_depth = precision < 256 ? 1 : 2; 
		
	if (ascii_encoding) {	
		for(int i = 0; i < img->width * img->height; i++) {
			fprintf(fp, "%d\n", (int)round(buffer[i]*precision));	
		}
	} else {
		char *data = malloc(byte_depth * img->width * img->height);
		for(int i = 0; i < img->width * img->height; i++) {
			int gv = (uint16_t)round(buffer[i]*precision);
			data[2 * i]	= (gv >> 8) & 0xFF;
			data[2 * i + 1]	= gv & 0xFF;
		}
		fwrite(data, byte_depth, img->width * img->height, fp); 
	}	
	fclose(fp);
	
	return 0;
}

