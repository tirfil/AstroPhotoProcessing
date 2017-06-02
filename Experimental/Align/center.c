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

int xmini, ymini;

unsigned short* a;
unsigned short* b;
unsigned short* aa;
unsigned short* bb;

typedef struct {
	int n;
	int x;
	int y;
	int dx;
	int dy;
} Tpivot;


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
		

int main(int argc, char* argv[]) {

	unsigned short* c;
	long nelements;

	unsigned short factor;
	unsigned short thresholda;
	unsigned short thresholdb;
	unsigned long  ul;
	
	unsigned short mini;
	unsigned short maxi;
	int i;
	int n;
	int na, nb;
	unsigned short us;
	
	Tpivot pivota[4];
	Tpivot pivotb[4];
	
	int iter;
	
	int dx,dy,distance,distmini;
	
	int x,y;
	int x0, y0;
	
	x=0;
	y=0;
	x0=0;
	y0=0;
	
	xmini = 0;
	ymini = 0;
	
	
	if (argc != 5){
		printf("Usage: %s <image1> <image2> <result> <factor>\n",argv[0]);
		return -1;
	}
	
	factor = atoi(argv[4]);
	
	if (read_fits(argv[1],&a) == 0){
		if (read_fits(argv[2],&b) == 0){
			nelements = height * width;
			// detect maxi/mini
			mini = USHRT_MAX;
			maxi = 0;
			for (i=0;i<nelements;i++){
				us = a[i];
				if ( us < mini ) mini = us;
				if ( us > maxi ) maxi = us;
			}
			ul = (unsigned long)(maxi-mini)*(unsigned long)factor;
			thresholda = (unsigned short)(ul/100L)+mini;
			printf("mini=%d - maxi=%d - threshold=%d\n",mini,maxi,thresholda);
			// 
			// detect stars
			aa = malloc(sizeof(unsigned short)*nelements);
			for (i=0;i<nelements;i++){
				aa[i] = 0;
			}
			n = 0;
			for (y=1;y<height-1;y++)
				for(x=1;x<width-1;x++){
					i = y*width+x;
					us = a[i];
					if ( us > thresholda ){
						aa[i] = 1;
						for(y0=y-1;y0<=y+1;y0++){
							for(x0=x-1;x0<=x+1;x0++){
								if (a[y0*width+x0] > us)
								{
									aa[i] = 0;
									break;
								}
							}
						}
						if (aa[i]==1){
							//printf("(%d,%d)\n",x,y);
							n++;
						}
					}			
				}
				
			printf("detect %d/%ld\n",n,nelements);
			// detect maxi/mini
			mini = USHRT_MAX;
			maxi = 0;
			for (i=0;i<nelements;i++){
				us = b[i];
				if ( us < mini ) mini = us;
				if ( us > maxi ) maxi = us;
			}
			ul = (unsigned long)(maxi-mini)*(unsigned long)factor;
			thresholdb = (unsigned short)(ul/100L)+mini;
			printf("mini=%d - maxi=%d - threshold=%d\n",mini,maxi,thresholdb);	
			// detect stars
			bb = malloc(sizeof(unsigned short)*nelements);
			for (i=0;i<nelements;i++){
				bb[i] = 0;
			}
			n = 0;
			for (y=1;y<height-1;y++)
				for(x=1;x<width-1;x++){
					i = y*width+x;
					us = b[i];
					if ( us > thresholdb ){
						bb[i] = 1;
						for(y0=y-1;y0<=y+1;y0++){
							for(x0=x-1;x0<=x+1;x0++){
								if (b[y0*width+x0] > us)
								{
									bb[i] = 0;
									break;
								}
							}
						}
						if (bb[i]==1){
							//printf("(%d,%d)\n",x,y);
							n++;
						}
					}			
				}
			printf("detect %d/%ld\n",n,nelements);	
			
// 			
			pivota[0].x = 0;
			pivota[0].y = 0;
			pivota[1].x = 0;
			pivota[1].y = height;	
			pivota[2].x = width;
			pivota[2].y = 0;					
			pivota[3].x = width;
			pivota[3].y = height;
			// iterate
			for(iter=0; iter<10; iter++){
				//printf("iteration #%d\n",iter);
				// init
				for (i=0; i<4; i++){
					pivota[i].n = 0;
					pivota[i].dx = 0;
					pivota[i].dy = 0;
				}
				for(x=0; x<width; x++){
					for(y=0;y<height;y++){
						if (aa[y*width+x] == 1){
							distmini = width*width + height*height;
							distmini = (int)sqrt((double)distmini);
							for (i=0; i<4; i++){
								dx = x - pivota[i].x;
								dy = y - pivota[i].y;
								distance = dx*dx + dy*dy;
								distance = (int)sqrt((double)distance);
								if ( distance < distmini){
									distmini = distance;
									n = i;
								}
							}
							pivota[n].n += 1;
							pivota[n].dx += x - pivota[n].x;
							pivota[n].dy += y - pivota[n].y;
						}	
					}
				}
				for (i=0; i<4; i++){
					n = pivota[i].n;
					if (n != 0) {
						pivota[i].x += pivota[i].dx/n;
						pivota[i].y += pivota[i].dy/n;
						//printf("%d: #%d x=%d y=%d dx=%d dy=%d\n",i,n,pivota[i].x,pivota[i].y,pivota[i].dx/n,pivota[i].dy/n);
					}
				}
			}

			pivotb[0].x = 0;
			pivotb[0].y = 0;
			pivotb[1].x = 0;
			pivotb[1].y = height;	
			pivotb[2].x = width;
			pivotb[2].y = 0;					
			pivotb[3].x = width;
			pivotb[3].y = height;
			// iterate
			for(iter=0; iter<10; iter++){
				//printf("iteration #%d\n",iter);
				// init
				for (i=0; i<4; i++){
					pivotb[i].n = 0;
					pivotb[i].dx = 0;
					pivotb[i].dy = 0;
				}
				for(x=0; x<width; x++){
					for(y=0;y<height;y++){
						if (bb[y*width+x] == 1){
							distmini = width*width + height*height;
							distmini = (int)sqrt((double)distmini);
							for (i=0; i<4; i++){
								dx = x - pivotb[i].x;
								dy = y - pivotb[i].y;
								distance = dx*dx + dy*dy;
								distance = (int)sqrt((double)distance);
								if ( distance < distmini){
									distmini = distance;
									n = i;
								}
							}
							pivotb[n].n++;
							pivotb[n].dx += x - pivotb[n].x;
							pivotb[n].dy += y - pivotb[n].y;
						}	
					}
				}
				for (i=0; i<4; i++){
					n = pivotb[i].n;
					if (n != 0) {
						pivotb[i].x += pivotb[i].dx/n;
						pivotb[i].y += pivotb[i].dy/n;
						//printf("%d: #%d x=%d y=%d dx=%d dy=%d\n",i,n,pivotb[i].x,pivotb[i].y,pivotb[i].dx/n,pivotb[i].dy/n);
					}
					
				}
			}
			
			maxi = 0;
			for (i=0; i<4; i++){
				n = pivota[i].n;
				if (n) printf("%d: #%d x=%d y=%d dx=%d dy=%d\n",i,n,pivota[i].x,pivota[i].y,pivota[i].dx/n,pivota[i].dy/n);
				if ( n > maxi){
					maxi = n;
					na = i;
				}
			}
			
			maxi = 0;
			for (i=0; i<4; i++){
				n = pivotb[i].n;
				if (n) printf("%d: #%d x=%d y=%d dx=%d dy=%d\n",i,n,pivotb[i].x,pivotb[i].y,pivotb[i].dx/n,pivotb[i].dy/n);
				if ( n > maxi){
					maxi = n;
					nb = i;
				}
			}
			
			xmini = pivotb[nb].x - pivota[na].x;
			ymini = pivotb[nb].y - pivota[na].y;
			
			printf("translate: x:%d y:%d\n",xmini,ymini);
	   
		   // output fits is the image2 translated ( relative to image1)
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
				free(a); free(b); free(c); free(aa); free(bb);
				return 0;
			}
		   
	   }
		   
	}
					
	free(a); free(b); free(c); free(aa); free(bb);
	return -1;
	
}
