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
	unsigned long sum, sum2;
	unsigned short tmp;
	unsigned short average;
	unsigned long variance;
	double stddev;
	int offset;
	
	unsigned int histo[10];
	unsigned short delta;
	unsigned int index;
	double percent;
	
	mini = 65535;
	maxi = 0;
	sum = 0L;
	sum2 = 0L;

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
					if( tmp > maxi) maxi = tmp;
					if( tmp < mini) mini = tmp;
					sum  += (unsigned long) tmp;
					sum2 += (unsigned long) tmp * (unsigned long) tmp;
				}
			
				printf("----------------------------------------\n");			
				printf("minimun = %d\n",mini);
				printf("maximum = %d\n",maxi);
				//printf("\n");
				printf("----------------------------------------\n");
				average = (unsigned short)(sum / (unsigned long) nelements);
				variance = (sum2 / (unsigned long)nelements) - (unsigned long)average * (unsigned long)average;
				stddev = sqrt((double)variance);
				
				printf("average = %d\n",average);
				printf("standard deviation = %.2f\n",stddev);
			
				printf("----------------------------------------\n");
				//printf("%d;%d;%d;%f\n",mini,maxi,average,stddev);
				delta = maxi - mini;
				for (i=0; i<10; i++) histo[i]=0; // init
				for (i=0; i<nelements; i++){
					tmp = image[i];
					index = ((tmp - mini)*10)/delta;
					if (index == 10) index = 9;
					histo[index] += 1;
				}
				printf("histogram\n");
				for (i=0; i<10; i++) {
				    percent = ((double)histo[i]*100.0)/(double)nelements;
					printf("%02d| %0.2f%%\n",i+1,percent);
				} 
				printf("\n");
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

	
	if (argc != 2){
		printf("Usage: %s <fits file> \n",argv[0]);
		return -1;
	}
	
	strcpy(path,argv[1]);

	file_count = 0;
	process_input_fits(path);				
	
	return 0;
}
	
	
