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
	unsigned int x;
	unsigned int y;
} Pt;

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
	long naxis = 2;
    long naxes[2];
    int status = 0;
    unsigned long nelements = (unsigned long)width*(unsigned long)height;
    naxes[0] = (long)width;
	naxes[1] = (long)height;
	fits_create_file(&fptrout,path, &status);
	fits_create_img(fptrout, USHORT_IMG, naxis, naxes, &status);
	fits_write_img(fptrout, TUSHORT, (long long)1L, (long long)nelements, image, &status);
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
	unsigned long long sum, sum2;
	unsigned long long variance;
	
	sum = 0L;
	sum2 = 0L;
	
	for (i=0; i<size; i++){
		tmp = image[i];
		sum  += (unsigned long long)tmp;
		sum2 += (unsigned long long)tmp * (unsigned long long)tmp;
	}
	printf("sum2 = %lld\n",sum2);
	*average = (unsigned short)(sum / (unsigned long long) size);
	variance = (sum2 / (unsigned long long)size) - ((unsigned long long)(*average) * (unsigned long long)(*average));
	*stddev = (unsigned short)sqrt((double)variance);

	printf("variance = %lld\n",variance);
	printf("average = %d\n",*average);
	printf("standard deviation = %d\n",*stddev);
	
}

bool isvalid(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int level)
{
	if (x < level) return false;
	if (y < level) return false;
	if (x + level >= width) return false;
	if (y + level >= height) return false;
	return true;
}

unsigned int test_bound(unsigned short* image,unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int level,unsigned short threshold)
{
	int score;
	int total;
	int x0, y0;
	total = 0;
	score = 0;
	y0=y-level;
	for(x0=x-level;x0<=(x+level);x0++)
	{
		total++;
		if (image[x0+y0*width] > threshold) score++;
	}
	y0=y+level;
	for(x0=x-level;x0<=(x+level);x0++)
	{
		total++;
		if (image[x0+y0*width] > threshold) score++;
	}
	x0=x-level;
	for(y0=y-level;y0<=(y+level);y0++)
	{
		total++;
		if (image[x0+y0*width] > threshold) score++;
	}	
	x0=x+level;
	for(y0=y-level;y0<=(y+level);y0++)
	{
		total++;
		if (image[x0+y0*width] > threshold) score++;

	}	
	score = score*100;
	score = score/total;
	return score;
}
/* 
 * Detect star objects. A star is a pixel surounded by less bright pixels.
 *
*/ 
int* detect_stars(unsigned short* image, unsigned int width, unsigned int height, unsigned short threshold)
{
	int* stars;
	unsigned int x,y;
	unsigned int level;
	unsigned int score;
	bool cont;
	unsigned int pmax;
	int u,v;
	int x0,y0;
	
	pmax=0;

	
	printf("detect_stars\n");
	
	stars = malloc(sizeof(int)*(width*height));
	
	for(y=0;y<height;y++){
		for(x=0;x<width;x++){
			cont = false;
			if (image[x+y*width] > threshold){
				score = 100;
				level = 1;
				while(score == 100){
					if(isvalid(x,y,width,height,level)==true)
					{
						score = test_bound(image,x,y,width,height,level,threshold);
					} else {
						stars[x+y*width] = -1*level;
						cont = true;
						score = 0;
					}

					level++;
				}
				level--;
				if(isvalid(x,y,width,height,2*level)==true)
				{
					score = test_bound(image,x,y,width,height,2*level,threshold);
					if (score < 10){
						stars[x+y*width] = level;
							if (pmax < level){
							printf("%d (%d,%d)\n", level,x,y);
							pmax = level;
						}
					} else {
						stars[x+y*width] = 0;
					}
				} else {
					stars[x+y*width] = -1*level;
				}				
			} else {
				stars[x+y*width] = 0;
			}
		}
	}
	
	for(y=1;y<height-1;y++){
		for(x=1;x<width-1;x++){
			v = stars[x+y*width];
			if (v != 0) 
				for(x0=x-1;x0<=x+1;x0++)
					for(y0=y-1;y0<=y+1;y0++){
						if ( v < 0 ){
							stars[x0+y0*width]=0;
						} else {
							u = stars[x0+y0*width];
							if (u<v) stars[x0+y0*width]=0;
						}
					}
		}
	}
	u = 0;
	for(y=0;y<height;y++){
		for(x=0;x<width;x++){
			v = stars[x+y*width];
			if (v < 0) 	stars[x+y*width]=0;
			if (v > 2) {
				u++;
				printf("%d(%d,%d)\n",v,x,y);
			}
		}
	}	
	printf("counting stars = %d\n",u);
	return stars;
}

