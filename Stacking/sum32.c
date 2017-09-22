#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include "fitsio.h"


long width=0;
long height=0;
unsigned int *imageout = NULL;

const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

void process_input_fits(char* path){
	fitsfile *fptr;    
    int status = 0;
    unsigned int bitpix, naxis;
    long naxes[2];
    long nelements;
    int anynul;
    unsigned short *image;
    unsigned int tmp;
    int i;

    if (!fits_open_file(&fptr, path, READONLY, &status))
    {
		if (!fits_get_img_param(fptr, 2, &bitpix, &naxis, naxes, &status) )
        {
			printf("bitpix=%d naxes=%d width=%ld height=%ld\n",bitpix,naxis,naxes[0],naxes[1]);
			nelements = naxes[0]*naxes[1];
			if (width==0 && height==0){
				width = naxes[0];
				height = naxes[1];
				imageout = malloc(sizeof(unsigned int)*nelements);
				for (i=0; i<nelements; i++){
					imageout[i] = 0;
				}
				
			} else {
				if (width != naxes[0] || height != naxes[1]){
					printf("naxes error: %ldx%ld instead of %ldx%ld\n",naxes[0],naxes[1],width,height);
					return;
				}
			}
			
			image = malloc(sizeof(unsigned short)*nelements);
			
			if (bitpix == 16){
				fits_read_img(fptr,TUSHORT,1,nelements,NULL,image, &anynul, &status);
				for (i=0; i<nelements; i++){
					tmp = imageout[i];
					tmp += (unsigned int)image[i];
					imageout[i]=tmp;
				}
			}
			// close and free
			free(image);
			fits_close_file(fptr, &status);
		}
	}
}

void write_output_fits(char* filename){
	int i,j;
	fitsfile *fptrout;
	unsigned int naxis = 2;
    long naxes[2];
    int status = 0;
	long nelements = width*height;
	naxes[0] = width;
	naxes[1] = height;
	fits_create_file(&fptrout,filename, &status);
	fits_create_img(fptrout, ULONG_IMG, naxis, naxes, &status);
	fits_write_img(fptrout, TUINT, 1, nelements, imageout, &status);
	fits_close_file(fptrout, &status);
	free(imageout);
}

int main(int argc, char* argv[]) {
	
	char* directory;
	char* output_file;
	DIR * dirp;
    struct dirent * entry;
    char path[256];
    int file_count;

	
	if (argc != 3){
		printf("Usage: %s <directory path> <output fits file>\n",argv[0]);
		return -1;
	}
	
	directory = argv[1];
	output_file = argv[2];

	file_count = 0;
	
	
	// count and parse entries
	dirp = opendir(directory);
	if (dirp != NULL)
		while ((entry = readdir(dirp)) != NULL)
			if (entry->d_type == DT_REG)
				if ( strncmp("fits", get_filename_ext(entry->d_name), 4) == 0 ) {
					sprintf(path,"%s/%s",directory,entry->d_name);
					process_input_fits(path);
					file_count++;
				}
				
	printf("Processing %d fits file(s)\n",file_count);
				
	write_output_fits(output_file);
	
	return 0;
}
	
	
