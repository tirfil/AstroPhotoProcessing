#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include "fitsio.h"


typedef struct ushort_node ushort_node;

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
	fits_create_img(fptrout, USHORT_IMG, naxis, naxes, &status);
	fits_write_img(fptrout, TUSHORT, 1, nelements, image, &status);
	fits_close_file(fptrout, &status);
	return 0;
}


int main(int argc, char* argv[]) {
	unsigned short* in;
	//unsigned short* out;
	long nelements;
	int i;
	unsigned short tmp;

    unsigned short mini, maxi;
	unsigned long sum, sum2;

	unsigned short average;
	unsigned long variance;
	double stddev;
	
	double factor;
	
	unsigned short low;
	unsigned short high;
	
	if (argc != 4){
		printf("Usage: %s <in> <out> <sigma>\n",argv[0]);
		return -1;
	}
	
	if (read_fits(argv[1],&in) == 0){
		nelements = width*height;
		//out = malloc(sizeof(unsigned short)*nelements);
		
		// compute mini maxi sum sum2
		mini = USHRT_MAX;
		maxi = 0;
		sum = 0L;
		sum2 = 0L;
		
		for (i=0; i<nelements; i++){
			tmp = in[i];
			if( tmp > maxi) maxi = tmp;
			if( tmp < mini) mini = tmp;
			sum  += (unsigned long) tmp;
			sum2 += (unsigned long) tmp * (unsigned long) tmp;
		}
		
		printf("----------------------------------------\n");			
		printf("minimun = %d\n",mini);
		printf("maximum = %d\n",maxi);
		//printf("\n");
		printf("----------------------------------------\n");
		average = (unsigned short)(sum / (unsigned long) nelements);
		variance = (sum2 / (unsigned long)nelements) - (unsigned long)average * (unsigned long)average;
		stddev = sqrt((double)variance);
		
		printf("average = %d\n",average);
		printf("standard deviation = %.2f\n",stddev);		
		
		factor = atof(argv[3]);
		
		printf("sigma = %f\n",factor);
		factor = factor*stddev;
		
		if (factor > (double)maxi)
			factor = (double)maxi;
			
		printf("factor= %f\n",factor);
		
		if (average < (unsigned short)factor){
			low = mini;
		} else {
			low = average - (unsigned short)factor;
		}
		
		if (average > (maxi - (unsigned short)factor)){
			high = maxi;
		} else {
			high = average + (unsigned short)factor;
		}
		
		printf("range: %d - %d\n",low,high);
		
		for (i=0; i<nelements; i++){
			tmp = in[i];
			if (tmp < low) in[i]=low;
			if (tmp > high) in[i]=high;
		}
		
		remove(argv[2]);
		if (write_fits(argv[2],in) != 0){
			printf("write_fits error\n");
		}
		free(in);

	}
}
				
				
		
		
