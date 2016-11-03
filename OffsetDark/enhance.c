#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include "fitsio.h"
#include <math.h>

long width=0;
long height=0;

int read_fits(char* path, unsigned short** image)
{
	fitsfile *fptr;    
    int status = 0;
    unsigned int bitpix, naxis;
    long naxes[2];
    long nelements;
    int anynul;
	if (!fits_open_file(&fptr, path, READONLY, &status))
    {
		if (!fits_get_img_param(fptr, 2, &bitpix, &naxis, naxes, &status) )
        {
			printf("bitpix=%d naxes=%d width=%ld height=%ld\n",bitpix,naxis,naxes[0],naxes[1]);
			nelements = naxes[0]*naxes[1];
			if (width==0 && height==0){
				width = naxes[0];
				height = naxes[1];
			} else {
				if (width != naxes[0] || height != naxes[1]){
					printf("naxes error: %ldx%ld instead of %ldx%ld\n",naxes[0],naxes[1],width,height);
					return -1;
				}
			}
			
			*image = malloc(sizeof(unsigned short)*nelements);
			
			if (bitpix == 16){
				fits_read_img(fptr,TUSHORT,1,nelements,NULL,*image, &anynul, &status);
				fits_close_file(fptr, &status);
				return status;
			} else {
				fits_close_file(fptr, &status);
				return -1;
			}
		  }
     }
     return -1;		
}

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
	fits_create_img(fptrout, SHORT_IMG, naxis, naxes, &status);
	fits_write_img(fptrout, TUSHORT, 1, nelements, image, &status);
	fits_close_file(fptrout, &status);
	return 0;
}

int main(int argc, char* argv[]) {
	unsigned short* in;
	unsigned short* out;
	long nelements;
	int i;
	unsigned short mini, maxi;
	unsigned long sum, sum2;
	int offset;
	double factor;

	
	mini = 65535;
	maxi = 0;
	sum = 0L;
	sum2 = 0L;
	
	unsigned short tmp;
	unsigned short average;
	unsigned long variance;
	double stddev;
	int itmp;
	unsigned int error = 0;
	

	if (argc == 1 || argc > 5){
		printf("Usage: %s <in> [ <out> [ <offset> [ <factor> ]]\n",argv[0]);
		return -1;
	}
	
	if (read_fits(argv[1],&in) == 0){
		nelements = width*height;
		for(i=0;i<nelements;i++){
			tmp = in[i];
			if( tmp > maxi) maxi = tmp;
			if( tmp < mini) mini = tmp;
			sum  += (unsigned long) tmp;
			sum2 += (unsigned long) tmp * (unsigned long) tmp;
		}
		
		printf("minimun = %d\n",mini);
		printf("maximum = %d\n",maxi);
		printf("\n");
		
		average = (unsigned short)(sum / (unsigned long) nelements);
		variance = (sum2 / (unsigned long)nelements) - (unsigned long)average * (unsigned long)average;
		stddev = sqrt((double)variance);
		
		printf("average = %d\n",average);
		printf("standard deviation = %f\n",stddev);
		
		printf("----------------------------------------\n");
		
		factor = 65535.0/(double)(maxi-mini);
		offset = -1*mini;
		
		printf("suggested factor = %f\n",factor);
		printf("suggested offset = %d\n",offset);
		
		printf("----------------------------------------\n");
		
		if (argc > 2){
			out = malloc(sizeof(unsigned short)*nelements);
			if (argc > 3){
				offset = atoi(argv[3]);
				if (argc == 5)
					factor = atof(argv[4]);
				else
					factor = 1.0;
			}
			printf("factor = %f\n",factor);
			printf("offset = %d\n",offset);
			for(i=0;i<nelements;i++){
				itmp = offset + (int)( factor*(double)in[i]);
				if (itmp > 65535){
					itmp = 65535;
					error++;
				}
				if (itmp < 0){
					itmp = 0;
					error++;
				}
				out[i] = (unsigned short)itmp;
			}
			printf("%d error(s)\n",error);
			remove(argv[2]);
			if (write_fits(argv[2],out) != 0){
				printf("write_fits error\n");
			}
			free(out);
		}
	}
}
				
				
		
		
