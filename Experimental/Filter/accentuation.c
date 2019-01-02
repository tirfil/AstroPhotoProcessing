#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include "fitsio.h"


#define RADIUS 5
#define MULT 1

struct ushort_node {
	unsigned short 		value;
	struct ushort_node 	*next;
};

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

int testxy(int x,int y)
{
	if (x < 0) return -1;
	if (y < 0) return -1;
	if (x >= width) return -1;
	if (y >= height) return -1;
	return 0;
}


int main(int argc, char* argv[]) {
	unsigned short* in;
	unsigned short* lowpass;
	unsigned short* out;
	long nelements;
	int i,n;
	int sum;
	int x,y;
	int x0,y0;

	
	unsigned short result;
	
	ushort_node* root;
	ushort_node* item;
	ushort_node* tmp;
	ushort_node* tmp2;
	int stop;	
	
	int debug = 0;

	if (argc != 3){
		printf("Usage: %s <in> <out> \n",argv[0]);
		return -1;
	}
	
	if (read_fits(argv[1],&in) == 0){
		nelements = width*height;
		out = malloc(sizeof(unsigned short)*nelements);
		
		lowpass = malloc(sizeof(unsigned short)*nelements);
		
		for(y=0;y< height;y++)
			for(x=0;x < width;x++){
				n=0;
				sum = 0;
				for (y0=y-RADIUS; y0 <= y+RADIUS; y0++)
					for (x0=x-RADIUS; x0 <= x+RADIUS; x0++)
					{
						if (testxy(x0,y0) < 0) continue;
						n++;
						sum += in[x0+y0*width];
					}
				if (n>0) lowpass[x+y*width] = sum/n;
			}
			
			
		for(i=0; i < nelements; i++)
		{
			n = 2*in[i] - lowpass[i]*MULT;
			if (n>0) 
				out[i]=n;
			else 
				out[i]=0;
		}
		
		
				
				
				//root = NULL;
				////if (x==100 && y==100) debug=1; else debug=0;
				//for(yy=y-1;yy<=y+1;yy++)
					//for(xx=x-1;xx<=x+1;xx++)
					//{
						////if (debug) printf("-> %d\n",in[xx+yy*width]);
						//item = malloc(sizeof(ushort_node));
						//item->value = in[xx+yy*width];
						//item->next = NULL;
						//// insert and sort
						//if (root == NULL){
							//root=item;
						//} else {
							//tmp = root;
							//tmp2 = NULL;
							//stop = 0;
							//do {
								//if (item->value >= tmp->value){
									//if (tmp->next != NULL){
										//// next
										//tmp2 = tmp;
										//tmp = tmp->next;
									//} else {
										//// append at end
										//tmp->next = item;
										//stop = 1;
									//}
								//} else {
									//// insert
									//item->next = tmp;
									//if (tmp2 == NULL){
										//// at start
										//root = item;
									//} else {
										//tmp2->next = item;
									//}
									//stop = 1;
								//}
						
							//} while (stop == 0);	
						//}			
					//}
				//tmp = root;
				//for(i=0;i<9;i++){
				////if (debug) printf("%d -> %d\n",i,tmp->value);
				//// take median item
				//if (i == 4) out[x+y*width] = tmp->value;
				//tmp2 = tmp;
				//tmp = tmp->next;
				//// free ushort_node struct
				//free(tmp2);
				//}

			//}
			}
		remove(argv[2]);
		if (write_fits(argv[2],out) != 0){
			printf("write_fits error\n");
		}
		free(lowpass);
		free(out);

	}

				
				
		
		
