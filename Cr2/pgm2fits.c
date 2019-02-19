#include <string.h>
#include <stdio.h>
#include "fitsio.h"

#define SEP 0x0A
#define SPACE 0x20

int width = 0;
int height = 0;


int write_fits(char* path,unsigned short* image)
{
	fitsfile *fptrout;
	unsigned int naxis = 2;
    long naxes[2];
    int status = 0;
    long nelements = width*height;
    naxes[0] = (long)width;
	naxes[1] = (long)height;
	fits_create_file(&fptrout,path, &status);
	fits_create_img(fptrout, USHORT_IMG, naxis, naxes, &status);
	fits_write_img(fptrout, TUSHORT, 1, nelements, image, &status);
	fits_close_file(fptrout, &status);
}


int main(int argc, char *argv[])
{
	FILE* fd;
	int maxi = 0;
	unsigned char c;
	int size;
	unsigned short* buffer;
	int i,j,k;
	
	if(argc != 3){
			printf("Usage:\n%s <16bit.pgm> <out.fits>\n",argv[0]);
			return 0;
	}
	
	fd = fopen(argv[1],"r");
	if (fd == NULL) {
		printf("File %s doesn't exist\n",argv[1]);
		exit(1);
	}
	if (('P' != fgetc(fd)) || ('5' != fgetc(fd)) || (SEP != fgetc(fd))) {
		printf("Wrong file header\n");
		exit(1);
	}
	c = fgetc(fd);
	while ( c != SPACE) {
		width = width * 10 + (c - '0');
		c = fgetc(fd);
	}
	printf("width=%d\n",width);
	c = fgetc(fd);
	while ( c != SEP) {
		height = height * 10 + (c - '0');
		c = fgetc(fd);
	}
	printf("height=%d\n",height);
	c = fgetc(fd);
	while ( c != SEP) {
		maxi = maxi * 10 + (c - '0');
		c = fgetc(fd);
	}	
	if (maxi != 65535) {
		printf("Not 16 bit resolution\n");
		exit(1);
	}
	size = width*height;
	
	buffer = (unsigned short*) malloc(size*sizeof(unsigned short));
	
	for(i=height-1;i>=0;i--){			// y mirror
		for(j=0;j<width;j++) {	
			k = i*width + j;
			buffer[k] = 256 * (unsigned short)fgetc(fd); // msb
			buffer[k] += (unsigned short)fgetc(fd); //lsb
		}
	}
	
	fclose(fd);
	
	write_fits(argv[2],buffer);
	
	free(buffer);
	

};
	
	
		
	
