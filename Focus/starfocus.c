#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include "fitsio.h"

struct ushort_node {
	unsigned short 		value;
	struct ushort_node 	*next;
};

typedef struct ushort_node ushort_node;

long width=0;
long height=0;
ushort_node** root = NULL;

const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

int precision;

void process_input_fits(char* path){
	fitsfile *fptr;    
    int status = 0;
    unsigned int bitpix, naxis;
    long naxes[2];
    long nelements;
    int anynul;
    unsigned short *image;
    int i;
    unsigned short mini, maxi;
	unsigned short tmp;
	int offset;
	
	int x,y;
	int xx,yy;
	
	unsigned int histo[10];
	unsigned short delta;
	unsigned int index;
	double percent;
	int score;
	
	mini = 65535;
	maxi = 0;

	//printf("%s\n",path);
    if (!fits_open_file(&fptr, path, READONLY, &status))
    {
		if (!fits_get_img_param(fptr, 2, &bitpix, &naxis, naxes, &status) )
        {
			//printf("bitpix=%d naxes=%d width=%ld height=%ld\n",bitpix,naxis,naxes[0],naxes[1]);
			nelements = naxes[0]*naxes[1];
			if (width==0 && height==0){
				width = naxes[0];
				height = naxes[1];
				root = malloc(sizeof(ushort_node*)*nelements);
				for (i=0; i<nelements; i++){
					root[i] = NULL;
				}
				
			} else {
				if (width != naxes[0] || height != naxes[1]){
					printf("naxes error: %ldx%ld instead of %ldx%ld\n",naxes[0],naxes[1],width,height);
					return;
				}
			}
			
			image = malloc(sizeof(unsigned short)*nelements);
			
			if (bitpix == 16){
				fits_read_img(fptr,TUSHORT,1,nelements,NULL,image, &anynul, &status);
				for (i=0; i<nelements; i++){
					tmp = image[i];
					if( tmp > maxi) {
						maxi = tmp;
						index = i;
					}
					if( tmp < mini) mini = tmp;
				}
			
				printf("----------------------------------------\n");			
				printf("minimun = %d\n",mini);
				printf("maximum = %d\n",maxi);
			
				printf("----------------------------------------\n");
				//printf("%d;%d;%d;%f\n",mini,maxi,average,stddev);
				delta = maxi - mini;
				offset = maxi - delta/precision;
				x = index % width;
				y = index / width;
				printf("(%d,%d)\n",x,y);
				score = 0;
				for(yy=y-5; yy<y+5; yy++){
					for(xx=x-5; xx<x+5;xx++){
						if (xx > 0 && yy > 0 && xx < width && yy < height){
							tmp = image[yy*width+xx];
							if (tmp > offset) score++;
						}
					}
				}
				
				printf("score=%d\n",101-score);

			}
			// close and free
			free(image);
			fits_close_file(fptr, &status);
		}
	}
}



int main(int argc, char* argv[]) {
	
	char* directory;
	//char* output_file;
	DIR * dirp;
    struct dirent * entry;
    char path[256];
    int file_count;

	
	if (argc != 3){
		printf("Usage: %s <fits file> <precision>\n",argv[0]);
		return -1;
	}
	
	strcpy(path,argv[1]);
	precision = atoi(argv[2]);

	file_count = 0;
	process_input_fits(path);				
	
	return 0;
}
	
	
