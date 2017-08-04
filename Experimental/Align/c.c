#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
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

typedef struct {
	int x;
	int y;
	unsigned short value;
} Item;

struct ushort_node {
	unsigned short 		value;
	struct ushort_node 	*next;
};

typedef struct ushort_node ushort_node;


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

unsigned int get_rand(bool init,unsigned int max, unsigned int* value)
{
	static unsigned int* table;
	static unsigned int size;
	int i;
	unsigned int index;
	if (init == true){
		size = max;
		table = malloc(sizeof(unsigned int)*max);
		if (table != NULL)
		{
			for(i=0;i<max;i++)
				table[i] = i;
		} else {
			printf("Memory error\n");
		}
		*value = 0;
		return size;
	}
	if (max == 0){
		free(table);
		return 0;
	}
	index = rand() % size;
	*value = table[index];
	table[index] = table[size-1];
	size--;
	return size;
}

unsigned short* medianfilter(unsigned short* image,unsigned int width, unsigned int height)
{
	unsigned int nelements;
	unsigned short* im;
	int x,y;
	ushort_node* root;
	ushort_node* item;
	ushort_node* tmp;
	ushort_node* tmp2;
	int xx,yy;
	bool stop;
	int i;
	
	nelements = width * height;
	
	im = malloc(sizeof(unsigned short)*nelements);
	
	for(y=0;y< height;y++)
		for(x=0;x < width;x++){
			root = NULL;
			// 3x3 matrix
			for(yy=y-1;yy<=y+1;yy++)
				for(xx=x-1;xx<=x+1;xx++)
				{
					item = malloc(sizeof(ushort_node));
					if (xx >=0 && yy >=0 && xx < width && yy < height)
						item->value = image[xx+yy*width];
					else
						item->value = image[x+y*width];
					item->next = NULL;
					// insert and sort
					if (root == NULL){
						root=item;
					} else {
						tmp = root;
						tmp2 = NULL;
						stop = false;
						do {
							if (item->value >= tmp->value){
								if (tmp->next != NULL){
									// next
									tmp2 = tmp;
									tmp = tmp->next;
								} else {
									// append at end
									tmp->next = item;
									stop = 1;
								}
							} else {
								// insert
								item->next = tmp;
								if (tmp2 == NULL){
									// at start
									root = item;
								} else {
									tmp2->next = item;
								}
								stop = true;
							}
						} while (stop == false);	
					}			
				}
			tmp = root;
			for(i=0;i<9;i++){
				if (i == 4) im[x+y*width] = tmp->value;
				tmp2 = tmp;
				tmp = tmp->next;
				// free ushort_node struct
				free(tmp2);
			}
		}
	return im;	
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

int stat(unsigned short* image,unsigned int size, unsigned short* average, unsigned short* stddev)
{
	int i;
	unsigned short tmp;
	unsigned long sum, sum2;
	unsigned long variance;
	
	sum = 0L;
	sum2 = 0L;
	
	for (i=0; i<size; i++){
		tmp = image[i];
		sum  += (unsigned long) tmp;
		sum2 += (unsigned long) tmp * (unsigned long) tmp;
	}
	
	*average = (unsigned short)(sum / (unsigned long) size);
	variance = (sum2 / (unsigned long)size) - ((unsigned long)*average * (unsigned long)*average);
	*stddev = (unsigned short)sqrt((double)variance);

	printf("average = %d\n",*average);
	printf("standard deviation = %d\n",*stddev);
	
}

/* 
 * Detect star objects. A star is a pixel surounded by less bright pixels.
 *
*/ 
unsigned short* detect_stars(unsigned short* image, unsigned int width, unsigned int height, unsigned short threshold)
{
	int x,y;
	int pos;
	unsigned short value;
	int counter;
	int x0,y0;
	int precision;
	
	precision = 10;
	
	unsigned short* stars;
	
	printf("detect_stars\n");
	
	stars = malloc(sizeof(unsigned short)*(width*height));
	
	//pass1
	for (y=1;y<height-1;y++){
		counter = 0;
		for(x=1;x<width-1;x++){
			pos = y*width+x;
			value = image[pos];
			stars[pos]=0;
			if (value < threshold){
				if (counter >= precision)
				{
					//printf("len: %d\n",counter);
					counter = counter/2;
					stars[y0*width+x0+counter/2] = counter;
				}
				counter = 0;
			} else {
				if (counter == 0)
				{
					x0 = x;
					y0 = y;
				}
				counter += 1;
			}
		}
	}
	// pass2 (filtering)
	for (y=1;y<height-1;y++)
		for(x=1;x<width-1;x++){
			pos = y*width+x;
			value = stars[pos];
			if (value == 0) continue;
			for(y0=y-1;y0<=y+1;y0++){
				for(x0=x-1;x0<=x+1;x0++){
					if (stars[y0*width+x0] > value)
						if ((x0 != x) || (y0 != y)){
							stars[pos]=0;
						}
				}
			}
		}
	
	return stars;
}

/*
unsigned short* detect_stars(unsigned short* image, unsigned int width, unsigned int height, unsigned short threshold)
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
			if (value < threshold){
				stars[pos]=0;
				continue;
			}
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
				//printf("Add (%d,%d) - %d\n",x,y,value);
				stars[pos] = value;
			} else {
				stars[pos] = 0;
			}
		}
	return stars;
}
*/

/* 
 * Select brighter stars
*/
Pixel* select_stars(unsigned short* stars, unsigned int width, unsigned int height, unsigned int number)
{
	unsigned short maxi;
	unsigned short upper;
	unsigned short tmp;
	int i,j,n;
	unsigned int x,y;
	unsigned int nelements;
	unsigned int xx,yy;

	Pixel* pix;
	bool cont;
	
	printf("select_stars\n");
	nelements = width*height;
	pix = malloc(sizeof(Pixel)*number);
	upper = USHRT_MAX;	
	cont = true;
	n = 0;
	while(cont){
		// pass 1
		maxi = 0;
		for(i=0;i<nelements;i++){
			tmp = stars[i];
			if (( tmp > maxi ) && ( tmp <= upper ))
			{
				maxi = tmp;
			} 
		}
		// pass 2
		//get_rand(true,nelements,&i); // init
		//while(get_rand(false,nelements,&i)>0){
		i = 0;
		for(j=0;j<nelements;j++){
			i = (i + 1013)%nelements;  // 1013 prime
			xx =  i % width;
			yy =  i / width;
			if (xx == 0 || yy == 0 || xx == width-1 || yy == height-1 ) continue;
			tmp = stars[i];
			if (tmp == maxi){
				pix[n].x = xx;
				pix[n].y = yy;
				pix[n].value = tmp;
				printf("%d: (%d,%d) - %d\n",n,pix[n].x,pix[n].y,pix[n].value);
				n++;
				if (n == number){
					cont = false;
					break;
				}
			}
		}
		//get_rand(false,0,&i); // free
		if (cont == false) break;
		if (upper > 0) upper = maxi - 1;
	}
	//for(i=0;i<number;i++)
		//printf("(%d,%d)\n",pix[i].x,pix[i].y);
	return pix;
}

/*
 * Compute distance between selected stars
*/
Distance* compute_distance(Pixel* pix,unsigned int number)
{
	int i,j;
	int dx,dy;
	int d;
	int size;
	Distance* distance;
	int n;
	
	printf("compute_distance\n");
	
	size = ((number-1)*number)/2;

	distance = malloc(sizeof(Distance)*size);
	
	//for(i=0;i<number;i++)
		//printf("(%d,%d)\n",pix[i].x,pix[i].y);
	
	n = 0;
	for(j=0;j<number;j++)
		for(i=j;i<number;i++){
			if (i != j){
				dx = (int)pix[i].x - (int)pix[j].x;
				dy = (int)pix[i].y - (int)pix[j].y;
				d=dx*dx+dy*dy;
				d = (int)sqrt((double)d);
				distance[n].value = (unsigned int)d;
				//printf("distance= %d (%d,%d)-(%d,%d) ;",d,pix[i].x,pix[i].y,pix[j].x,pix[j].y);
				distance[n].x0 = pix[i].x;
				distance[n].x1 = pix[j].x;
				distance[n].y0 = pix[i].y;
				distance[n].y1 = pix[j].y;
				n++;
			}
		}
	/*
	for(i=0;i<size-1;i++){
		printf("%d ",distance[i].value);
	}
	printf("\n");
	*/	
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
	size = ((side-1)*side)/2;
	
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
				distance[i].value = distance[i+1].value;
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
	for(i=0;i<size-1;i++){
		printf("%d ",distance[i].value);
	}
	printf("\n");
}
/*
 * Compute translation x & y
 */
void compute_translation(Distance* d0, Distance* d1, unsigned int side, int* x, int* y)
{
	int size;

	int i;
	unsigned int value0,value1;
	int i0,i1;
	int delta;
	unsigned int x00,y00,x01,y01,x10,y10,x11,y11;
	
	size = ((side-1)*side)/2;
	i0 = 0;
	i1 = 0;
	value0=d0[0].value;
	value1=d1[0].value;
	
	while((i0 < size-1) && (i1 < size-1)){
		if (value0 > value1){
			while(value0 > value1){
				value0=d0[++i0].value;
			}
			delta = abs((int)value0 - (int)value1);
			if (delta < 2){
				x00 = d0[i0].x0;
				y00 = d0[i0].y0;
				x01 = d0[i0].x1;
				y01 = d0[i0].y1;

				x10 = d1[i1].x0;
				y10 = d1[i1].y0;
				x11 = d1[i1].x1;
				y11 = d1[i1].y1;	
				printf("%d (%d,%d)-(%d,%d) %d (%d,%d)-(%d,%d)\n",value0,x00,y00,x01,y01,value1,x10,y10,x11,y11);
				printf("pt1(%d,%d) pt2(%d,%d)\n",x00-x10,y00-y10,x01-x11,y01-y11);	
				
				i1++;		
			} else {
				value1=d1[++i1].value;
			}
		} else {
			while(value0 < value1){
				value1=d1[++i1].value;
			}
			delta = abs((int)value0 - (int)value1);
			if (delta < 2){
				x00 = d0[i0].x0;
				y00 = d0[i0].y0;
				x01 = d0[i0].x1;
				y01 = d0[i0].y1;

				x10 = d1[i1].x0;
				y10 = d1[i1].y0;
				x11 = d1[i1].x1;
				y11 = d1[i1].y1;	
				printf("%d (%d,%d)-(%d,%d) %d (%d,%d)-(%d,%d)\n",value0,x00,y00,x01,y01,value1,x10,y10,x11,y11);			
				printf("pt1(%d,%d) pt2(%d,%d)\n",x00-x10,y00-y10,x01-x11,y01-y11);
				
				i0++;
			} else {
				value0=d0[++i0].value;
			}
		}
	}
	
	/*
	
		
	x00 = d0[i0].x0;
	y00 = d0[i0].y0;
	x01 = d0[i0].x1;
	y01 = d0[i0].y1;

	x10 = d1[i1].x0;
	y10 = d1[i1].y0;
	x11 = d1[i1].x1;
	y11 = d1[i1].y1;	
	
	printf("%d (%d,%d)-(%d,%d) %d (%d,%d)-(%d,%d)\n",value0,x00,y00,x01,y01,value1,x10,y10,x11,y11);
	
	*/
	
}
/*
void compute_translation(Distance* d0, Distance* d1, unsigned int side, int* x, int* y)
{
	int size;
	int i,j;
	int delta;
	int x0,x1,y0,y1;
	int i0,i1;
	unsigned int value0,value1;
	
	printf("compute_translate\n");
	
	size = ((side-1)*side)/2;
	
	i0 = 0;
	i1 = 0;
	value0=d0[0].value;
	value1=d1[0].value;
	
	if (value0 > value1){
		while(value0 > value1){
			value0=d0[++i0].value;
		}
	} else {
		while(value0 < value1){
			value1=d1[++i1].value;
		}
	}
	
	printf("%d - %d\n",value0,value1);
	
	for(i=i0;i<i0+8;i++){
		//printf("d0: %d\n",d0[i].value);
		for(j=i1;j<size;j++){
			//printf("d1: %d\n",d1[j].value);
			delta = abs((int)d0[i].value - (int)d1[j].value);
			if (d0[i].value < 20 || d1[j].value < 20)
				continue;
			if (delta < 1){
				// middle segment
				x0 = (d0[i].x0 + d0[i].x1)/2;
				y0 = (d0[i].y0 + d0[i].y1)/2;
				x1 = (d1[j].x0 + d1[j].x1)/2;
				y1 = (d1[j].y0 + d1[j].y1)/2;
				printf("translation: %d (%d,%d)\n",d0[i].value,x1-x0,y1-y0);
				*x = x1-x0;
				*y = y1-y0;
			}
		}
	}	
}
*/
int main(int argc, char* argv[]) {
	
		unsigned short* a;
		unsigned short* b;
		unsigned short* m;
		unsigned short* stars;
		Pixel* pixels;
		Distance* dista;
		Distance* distb;
		int x;
		int y;
		unsigned short average;
		unsigned short stddev;
	
		if (read_fits(argv[1],&a) == 0){
			if (read_fits(argv[2],&b) == 0){
				m = medianfilter(a,width,height);
				stat(a,width*height,&average,&stddev);
				stars = detect_stars(m,width,height,average+stddev);
				pixels = select_stars(stars,width,height,20);
				dista = compute_distance(pixels,20);
				sort_distance(dista,20);
				free(m);free(stars); free(pixels);
				m = medianfilter(b,width,height);
				stat(b,width*height,&average,&stddev);
				stars = detect_stars(m,width,height,average+stddev);
				pixels = select_stars(stars,width,height,20);
				distb = compute_distance(pixels,20);
				sort_distance(distb,20);
				free(m);free(stars); free(pixels);
				compute_translation(dista,distb,20,&x,&y);
				free(dista); free(distb);
				free(a);free(b);
			}
		}
}
