#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include "fitsio.h"
#include <time.h>

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
    ushort_node* item;
    ushort_node* tmp;
    ushort_node* tmp2;
    int stop;

    if (!fits_open_file(&fptr, path, READONLY, &status))
    {
		if (!fits_get_img_param(fptr, 2, &bitpix, &naxis, naxes, &status) )
        {
			printf("bitpix=%d naxes=%d width=%ld height=%ld\n",bitpix,naxis,naxes[0],naxes[1]);
			nelements = naxes[0]*naxes[1];
			if (width==0 && height==0){
				width = naxes[0];
				height = naxes[1];
				root = malloc(sizeof(ushort_node*)*nelements);
				if (root == NULL){
						printf("malloc failed - root\n");
						return;
				}	
				for (i=0; i<nelements; i++){
					root[i] = NULL;
				}
				
			} else {
				if (width != naxes[0] || height != naxes[1]){
					printf("naxes error: %ldx%ld instead of %ldx%ld\n",naxes[0],naxes[1],width,height);
					return;
				}
			}
			
			image = malloc(sizeof(short)*nelements);
			if (image == NULL){
				printf("malloc failed - image\n");
				return;
			}
			
			if (bitpix == 16){
				fits_read_img(fptr,TSHORT,1,nelements,NULL,image, &anynul, &status);
				for (i=0; i<nelements; i++){
					item = malloc(sizeof(ushort_node));
					if (item == NULL){
						printf("malloc failed - item\n");
						return;
					}	
					item->value = image[i];
					item->next = NULL;
					// insert and sort
					if (root[i] == NULL){
						root[i]=item;
					} else {
						tmp = root[i];
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
									root[i] = item;
								} else {
									tmp2->next = item;
								}
								stop = 1;
							}
					
						} while (stop == 0);	
					}				
				}
			}
			// close and free
			free(image);
			fits_close_file(fptr, &status);
		}
	}
}

void write_output_fits(char* filename, int file_count){
	unsigned short *imageout;
	int i,j;
	ushort_node* tmp;
	ushort_node* tmp2;
	fitsfile *fptrout;
	unsigned int naxis = 2;
    long naxes[2];
    int status = 0;
	long nelements = width*height;
	int median = file_count/2;
	naxes[0] = width;
	naxes[1] = height;
	imageout = malloc(sizeof(unsigned short)*nelements);
	if (imageout == NULL){
		printf("malloc failed - imageout\n");
		return;
	}	
	for(i=0;i<nelements;i++){
		tmp = root[i];
		for(j=0;j<file_count;j++){
			// take median item
			if (j == median) imageout[i] = tmp->value;
			tmp2 = tmp;
			tmp = tmp->next;
			// free ushort_node struct
			free(tmp2);
		}
	}
	// free main array
	free(root);
	fits_create_file(&fptrout,filename, &status);
	fits_create_img(fptrout, SHORT_IMG, naxis, naxes, &status);
	fits_write_img(fptrout, TSHORT, 1, nelements, imageout, &status);
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
    char ps_cmd[256];
    time_t start;

	start = time(NULL);
	if (argc != 3){
		printf("Usage: %s <directory path> <output fits file>\n",argv[0]);
		return -1;
	}
	
	sprintf(ps_cmd,"ps -un %d",getpid());
	system(ps_cmd);
	
	directory = argv[1];
	output_file = argv[2];

	file_count = 0;
	
	// count and parse entries
	dirp = opendir(directory);
	if (dirp != NULL)
		while ((entry = readdir(dirp)) != NULL)
			if (entry->d_type == DT_REG)
				if ( strncmp("fits", get_filename_ext(entry->d_name), 4) == 0 ) {
					sprintf(path,"%s/%s",directory,entry->d_name);
					process_input_fits(path);
					file_count++;
				}
				
	system(ps_cmd);
				
	printf("Processing %d fits file(s)\n",file_count);
				
	write_output_fits(output_file,file_count);
	
	system(ps_cmd);
	
	printf("Duration is %f sec\n",difftime(time(NULL),start));
	
	return 0;
}
	
	
