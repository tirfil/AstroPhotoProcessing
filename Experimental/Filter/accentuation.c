#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include <math.h>
#include "fitsio.h"

#define M_PI 3.14159265358979323846

#define RADIUS 2
#define MULT 2
#define RHO 1.2


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

int testxy(int x,int y)
{
	if (x < 0) return -1;
	if (y < 0) return -1;
	if (x >= width) return -1;
	if (y >= height) return -1;
	return 0;
}

int create_gauss_filter(double* filter, int size)
{
	double div1;
	double div2;
	int x,y;
	int i;
	double sum;
	
	sum = 0.0;
	
	div2 = 2.0*RHO*RHO;
	div1 = div2 * M_PI;
	
	i=0;
	
	for (y=-size;y<=size;y++)
		for (x=-size;x<=size;x++){
			filter[i] = exp(-1.0*(x*x+y*y)/div2)/div1;
			printf("g(%d,%d) =\t%f\n",x,y,filter[i]);
			sum += filter[i];
			i++;
		}
		
	printf("sum=%f\n",sum); 
		
	return 0;
	
}


int main(int argc, char* argv[]) {
	unsigned short* in;
	unsigned short* lowpass;
	unsigned short* out;
	long nelements;
	int i;
	int x,y;
	int dx,dy;
	int index;
	//int x0,y0;
	double* filter;
	double sum,n,value;

	if (argc != 3){
		printf("Usage: %s <in> <out> \n",argv[0]);
		return -1;
	}
	
	if (read_fits(argv[1],&in) == 0){
		nelements = width*height;
		filter = malloc(sizeof(double)*(2*RADIUS+1)*(2*RADIUS+1));
		
		create_gauss_filter(filter,RADIUS);
		
		
		out = malloc(sizeof(unsigned short)*nelements);
		lowpass = malloc(sizeof(unsigned short)*nelements);
		
		for(y=0;y< height;y++)
			for(x=0;x < width;x++){
				n = 0.0;
				sum = 0.0;
				for(dy=-RADIUS;dy<=RADIUS;dy++)
					for(dx=-RADIUS;dx<=RADIUS;dx++){
						if (testxy(x+dx,y+dy) < 0) continue;
						index = dx+RADIUS +(dy+RADIUS)*(2*RADIUS+1);
						value = filter[index];
						sum += value*(double)in[x+dx+(y+dy)*width];
						n += value;
					}
				if (n>0.0) 
					lowpass[x+y*width] = (int)floor(sum/n+0.5);
				else
					lowpass[x+y*width] = 0;
			}
			
		free(filter);
		
			
		for(i=0; i < nelements; i++)
		{
			n = (in[i] - lowpass[i])*MULT;
			n += in[i];
			if (n>0) 
				out[i]=n;
			else 
				out[i]=0;
		}
		

	}
		remove(argv[2]);
		if (write_fits(argv[2],out) != 0){
			printf("write_fits error\n");
		}
		free(lowpass);
		free(out);

	}

				
				
		
		
