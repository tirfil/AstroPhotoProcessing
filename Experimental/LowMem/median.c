#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include "fitsio.h"

#define MAXSIZE 30000000L

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

int get_fits_size(char* path){
	fitsfile *fptr;    
    int status = 0;
    unsigned int bitpix, naxis;
    long naxes[2];
    long nelements;
    int anynul;
    if (!fits_open_file(&fptr, path, READONLY, &status))
    {
		if (!fits_get_img_param(fptr, 2, &bitpix, &naxis, naxes, &status)){
			printf("bitpix=%d naxes=%d width=%ld height=%ld\n",bitpix,naxis,naxes[0],naxes[1]);
			if (width==0 && height==0){
				width = naxes[0];
				height = naxes[1];
			} else {
				if (width != naxes[0] || height != naxes[1]){
					printf("naxes error: %ldx%ld instead of %ldx%ld\n",naxes[0],naxes[1],width,height);
					fits_close_file(fptr, &status);
					return -1;
				}
			}
			if (bitpix != 16){
				fits_close_file(fptr, &status); 
				return -1;
			}
		}
		fits_close_file(fptr, &status);
		return 0;
	}
}

void process_input_fits(char* path, int start, int stop2){
	fitsfile *fptr;    
    int status = 0;
    unsigned int bitpix, naxis;
    long naxes[2];
    long nelements;
    int anynul;
    unsigned short *image;
    int i,j;
    ushort_node* item;
    ushort_node* tmp;
    ushort_node* tmp2;
    int stop;

	printf(".");
	if (!fits_open_file(&fptr, path, READONLY, &status))
    {
		if (!fits_get_img_param(fptr, 2, &bitpix, &naxis, naxes, &status) )
        {
			nelements = naxes[0] * naxes[1];
			
			image = malloc(sizeof(unsigned short)*nelements);
			
			fits_read_img(fptr,TUSHORT,1,nelements,NULL,image, &anynul, &status);
			for (i=start; i<stop2; i++){
				j = i - start;
				item = malloc(sizeof(ushort_node));
				item->value = image[i];
				item->next = NULL;
				// insert and sort
				if (root[j] == NULL){
					root[j]=item;
				} else {
					tmp = root[j];
					tmp2 = NULL;
					stop = 0;
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
								root[j] = item;
							} else {
								tmp2->next = item;
							}
							stop = 1;
						}
				
					} while (stop == 0);	
				}				
			}
			// close and free
			free(image);
			fits_close_file(fptr, &status);
		}
	}
}

void write_output_fits(char* filename, unsigned short* imageout){
	fitsfile *fptrout;
	unsigned int naxis = 2;
    long naxes[2];
    int status = 0;
	long nelements = width*height;
	naxes[0] = width;
	naxes[1] = height;
	fits_create_file(&fptrout,filename, &status);
	fits_create_img(fptrout, SHORT_IMG, naxis, naxes, &status);
	fits_write_img(fptrout, TUSHORT, 1, nelements, imageout, &status);
	fits_close_file(fptrout, &status);
	free(imageout);
}

int main(int argc, char* argv[]) {
	
	char* directory;
	char* output_file;
	DIR * dirp;
    struct dirent * entry;
    char path[256];
    int file_count;
    int rc;
    long size;
    long nelements;
    int start;
    int stop;
    unsigned short *imageout;
    int cluster;
    int i,j,k;
    ushort_node* tmp;
	ushort_node* tmp2;
	int median; 

	if (argc != 3){
		printf("Usage: %s <directory path> <output fits file>\n",argv[0]);
		return -1;
	}
	
	directory = argv[1];
	output_file = argv[2];

	file_count = 0;
	
	// evaluate data to process
	dirp = opendir(directory);
	if (dirp != NULL)
		while ((entry = readdir(dirp)) != NULL)
			if (entry->d_type == DT_REG)
				if ( strncmp("fits", get_filename_ext(entry->d_name), 4) == 0 ) {
					sprintf(path,"%s/%s",directory,entry->d_name);
					rc = get_fits_size(path);
					if ( rc != 0 ) exit(-1);
					file_count++;
				}
				
	nelements = width * height;		
	size = nelements * file_count;
	median = file_count/2;
	printf("#pixel= %ld - nelements=%ld - count=%d\n",size,nelements,file_count);
	start = 0;
	if (size > MAXSIZE){
		stop = (double)MAXSIZE/(double)size * (double)nelements;
	} else {
		stop = nelements;
	}

	imageout = malloc(sizeof(unsigned short)*nelements);
	
	while(start < nelements){
		printf("Processing start=%d stop=%d\n",start,stop);
		cluster = stop-start;
		root = malloc(sizeof(ushort_node*)*cluster);
		for(i=0;i<cluster;i++) root[i] = NULL;
		dirp = opendir(directory);
		if (dirp != NULL)
			while ((entry = readdir(dirp)) != NULL)
				if (entry->d_type == DT_REG)
					if ( strncmp("fits", get_filename_ext(entry->d_name), 4) == 0 ) {
						sprintf(path,"%s/%s",directory,entry->d_name);
						process_input_fits(path,start,stop);
					}
					
		printf("\n");
		printf("cluster= %d\n",cluster);
					
		for(i=0;i<cluster;i++){
			tmp = root[i];
			k = i + start;
			for(j=0;j<file_count;j++){
				// take median item
				if (j == median){
					imageout[k] = tmp->value;
				}
				tmp2 = tmp;
				tmp = tmp->next;
				// free ushort_node struct
				free(tmp2);
			}
		}
		
		free(root);
		start = stop;
		stop = stop + cluster;
		if (stop > nelements) stop = nelements;
	}
	
	printf("Writing ..\n");
	write_output_fits(output_file,imageout);
	
	return 0;
}
	
	
