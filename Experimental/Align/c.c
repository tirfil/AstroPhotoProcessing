#include <dirent.h>
#include "fitsio.h"
#include <limits.h>
#include <math.h>

typedef enum { false, true } bool;
typedef struct {
	unsigned int x;
	unsigned int y;
	unsigned short value;
} Pixel;

typedef struct {
	unsigned int x0;
	unsigned int y0;
	unsigned int x1;
	unsigned int y1;
	unsigned int value;
} Distance;

int width=0;
int height=0;

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
					printf("naxes error: %ldx%ld instead of %dx%d\n",naxes[0],naxes[1],width,height);
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

int minmax(unsigned short* image,unsigned int size, unsigned short* mini, unsigned short* maxi)
{
	int i;
	unsigned short tmp;
	*mini = USHRT_MAX;
	*maxi = 0;
	for (i=0;i<size;i++){
		tmp = image[i];
		if ( tmp < *mini ) *mini = tmp;
		if ( tmp > *maxi ) *maxi = tmp;
	}
} 

/* 
 * Detect star objects. A star is a pixel surounded by less bright pixels.
 *
*/ 
unsigned short* detect_stars(unsigned short* image, unsigned int width, unsigned int height)
{
	int x,y;
	int pos;
	unsigned short value;
	int x0,y0;
	bool top;
	
	unsigned short* stars;
	
	printf("detect_stars\n");
	
	stars = malloc(sizeof(unsigned short)*(width*height));
	
	for (y=1;y<height-1;y++)
		for(x=1;x<width-1;x++){
			pos = y*width+x;
			value = image[pos];
			//printf("%d\n",value);
			top = true;
			for(y0=y-1;y0<=y+1;y0++){
				for(x0=x-1;x0<=x+1;x0++){
					if (image[y0*width+x0] >= value)
					{
						if ((x0 != x) || (y0 != y))
						{
							top = false;
						}
						
					}
					if (top == false) break;
				}
				if (top == false) break;
			}
			if (top == true)
			{
				printf("Add (%d,%d) - %d\n",x,y,value);
				stars[pos] = value;
			} else {
				stars[pos] = 0;
			}
		}
	return stars;
}

/* 
 * Select more bright stars
*/
Pixel* select_stars(unsigned short* stars, unsigned int width, unsigned int height, unsigned int number)
{
	unsigned short maxi;
	unsigned short upper;
	unsigned short tmp;
	int i,j,n;
	unsigned int x,y;
	unsigned int nelements;

	Pixel* pix;
	bool cont;
	
	printf("select_stars\n");
	maxi = 0;
	nelements = width*height;
	upper = USHRT_MAX;	
	cont = true;
	n = 0;
	while(cont){
		// pass 1
		for(i=0;i<nelements;i++){
			tmp = stars[i];
			if (( tmp > maxi ) && ( maxi <= upper ))
			{
				maxi = tmp;
			} 
		}
		// pass 2
		pix = malloc(sizeof(Pixel)*number);
		for(i=0;i<nelements;i++){
			tmp = stars[i];
			if (tmp == maxi){
				pix[n].x = i % width;
				pix[n].y = i / width;
				pix[n].value = tmp;
				//printf("(%d,%d) - %d\n",pix[n].x,pix[n].y,pix[n].value);
				n++;
				if (n == number){
					cont = false;
				}
			}
		}
		if (cont == false) break;
		if (upper > 0) upper = maxi - 1;
	}
	
	return pix;
}

/*
 * Compute distance between selected stars
*/
Distance* compute_distance(Pixel* pix,unsigned int number)
{
	int i,j;
	int dx,dy;
	unsigned int d;
	int size;
	Distance* distance;
	int n;
	
	printf("compute_distance\n");
	
	size = (number-1)*number/2;

	distance = malloc(sizeof(Distance)*size);
	
	n = 0;
	for(j=0;j<number;j++)
		for(i=j;i<number;i++){
			if (i != j){
				dx = pix[i].x - pix[j].x;
				dy = pix[i].y - pix[j].y;
				d=dx*dx+dy*dy;
				d = (int)sqrt((double)d);
				distance[n].value = d;
				//printf("distance= %d (%d,%d)-(%d,%d)\n",d,pix[i].x,pix[i].y,pix[j].x,pix[j].y);
				distance[n].x0 = pix[i].x;
				distance[n].x1 = pix[j].x;
				distance[n].y0 = pix[i].y;
				distance[n].y1 = pix[j].y;
				n++;
			}
		}
	return distance;
}
/*
 * sort distance beween star. Greater distance first.
 * 
*/
void sort_distance(Distance* distance, unsigned int side)
{
	Distance tmp;
	int size;
	int i,j;
	bool change;
	
	printf("sort_distance\n");
	size = (side-1)*side/2;
	
	change = true;
	while(change)
	{
		change = false;
		for(i=0;i<size-1;i++)
		{
			if (distance[i].value < distance[i+1].value)
			{
				// swap
				tmp.value = distance[i].value;
				tmp.x0 = distance[i].x0;
				tmp.y0 = distance[i].y0;
				tmp.x1 = distance[i].x1;
				tmp.y1 = distance[i].y1;
				distance[i].value = distance[i].value;
				distance[i].x0 = distance[i+1].x0;
				distance[i].y0 = distance[i+1].y0;
				distance[i].x1 = distance[i+1].x1;
				distance[i].y1 = distance[i+1].y1;
				distance[i+1].value = tmp.value;
				distance[i+1].x0 = tmp.x0;
				distance[i+1].y0 = tmp.y0;
				distance[i+1].x1 = tmp.x1;
				distance[i+1].y1 = tmp.y1;
				change = true;
			}
		}
	}
	
}
/*
 * Compute translation x & y
 */
void compute_translation(Distance* d0, Distance* d1, unsigned int side, int* x, int* y)
{
	int size;
	int i,j;
	int delta;
	int x0,x1,y0,y1;
	
	printf("compute_translate\n");
	
	size = (side-1)*side/2;
	for(i=0;i<size;i++)
		for(j=0;j<size;j++){
			delta = abs(d0[i].value - d1[j].value);
			if (delta < 2){
				// middle segment
				x0 = (d0[i].x0 + d0[i].x1)/2;
				y0 = (d0[i].y0 + d0[i].y1)/2;
				x1 = (d1[j].x0 + d1[j].x1)/2;
				y1 = (d1[j].y0 + d1[j].y1)/2;
				//printf("translate: %d (%d,%d)\n",d0[i].value,x1-x0,y1-y0);
				*x = x1-x0;
				*y = y1-y0;
			}
		}	
}

int main(int argc, char* argv[]) {
	
		unsigned short* a;
		unsigned short* b;
		unsigned short* stars;
		Pixel* pixels;
		Distance* dista;
		Distance* distb;
		int x;
		int y;
	
		if (read_fits(argv[1],&a) == 0){
			if (read_fits(argv[2],&b) == 0){
				stars = detect_stars(a,width,height);
				pixels = select_stars(stars,width,height,20);
				dista = compute_distance(pixels,20);
				sort_distance(dista,20);
				free(stars); free(pixels);
				stars = detect_stars(b,width,height);
				pixels = select_stars(stars,width,height,20);
				distb = compute_distance(pixels,20);
				sort_distance(distb,20);
				free(stars); free(pixels);
				compute_translation(dista,distb,20,&x,&y);
				free(dista); free(distb);
			}
		}
}
