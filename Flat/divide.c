#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include "fitsio.h"

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
}

int main(int argc, char* argv[]) {
	unsigned short* a;
	unsigned short* b;
	unsigned short* c;
	long nelements;
	int i;
	int errors = 0;
	unsigned long lint_flat=0L;
	unsigned long lint_light=0L;
	unsigned short mini_flat=65535;
	unsigned short maxi_flat=0;
	unsigned short mini_light=65535;
	unsigned short maxi_light=0;
	unsigned short average_flat;
	unsigned short average_light;
	unsigned short delta_flat;
	unsigned short delta_light;
	unsigned short average;
	unsigned long factor;
	
	if (argc != 4){
		printf("Usage: %s <raw> <flat> <result>\n",argv[0]);
		return -1;
	}
	
	if (read_fits(argv[1],&a) == 0){
		if (read_fits(argv[2],&b) == 0){	
			nelements = width*height;
			c = malloc(sizeof(unsigned short)*nelements);
			// compute average value
			for(i=0;i<nelements;i++){
				lint_light += a[i];
				if (a[i] > maxi_light) maxi_light=a[i];
				if (a[i] < mini_light) mini_light=a[i];
			}
			for(i=0;i<nelements;i++){
				lint_flat += b[i];
				if (b[i] > maxi_flat) maxi_flat=b[i];
				if (b[i] < mini_flat) mini_flat=b[i];
			}
			average_flat = (unsigned short)(lint_flat/(unsigned long)nelements);
			average_light = (unsigned short)(lint_light/(unsigned long)nelements);
			printf("flat : average= %d min=%d max=%d\n",average_flat,mini_flat,maxi_flat);
			printf("light: average= %d min=%d max=%d\n",average_light,mini_light,maxi_light);
			delta_flat = maxi_flat - mini_flat;
			delta_light = maxi_light - mini_light;
			// algo 4
			factor = 65535L*(unsigned long)mini_flat/(unsigned long)maxi_light;
			lint_flat = (unsigned long)mini_light*factor/(unsigned long)maxi_flat;
			printf("factor=%ld min=%ld",factor,lint_flat);
			for(i=0;i<nelements;i++){
				lint_flat = (unsigned long)a[i]*factor/(unsigned long)b[i];
				c[i] = (unsigned short) lint_flat;
			}
			// algo 1
			// 
			// -> average_flat/flat_pixel * light_pixel
			//
			//for(i=0;i<nelements;i++){
				//lint_flat = (unsigned long)a[i]*(unsigned long)average_flat;
				//if ( b[i] != 0){
					//c[i] = (unsigned short)(lint_flat/(unsigned long) b[i]);
				//} else {
					//c[i] = 65535;
				//}
			//}
			// algo 2
			//
			// 
			//average = maxi_flat/2 + mini_flat/2;
			//for(i=0;i<nelements;i++){
				//lint_flat = (unsigned long)a[i]*(unsigned long)average;
				//if ( b[i] != 0){
					//c[i] = (unsigned short)(lint_flat/(unsigned long) b[i]);
				//} else {
					//c[i] = 65535;
				//}
			//}
			// algo 3
			//
			//
			//for(i=0;i<nelements;i++){
				//lint_flat = ((unsigned long)(maxi_flat - b[i]) * (unsigned long)(a[i] - mini_light));
				//lint_flat = lint_flat /(unsigned long) delta_flat + (unsigned long)mini_light;
				//c[i] = (unsigned short) lint_flat;
			//}
			remove(argv[3]);
			if (write_fits(argv[3],c) == 0){
				free(a); free(b); free(c);
				return 0;
			}
		}
	}
	free(a); free(b); free(c);
	return -1;
	
}
