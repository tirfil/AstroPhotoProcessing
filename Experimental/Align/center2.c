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

typedef struct {
	int x;
	int y;
} Px;


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

int get_max(int*a,int na,int* upper){
	int i,ii;
	int n,maxi;
	maxi = 0;
	ii = -1;
	for(i=0;i<na;i++){
		n = a[i];
		if ((maxi < n) && (n < *upper)){
			maxi = n;
			ii = i;
		}
	}
	*upper=maxi;
	return ii;	
}

int get_index(int*a,int na,int value){
	int i;
	int n;
	for(i=0;i<na;i++){
		n = a[i];
		if(n == value) return i;
	}
	for(i=0;i<na;i++){
		n = a[i];
		if(n == value+1) return i;
		if(n == value-1) return i;
	}
	return -1;
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
	int i,j,k,l;
	int n;
	int na, nb;
	unsigned short us;
	int va,vb,vc,size,index;
	
	Tpivot pivota[4];
	Tpivot pivotb[4];
	
	Px* pxa;
	Px* pxb; 
	
	int* a2;
	int* b2;
	
	int iter;
	
	int dx,dy,distance,distmini;
	
	int x,y;
	int x0, y0;
	int x1, y1;
	
	int upper;
	
	int lista[10];
	int listb[10];
	
	int pointa[20];
	int pointb[20];
	
	int point[20];
	
	int la,lb;
	
	int go;
	
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
			
			na = n;
			pxa = malloc(sizeof(Px)*na);
			
			i=0;
			for(y=0;y<height;y++)
				for(x=0;x<width;x++)
					if(aa[x+y*width] != 0){
						pxa[i].x=x;
						pxa[i].y=y;
						printf("star %d (%d,%d)\n",i,x,y);
						i++;
					}
						
			a2 = malloc(sizeof(int)*na*na);
			for(i=0;i<na;i++)
				for(j=0;j<na;j++){
					dx=pxa[i].x-pxa[j].x;
					dy=pxa[i].y-pxa[j].y;
					distance=dx*dx+dy*dy;
					distance = (int)sqrt((double)distance);
					printf("%d:%d -> %d\n",i,j,distance);
					a2[j*na+i]=distance;	
				}
			
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
			
			nb = n;
			pxb = malloc(sizeof(Px)*nb);
			
			i=0;
			for(y=0;y<height;y++)
				for(x=0;x<width;x++)
					if(bb[x+y*width] != 0){
						pxb[i].x=x;
						pxb[i].y=y;
						printf("star %d (%d,%d)\n",i,x,y);
						i++;
					}	
					
			b2 = malloc(sizeof(int)*nb*nb);
			for(i=0;i<nb;i++)
				for(j=0;j<nb;j++){
					dx=pxb[i].x-pxb[j].x;
					dy=pxb[i].y-pxb[j].y;
					distance=dx*dx+dy*dy;
					distance = (int)sqrt((double)distance);
					printf("%d:%d -> %d\n",i,j,distance);
					b2[j*nb+i]=distance;	
				}	
				
				
			//
			upper = INT_MAX;
			go = 1;
			i = 0;
			while ( go ) {
				la = get_max(a2,na*na,&upper);
				if (la>=0) {
					printf("upper=%d a=%d\n",upper,la);
					lb = get_index(b2,nb*nb,upper);
					if (lb>=0) {
						printf("found b=%d\n",lb);
						lista[i] = la;
						listb[i++] = lb;
						if (i==10) go=0;
					}
				} else {
					go = 0;
				}
			}
			
			size = i;
			printf("size=%d\n",size);
			
			for(j=0;j<size;j++){
				la = lista[j];
				pointa[2*j  ]=la%na;
				pointa[2*j+1]=la/na;
				lb = listb[j];
				pointb[2*j  ]=lb%nb;
				pointb[2*j+1]=lb/nb;
			}
			
			for(j=0;j<1;j++){
				x0 = pointa[2*j];
				x1 = pointa[2*j+1];
				y0 = pointb[2*j];
				y1 = pointb[2*j+1];
				printf("a (%d,%d) - (%d,%d)\n",pxa[x0].x,pxa[x0].y,pxa[x1].x,pxa[x1].y);
				x = (pxa[x0].x+pxa[x1].x) - (pxb[y0].x+pxb[y1].x);
				x = x/2;
				printf("b (%d,%d) - (%d,%d)\n",pxb[y0].x,pxb[y0].y,pxb[y1].x,pxb[y1].y);
				y = (pxa[x0].y+pxa[x1].y) - (pxb[y0].y+pxb[y1].y);
				y = y/2;
				printf("dx=%d , dy=%d\n",x,y);
			}
			
			xmini = -x;
			ymini = -y;
			
			//// triangle
			//index=0;
			//for(j=0;j<(2*size+1);j++){
				//la = pointa[j];
				//n = 0;
				//for(k=j+1;k<(2*size+1);k++){
					//lb = pointa[k];
					//if (la==lb) n++;
					//if (n==2){
						//point[index++]=la;
						//break;
					//}
				//}
			//}
			
			//vb = -1;
			//vc = -1;
			//for(i=0;i<index;i++)
				//va = point[i];
				//for(j=0;j<(2*size+1);j++)
					//if (pointa[j] == va){
						//if (j%2)
							//n=j+1;
						//else 
							//n=j-1;
						//n = pointa[n];
						//for(k=0;k<index;k++)
							//if (point[k]==n){
								//vb = point[k];
								//break;
							//}
						//if (vb<0) break;
						//for(k=0;k<(2*size+1);k++)
							//if (pointa[k] == vb){
								//if (k%2)
									//n=k+1;
								//else
									//n=k-1;
								//n = pointa[n];
								//if(n==va) continue;
								//for(l=0;l<index;l++)
									//if (point[l]==n){
										//vc = point[l];
										//break;
									//}
								//if (vc<0) break;
							//}
					//}
									
						
			
			
			//i = 0;
			//upper = INT_MAX;			
			//if (na < nb){
				//while (i<3) {
					//x = get_max(a2,na*na,&upper);
					//if (x){
						//y = get_index(b2,nb*nb,upper);
						//if (y) {
							//i++;
							//x0 = x%na;
							//y0 = x/na;
							//printf("\na (%d,%d) - (%d,%d)\n",pxa[x0].x,pxa[x0].y,pxa[y0].x,pxa[y0].y);
							//x1 = y%nb;
							//y1 = y/nb;
							//printf("b (%d,%d) - (%d,%d)\n",pxb[x1].x,pxb[x1].y,pxb[y1].x,pxb[y1].y);
							////
							//x = pxa[x0].x-pxb[x1].x;
							//y = pxa[y0].x-pxb[y1].x;
							//printf("%d // %d\n",x,y);
							//if ((x-y)*(x-y)<=1) printf("||\n");
							//x = pxa[x0].x-pxb[y1].x;
							//y = pxa[y0].x-pxb[x1].x;
							//printf("%d // %d\n",x,y);
							//if ((x-y)*(x-y)<=1) printf("X\n");
						//}
					//} else {
						//break;
					//}
				//}
			//} else {
				//while (i<3) {
					//x = get_max(b2,nb*nb,&upper);
					//if (x){
						//y = get_index(a2,na*na,upper);
						//if (y) {
							//i++;
							//x0 = x%nb;
							//y0 = x/nb;
							//printf("\nb (%d,%d) - (%d,%d)\n",pxb[x0].x,pxb[x0].y,pxb[y0].x,pxb[y0].y);
							//x1 = y%na;
							//y1 = y/na;
							//printf("a (%d,%d) - (%d,%d)\n",pxa[x1].x,pxa[x1].y,pxa[y1].x,pxa[y1].y);
							////
							//x = pxb[x0].x-pxa[x1].x;
							//y = pxb[y0].x-pxa[y1].x;
							//printf("%d // %d\n",x,y);
							//if ((x-y)*(x-y)<=1) printf("||\n");
							//x = pxb[x0].x-pxa[y1].x;
							//y = pxb[y0].x-pxa[x1].x;
							//printf("%d // %d\n",x,y);
							//if ((x-y)*(x-y)<=1) printf("X\n");
						//}
					//} else {
						//break;
					//}
				//}				
			//}	
// 			
			//pivota[0].x = 0;
			//pivota[0].y = 0;
			//pivota[1].x = 0;
			//pivota[1].y = height;	
			//pivota[2].x = width;
			//pivota[2].y = 0;					
			//pivota[3].x = width;
			//pivota[3].y = height;
			//// iterate
			//for(iter=0; iter<10; iter++){
				////printf("iteration #%d\n",iter);
				//// init
				//for (i=0; i<4; i++){
					//pivota[i].n = 0;
					//pivota[i].dx = 0;
					//pivota[i].dy = 0;
				//}
				//for(x=0; x<width; x++){
					//for(y=0;y<height;y++){
						//if (aa[y*width+x] == 1){
							//distmini = width*width + height*height;
							//distmini = (int)sqrt((double)distmini);
							//for (i=0; i<4; i++){
								//dx = x - pivota[i].x;
								//dy = y - pivota[i].y;
								//distance = dx*dx + dy*dy;
								//distance = (int)sqrt((double)distance);
								//if ( distance < distmini){
									//distmini = distance;
									//n = i;
								//}
							//}
							//pivota[n].n += 1;
							//pivota[n].dx += x - pivota[n].x;
							//pivota[n].dy += y - pivota[n].y;
						//}	
					//}
				//}
				//for (i=0; i<4; i++){
					//n = pivota[i].n;
					//if (n != 0) {
						//pivota[i].x += pivota[i].dx/n;
						//pivota[i].y += pivota[i].dy/n;
						////printf("%d: #%d x=%d y=%d dx=%d dy=%d\n",i,n,pivota[i].x,pivota[i].y,pivota[i].dx/n,pivota[i].dy/n);
					//}
				//}
			//}

			//pivotb[0].x = 0;
			//pivotb[0].y = 0;
			//pivotb[1].x = 0;
			//pivotb[1].y = height;	
			//pivotb[2].x = width;
			//pivotb[2].y = 0;					
			//pivotb[3].x = width;
			//pivotb[3].y = height;
			//// iterate
			//for(iter=0; iter<10; iter++){
				////printf("iteration #%d\n",iter);
				//// init
				//for (i=0; i<4; i++){
					//pivotb[i].n = 0;
					//pivotb[i].dx = 0;
					//pivotb[i].dy = 0;
				//}
				//for(x=0; x<width; x++){
					//for(y=0;y<height;y++){
						//if (bb[y*width+x] == 1){
							//distmini = width*width + height*height;
							//distmini = (int)sqrt((double)distmini);
							//for (i=0; i<4; i++){
								//dx = x - pivotb[i].x;
								//dy = y - pivotb[i].y;
								//distance = dx*dx + dy*dy;
								//distance = (int)sqrt((double)distance);
								//if ( distance < distmini){
									//distmini = distance;
									//n = i;
								//}
							//}
							//pivotb[n].n++;
							//pivotb[n].dx += x - pivotb[n].x;
							//pivotb[n].dy += y - pivotb[n].y;
						//}	
					//}
				//}
				//for (i=0; i<4; i++){
					//n = pivotb[i].n;
					//if (n != 0) {
						//pivotb[i].x += pivotb[i].dx/n;
						//pivotb[i].y += pivotb[i].dy/n;
						////printf("%d: #%d x=%d y=%d dx=%d dy=%d\n",i,n,pivotb[i].x,pivotb[i].y,pivotb[i].dx/n,pivotb[i].dy/n);
					//}
					
				//}
			//}
			
			//maxi = 0;
			//for (i=0; i<4; i++){
				//n = pivota[i].n;
				//if (n) printf("%d: #%d x=%d y=%d dx=%d dy=%d\n",i,n,pivota[i].x,pivota[i].y,pivota[i].dx/n,pivota[i].dy/n);
				//if ( n > maxi){
					//maxi = n;
					//na = i;
				//}
			//}
			
			//maxi = 0;
			//for (i=0; i<4; i++){
				//n = pivotb[i].n;
				//if (n) printf("%d: #%d x=%d y=%d dx=%d dy=%d\n",i,n,pivotb[i].x,pivotb[i].y,pivotb[i].dx/n,pivotb[i].dy/n);
				//if ( n > maxi){
					//maxi = n;
					//nb = i;
				//}
			//}
			
			//xmini = pivotb[nb].x - pivota[na].x;
			//ymini = pivotb[nb].y - pivota[na].y;
			
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
				free(a); free(b); free(c);
				return 0;
			}
		   
	   }
		   
	}
					
	free(a); free(b); free(c);
	return -1;
	
}