/*
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
*/

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
Pixel* select_stars(int* stars, unsigned int width, unsigned int height, unsigned int number)
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
		printf("pass1\n");
		// pass 1
		maxi = 0;
		for(i=0;i<nelements;i++){
			if (stars[i]<0) printf("%d:%d\n",i,stars[i]);
			tmp = (unsigned short)stars[i];
			if (( tmp > maxi ) && ( tmp <= upper ))
			{
				maxi = tmp;
			} 
		}
		printf("maxi=%d\n",maxi);
		// pass 2
		//get_rand(true,nelements,&i); // init
		//while(get_rand(false,nelements,&i)>0){
		i = 0;
		for(j=0;j<nelements;j++){
			i = (i + 1013)%nelements;  // 1013 prime
			xx =  i % width;
			yy =  i / width;
			if (xx == 0 || yy == 0 || xx == width-1 || yy == height-1 ) continue;
			tmp = (unsigned short)stars[i];
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

int get_two_points(Distance* d0, Distance* d1, unsigned int side, Pt* pta, Pt* ptb)
{
	int size;

	int i;
	unsigned int value0,value1;
	int i0,i1;
	int delta;
	
	size = ((side-1)*side)/2;
	i0 = 0;
	i1 = 0;
	value0=d0[i0].value;
	value1=d1[i1].value;
	
	while(abs(value0-value1) > 2){
		if ( value0 > value1){
			i0++;
		} else {
			i1++;
		}
		if ((i0 == size) || (i1 == size)){
			return -1;
		} else {
			value0=d0[i0].value;
			value1=d1[i1].value;	
		}		
	}
	printf("distance 12 = %d\n",value0);	
	if (value0 < 10) return -1;
	pta[0].x = d0[i0].x0;
	pta[0].y = d0[i0].y0;
	pta[1].x = d0[i0].x1;
	pta[1].y = d0[i0].y1;

	ptb[0].x = d1[i1].x0;
	ptb[0].y = d1[i1].y0;
	ptb[1].x = d1[i1].x1;
	ptb[1].y = d1[i1].y1;	
	
	return 0;
/*	
	while((i0 < size-1) && (i1 < size-1)){
		if (value0 > value1){
			while(value0 > value1){
				value0=d0[++i0].value;
			}
			delta = abs((int)value0 - (int)value1);
			if (delta < 2){
				pta[0].x = d0[i0].x0;
				pta[0].y = d0[i0].y0;
				pta[1].x = d0[i0].x1;
				pta[1].y = d0[i0].y1;

				ptb[0].x = d1[i1].x0;
				ptb[0].y = d1[i1].y0;
				ptb[1].x = d1[i1].x1;
				ptb[1].y = d1[i1].y1;	
				
				return 0;
				
				//i1++;		
			} else {
				value1=d1[++i1].value;
			}
		} else {
			while(value0 < value1){
				value1=d1[++i1].value;
			}
			delta = abs((int)value0 - (int)value1);
			if (delta < 2){
				pta[0].x = d0[i0].x0;
				pta[0].y = d0[i0].y0;
				pta[1].x = d0[i0].x1;
				pta[1].y = d0[i0].y1;

				ptb[0].x = d1[i1].x0;
				ptb[0].y = d1[i1].y0;
				ptb[1].x = d1[i1].x1;
				ptb[1].y = d1[i1].y1;	
				
				return 0;
				//i0++;
			} else {
				value0=d0[++i0].value;
			}
		}
	}
	
	return -1;
*/
}
		
/*
 * Compute translation x & y
 */
 /*
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
	

	
		
	x00 = d0[i0].x0;
	y00 = d0[i0].y0;
	x01 = d0[i0].x1;
	y01 = d0[i0].y1;

	x10 = d1[i1].x0;
	y10 = d1[i1].y0;
	x11 = d1[i1].x1;
	y11 = d1[i1].y1;	
	
	printf("%d (%d,%d)-(%d,%d) %d (%d,%d)-(%d,%d)\n",value0,x00,y00,x01,y01,value1,x10,y10,x11,y11);
	

	
}*/
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

int add_third_point(Pt* pta, Pt* ptb, Pixel* pixelsa, Pixel* pixelsb, unsigned int number)
{
	int i;
	int u0,v0,u1,v1;
	int d0,d1;
	bool change;
	Pixel tmp;
	int i0,i1;
	unsigned short value0,value1;
	//for(i=0;i<number;i++){
		//printf("%d -- %d\n",pixelsa[i].value,pixelsb[i].value);
		//printf("x %d -- %d\n",pixelsa[i].x,pixelsb[i].x);
		//printf("y %d -- %d\n",pixelsa[i].y,pixelsb[i].y);
	//}	
	// compute distance
	for(i=0;i<number;i++){
		u0 = (int)(pta[0].x - pixelsa[i].x);
		v0 = (int)(pta[0].y - pixelsa[i].y);	
		u1 = (int)(pta[1].x - pixelsa[i].x);
		v1 = (int)(pta[1].y - pixelsa[i].y);
		d0 = u0*u0+v0*v0;
		d1 = u1*u1+v1*v1;
		if ( d0 < d1){
			pixelsa[i].value = (unsigned short)(sqrt((double)d0));
		} else {
			pixelsa[i].value = (unsigned short)(sqrt((double)d1));
		}
	}
	for(i=0;i<number;i++){
		u0 = (int)(ptb[0].x - pixelsb[i].x);
		v0 = (int)(ptb[0].y - pixelsb[i].y);	
		u1 = (int)(ptb[1].x - pixelsb[i].x);
		v1 = (int)(ptb[1].y - pixelsb[i].y);
		d0 = u0*u0+v0*v0;
		d1 = u1*u1+v1*v1;
		if ( d0 < d1){
			pixelsb[i].value = (unsigned short)(sqrt((double)d0));
		} else {
			pixelsb[i].value = (unsigned short)(sqrt((double)d1));
		}
	}
	// sort
	change = true;
	while(change)
	{
		change = false;
		for(i=0;i<number-1;i++){
			if (pixelsa[i].value < pixelsa[i+1].value){
				change = true;
				tmp.value = pixelsa[i].value;
				tmp.x     = pixelsa[i].x;
				tmp.y     = pixelsa[i].y;
				pixelsa[i].value = pixelsa[i+1].value;
				pixelsa[i].x = pixelsa[i+1].x;
				pixelsa[i].y = pixelsa[i+1].y;
				pixelsa[i+1].value = tmp.value;
				pixelsa[i+1].x = tmp.x;
				pixelsa[i+1].y = tmp.y;
			}
		}
	}
	change = true;
	while(change)
	{
		change = false;
		for(i=0;i<number-1;i++){
			if (pixelsb[i].value < pixelsb[i+1].value){
				change = true;
				tmp.value = pixelsb[i].value;
				tmp.x     = pixelsb[i].x;
				tmp.y     = pixelsb[i].y;
				pixelsb[i].value = pixelsb[i+1].value;
				pixelsb[i].x = pixelsb[i+1].x;
				pixelsb[i].y = pixelsb[i+1].y;
				pixelsb[i+1].value = tmp.value;
				pixelsb[i+1].x = tmp.x;
				pixelsb[i+1].y = tmp.y;
			}
		}
	}
	
	//for(i=0;i<number;i++){
		//printf("%d -- %d\n",pixelsa[i].value,pixelsb[i].value);
		//printf("x %d -- %d\n",pixelsa[i].x,pixelsb[i].x);
		//printf("y %d -- %d\n",pixelsa[i].y,pixelsb[i].y);
	//}
	
	i0 = 0;
	i1 = 0;
	value0 = pixelsa[i0].value;
	value1 = pixelsb[i1].value;
	while(abs(value0 - value1) > 2){
		if (value0 > value1){
			i0++;
		} else {
			i1++;
		}
		if ((i0 == number) || (i1 == number)){
			return -1;
		} else {
			value0 = pixelsa[i0].value;
			value1 = pixelsb[i1].value;	
		}	
	}
	printf("distance 3 min = %d\n",value0);
	if (value0 < 10) return -1;
	pta[2].x = pixelsa[i0].x;
	pta[2].y = pixelsa[i0].y;
	ptb[2].x = pixelsb[i1].x;
	ptb[2].y = pixelsb[i1].y;
	return 0;		
/*		
	i0 = 0;
	i1 = 0;
	value0 = pixelsa[i0].value;
	value1 = pixelsb[i1].value;
	printf("%d %d\n",value0,value1);
	while ((i0 < number-1) && (i1 < number-1)){
		if (value0 > value1) {
			while((value0 - value1) >= 2){
				//printf("0");
				value0 = pixelsa[++i0].value;
			}
			if (abs(value0 - value1) < 2 ){
				printf("v= %d\n",value0);
				pta[2].x = pixelsa[i0].x;
				pta[2].y = pixelsa[i0].y;
				ptb[2].x = pixelsb[i1].x;
				ptb[2].y = pixelsb[i1].y;
				return 0;		
			}
		} else {
			while((value1 - value0) >= 2){
				//printf("1");
				value1 = pixelsb[++i1].value;
			}
			if (abs(value0 - value1) < 2 ){
				printf("v= %d\n",value0);
				pta[2].x = pixelsa[i0].x;
				pta[2].y = pixelsa[i0].y;
				ptb[2].x = pixelsb[i1].x;
				ptb[2].y = pixelsb[i1].y;
				return 0;		
			}			
		}
	}
	return -1;
*/
}

int cramer3(int* m,int* s,double* abc){
	int detM, detA,detB,detC;
	detM = m[0]*(m[4]*m[8]-m[5]*m[7])-m[1]*(m[3]*m[8]-m[5]*m[6])+m[2]*(m[3]*m[7]-m[4]*m[6]);
	printf("detM=%d\n",detM);
	if (detM == 0) return -1;
	detA = s[0]*(m[4]*m[8]-m[5]*m[7])-m[1]*(s[1]*m[8]-m[5]*s[2])+m[2]*(s[1]*m[7]-m[4]*s[2]);
	detB = m[0]*(s[1]*m[8]-m[5]*s[2])-s[0]*(m[3]*m[8]-m[5]*m[6])+m[2]*(m[3]*s[2]-s[1]*m[6]);
	detC = m[0]*(m[4]*s[2]-s[1]*m[7])-m[1]*(m[3]*s[2]-s[1]*m[6])+s[0]*(m[3]*m[7]-m[4]*m[6]);
	abc[0]=(double)detA/(double)detM;
	abc[1]=(double)detB/(double)detM;
	abc[2]=(double)detC/(double)detM;
	return 0;
}

int matrix3(Pt* pta, Pt* ptb, double* xabc, double*yabc) {
	int m3x3[9];
	int m3x1[3];
	int rc;
	
	m3x3[0]=ptb[0].x;
	m3x3[1]=ptb[0].y;
	m3x3[2] = 1;
	m3x1[0]=pta[0].x;
	
	m3x3[3]=ptb[1].x;
	m3x3[4]=ptb[1].y;
	m3x3[5] = 1;
	m3x1[1]=pta[1].x;
	
	m3x3[6]=ptb[2].x;
	m3x3[7]=ptb[2].y;
	m3x3[8] = 1;
	m3x1[2]=pta[2].x;	
	
	rc = cramer3(m3x3,m3x1,xabc);
	
	printf("x'= %f*x + %f*y + %f\n",xabc[0],xabc[1],xabc[2]);
	
	m3x3[0]=ptb[0].x;
	m3x3[1]=ptb[0].y;
	m3x3[2] = 1;
	m3x1[0]=pta[0].y;
	
	m3x3[3]=ptb[1].x;
	m3x3[4]=ptb[1].y;
	m3x3[5] = 1;
	m3x1[1]=pta[1].y;
	
	m3x3[6]=ptb[2].x;
	m3x3[7]=ptb[2].y;
	m3x3[8] = 1;
	m3x1[2]=pta[2].y;
	
	rc = cramer3(m3x3,m3x1,yabc);
	
	printf("y'= %f*x + %f*y + %f\n",yabc[0],yabc[1],yabc[2]);
	
}

void convert(unsigned short* in,unsigned short** out, int width, int height, double* xabc, double* yabc)
{
	
	int nelements;
	int i;
	int x,y;
	int u,v;
	
	nelements = width*height;
	*out = malloc(sizeof(unsigned short)*nelements);
	// init
	for(i=0;i<nelements;i++)
		(*out)[i]=0;
	// convert
	for(y=0;y<height;y++)
	for(x=0;x<width;x++){
		u = floor(0.5+xabc[0]*(double)x+xabc[1]*(double)y+xabc[2]);
		if (u<0 || u>=width) continue;
		v = floor(0.5+yabc[0]*(double)x+yabc[1]*(double)y+yabc[2]);
		if (v<0 || v>=height) continue;
		(*out)[u+v*width]=in[x+y*width];
	}
}

int main(int argc, char* argv[]) {
	
		unsigned short* a;
		unsigned short* b;
		unsigned short* m;
		int* stars;
		Pixel* pixelsa;
		Pixel* pixelsb;
		Distance* dista;
		Distance* distb;
		int x;
		int y;
		unsigned short average;
		unsigned short stddev;
		
		Pt pta[3];
		Pt ptb[3];
		
		double xabc[3],yabc[3];
		
		int rc;
		
		int npts;
		
		unsigned short* im;
	
		npts = 30;
	
		if (read_fits(argv[1],&a) == 0){
			if (read_fits(argv[2],&b) == 0){
				m = medianfilter(a,width,height);
				stat(m,width*height,&average,&stddev);
				stars = detect_stars(m,width,height,average+3*stddev);
				printf("-- stars a\n");
				pixelsa = select_stars(stars,width,height,npts);
				dista = compute_distance(pixelsa,npts);
				sort_distance(dista,npts);
				free(m);free(stars);
				m = medianfilter(b,width,height);
				stat(m,width*height,&average,&stddev);
				stars = detect_stars(m,width,height,average+3*stddev);
				printf("-- stars b\n");
				pixelsb = select_stars(stars,width,height,npts);
				distb = compute_distance(pixelsb,npts);
				sort_distance(distb,npts);
				free(m);free(stars);
				//compute_translation(dista,distb,20,&x,&y);
				rc = get_two_points(dista,distb,npts,pta,ptb);				
				if (rc < 0){
					free(dista); free(distb);
					free(pixelsa); free(pixelsb);
					free(a);free(b);
					return -1;
				}	

				rc = add_third_point(pta,ptb,pixelsa,pixelsb,npts);
				if (rc < 0){
					free(dista); free(distb);
					free(pixelsa); free(pixelsb);
					free(a);free(b);
					return -1;
				}
				printf("a: (%d,%d)-(%d,%d)-(%d,%d)\n",pta[0].x,pta[0].y,pta[1].x,pta[1].y,pta[2].x,pta[2].y);
				printf("b: (%d,%d)-(%d,%d)-(%d,%d)\n",ptb[0].x,ptb[0].y,ptb[1].x,ptb[1].y,ptb[2].x,ptb[2].y);		
				free(dista); free(distb);
				free(pixelsa); free(pixelsb);
				matrix3(pta,ptb,xabc,yabc);
				convert(b,&im,width,height,xabc,yabc);
				m = medianfilter(im,width,height);
				printf("write\n");
				write_fits(argv[3],m);
				free(im); free(m);
				free(a);free(b);
			}
		}
}
