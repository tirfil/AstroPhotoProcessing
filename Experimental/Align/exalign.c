#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include "fitsio.h"
#include <limits.h>

long width=0;
long height=0;

int xmin, xmax, ymin, ymax;

unsigned short* a;
unsigned short* b;

unsigned long mini = ULONG_MAX;
int xmini, ymini;

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
				if (status != 0) printf("status = %d\n",status);
				return status;
			} 
			
			if (bitpix == 8){
				unsigned char* image8;
				int i;
				image8 = malloc(sizeof(unsigned char)*nelements);
				fits_read_img(fptr,TBYTE,1,nelements,NULL,image8, &anynul, &status);
				fits_close_file(fptr, &status);
				if (status != 0) printf("status = %d\n",status);
				// convert from 8 bit to 16 bit
				for(i=0; i < nelements; i++)
					(*image)[i]=((unsigned short)image8[i]);
				return status;
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

void check(int x0, int y0){
	unsigned long sum = 0L;
	int diff;
	int x,y;
	//int xmin, xmax, ymin, ymax;
	// use image center to compute difference
	//xmin = width/3;
	//xmax = 2*width/3;
	//ymin = height/3;
	//ymax = 2*height/3;
	for(x=xmin; x<xmax; x++)
		for(y=ymin;y<ymax;y++){
			// compute difference between image1 and image2 translated (in x and y axis)
			diff = a[x+y*width] - b[x+x0+(y+y0)*width];
		    sum += (unsigned long)diff*(unsigned long)diff;
		}
    //printf("sum = %ld x0=%d y0=%d\n",sum,x0,y0);		
	if (sum < mini){
		// current difference minimum
		mini = sum;
		xmini = x0;
		ymini = y0;
		printf("mini = %ld x0=%d y0=%d\n",mini,xmini,ymini);
	}
		
}

int main(int argc, char* argv[]) {

	unsigned short* c;
	long nelements;


	
	int x,y;
	int x0, y0;
	int pivot;
	
	x=0;
	y=0;
	x0=0;
	y0=0;
	pivot=1;
	
	
	if (argc != 8){
		printf("Usage: %s <image1> <image2> <result> <xmin%> <xmax%> <ymin%> <ymax%>\n",argv[0]);
		return -1;
	}
	
	xmin = atoi(argv[4]);
	xmax = atoi(argv[5]);
	ymin = atoi(argv[6]);
	ymax = atoi(argv[7]);
	
	if (read_fits(argv[1],&a) == 0){
		//
		xmin = (xmin * width)/100;
		xmax = (xmax * width)/100;
		ymin = (ymin * height)/100;
		ymax = (ymax * height)/100;
		
		// add gap
		if (xmin < 10) xmin=10;
		if (xmax > (width-10)) xmax=width-10;
		if (ymin < 10) ymin=10;
		if (ymax > (height-10)) ymax=height-10;
		
		printf("x: %d-%d y: %d-%d\n",xmin,xmax,ymin,ymax);
		//
		if (read_fits(argv[2],&b) == 0){
			// try to align both images using x and y translation (spiral move)
			for(pivot=1; pivot<10; pivot++){
				for( x=x0; x < pivot; x++) check(x,y);
				for( y=y0; y < pivot; y++) check(x,y);
				for( x=pivot; x > -1*pivot; x--) check(x,y);
				for( y=pivot; y > -1*pivot; y--) check(x,y);
				x0 = x;
				y0 = y;
				pivot++;
		   }
		   
		   printf("=>%ld (%d,%d)\n",mini,xmini,ymini);
		   
		   // output fits is the image2 translated ( relative to image1)
		   nelements = height * width;
		   c = malloc(sizeof(unsigned short)*nelements);
		   for(y=0;y< height;y++)
			for(x=0;x< width; x++){
				x0=x+xmini;
				y0=y+ymini;
				if ( ( x0 >= width ) || ( x0 < 0) ||  ( y0 >= height ) || (y0 < 0) ){
						c[x+y*width] = 0;
				} else {
						c[x+y*width] = b[x0+y0*width];
				}
			}
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
