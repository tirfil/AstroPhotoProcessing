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
	return 0;
}

int read_coeff(char* path,double **coeff){
	FILE * fd;
	char *buffer;
	long lsize;
	size_t size;
	char *str;
	int i=0;
	double val;
	double* tab;
	
	tab = *coeff;
	
	fd = fopen (path,"r");
	if (fd==NULL) return -1;
	
	// obtain file size:
	fseek(fd , 0 , SEEK_END);
	lsize = ftell(fd);
	rewind(fd);
	
	buffer = (char*) malloc (sizeof(char)*lsize);
	if (buffer == NULL) return -1;
	size= fread(buffer,1,lsize,fd);
	if (size != lsize) return -1;
	fclose (fd);
	
	str = strtok(buffer,",\n");
	while (str != NULL){
		val = atof(str);
		tab[i] = val;
		printf("coeff[%d] = %f\n",i,val);
		str = strtok(NULL,",\n");
		i++;
	}
	free(buffer);
	return i;
}

int main(int argc, char* argv[]) {
	unsigned short* in;
	unsigned short* out;
	long nelements;
	int i;
	
	double *coeff;
	int nb;
	int x,y;
	int xx,yy;
	
	double result;
	
	unsigned int error = 0;

	

	if (argc != 4){
		printf("Usage: %s <in> <out> <coeff>\n",argv[0]);
		return -1;
	}
	
	coeff = malloc(sizeof(double)*256);
	nb = read_coeff(argv[3],&coeff);
	printf("nb=%d\n",nb);
	if (nb < 0) {
		printf("Read coeff error\n");
		return -1;
	}
	
	if (nb == 9) coeff[9] = 0.0;
	else if (nb != 10) {
		printf("Wrong number of coeff\n");
		return -1;
	}
	
	if (read_fits(argv[1],&in) == 0){
		nelements = width*height;
		out = malloc(sizeof(unsigned short)*nelements);
		for(y=0;y< height;y++)
			for(x=0;x < width;x++){
				result = 0.0;
				i = 0;
				for(yy=y-1;yy<=y+1;yy++)
					for(xx=x-1;xx<=x+1;xx++)
					{
						if(xx<0 || xx>=width || yy<0 || yy>=height){
							result += 0.0;
						} else {
							//printf("%d\n",i);
							result += coeff[i]*(double)in[xx+yy*width];
						}
						i++;
					}
				result += coeff[9];
				if (result < 0.0) {
					result=0.0;
					error++;
				}
				if (result > 65535.0){
					 result = 65535.0;
					 error++;
				}
				out[x+y*width] = (unsigned short)result;
			}
				
		printf("error(s)=%d\n",error);
		remove(argv[2]);
		if (write_fits(argv[2],out) != 0){
			printf("write_fits error\n");
		}
		free(out);

	}
	free(coeff);
}
				
				
		
		
