#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include "fitsio.h"

int width;
int height;

int write_fits(char* path,unsigned short* image)
{
	fitsfile *fptrout;
	unsigned int naxis = 2;
    long naxes[2];
    int status = 0;
    long nelements = width*height;
    naxes[0] = width;
	naxes[1] = height;
	fits_create_file(&fptrout,path, &status);
	fits_create_img(fptrout, USHORT_IMG, naxis, naxes, &status);
	fits_write_img(fptrout, TUSHORT, 1, nelements, image, &status);
	fits_close_file(fptrout, &status);
}

int main(int argc, char* argv[]) {
	

	int number;
	int interval;
	unsigned short** root;
	long nelements;
	int i,j,k;
	unsigned short value;
	char filename[256];
	unsigned short* image;
	int r;
	
	if (argc != 4){
		printf("Usage: %s <width> <height> <number of fits>\n",argv[0]);
		return -1;
	}
	
	width  = atoi(argv[1]);
	height = atoi(argv[2]);
	number = atoi(argv[3]);
	
	printf("generate %d %dx%d fits\n",number,width, height);
	
	interval = 65535/number;
	//interval = 65536/2/number;
	
	nelements = width*height;
	
	printf("interval is %d\n",interval);
	
	// init
	printf("init\n");
	root = malloc(sizeof(unsigned short*)*nelements);
	
	for(i=0; i<nelements; i++){
		root[i] = malloc(sizeof(unsigned short)*number);
		value = 0;
		for(j=0; j<number; j++){
			root[i][j] = value;
			value += interval;
		}
	}
		
	// create noisy fits files
	printf("create fits\n");
	srand(time(NULL));
	for(k=0; k<number; k++){
		sprintf(filename,"test%03d.fits",k);
		printf("create fits %s\n",filename);
		image = malloc(sizeof(unsigned short)*nelements);
		for(i=0; i<nelements; i++){
			r = rand() % (number - k);
			image[i] = root[i][r];
			if (r < (number -k - 1))
				root[i][r] = root[i][number -k - 1];
		}
		write_fits(filename,image);
		free(image);
	}
	// free memory
	for(i=0; i<nelements; i++){
		free(root[i]);
	}
	free(root);
}
