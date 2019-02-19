#include <string.h>
#include <stdio.h>
#include "fitsio.h"

// 
//
//
// pa pb
// pc pd
// 
// p11 p01
// p10 p00
//

int main(int argc, char *argv[])
{
    fitsfile *fptr;    
    fitsfile *fptrout;
    int status = 0;
    unsigned int bitpix, naxis;
    long naxes[2];
    long nelements;
    unsigned short *image;
    unsigned short *colora;
    unsigned short *colorb;
    unsigned short *colorc;
    unsigned short *colord;
    int anynul;
    
    unsigned short p00,p10,p11,p01;
    int pa,pb,pc,pd;
    int mode;
    
    int x,y;
    int index;
    
	if(argc != 2){
			printf("Usage:\n%s <raw16.fits>\n",argv[0]);
			return 0;
	}
	
	if (!fits_open_file(&fptr, argv[1], READONLY, &status))
    {
		if (!fits_get_img_param(fptr, 2, &bitpix, &naxis, naxes, &status) )
        {
			//printf("bitpix=%d naxes=%d width=%ld heigth=%ld\n",bitpix,naxis,naxes[0],naxes[1]);
			nelements = naxes[0]*naxes[1];
			image = malloc(sizeof(short)*nelements);
			
			if (bitpix == 16){
				fits_read_img(fptr,TUSHORT,1,nelements,NULL,image, &anynul, &status);

				nelements = (naxes[0]-1)*(naxes[1]-1);
				colora = malloc(sizeof(short)*nelements);
				colorb = malloc(sizeof(short)*nelements);
				colorc = malloc(sizeof(short)*nelements);
				colord = malloc(sizeof(short)*nelements);
				index = 0;
				
				for(y=1;y<naxes[1];y++)
					for(x=1;x<naxes[0];x++) {
						p00 = image[x+y*naxes[0]];
						p10 = image[x-1+y*naxes[0]];
						p01 = image[x+(y-1)*naxes[0]];
						p11 = image[x-1+(y-1)*naxes[0]];
						mode = x%2 + 2*(y%2);
						switch(mode){
							case 3: 
								pa = p11;
								pb = p01;
								pc = p10;
								pd = p00;
								break;
							case 2:
								pb = p11;
								pa = p01;
								pd = p10;
								pc = p00;
								break;
							case 1:
								pc = p11;
								pd = p01;
								pa = p10;
								pb = p00;
								break;
							case 0:
								pd = p11;
								pc = p01;
								pb = p10;
								pa = p00;
								break;
							default:
								printf("ERROR: unknown mode %d\n",mode);
								pa = 0;
								pb = 0;
								pc = 0;
								pd = 0;
						} 
						
						colora[index] = pa;
						colorb[index] = pb;
						colorc[index] = pc;
						colord[index] = pd;
						index++;
						
					}
				
				naxes[0]--; // one row and columm less
				naxes[1]--;
				
				fits_create_file(&fptrout,"colora.fits", &status);
				fits_create_img(fptrout, USHORT_IMG, naxis, naxes, &status);
				fits_write_img(fptrout, TUSHORT, 1, nelements, colora, &status);
				fits_close_file(fptrout, &status);
				fits_create_file(&fptrout,"colorb.fits", &status);
				fits_create_img(fptrout, USHORT_IMG, naxis, naxes, &status);
				fits_write_img(fptrout, TUSHORT, 1, nelements, colorb, &status);
				fits_close_file(fptrout, &status);
				fits_create_file(&fptrout,"colorc.fits", &status);
				fits_create_img(fptrout, USHORT_IMG, naxis, naxes, &status);
				fits_write_img(fptrout, TUSHORT, 1, nelements, colorc, &status);
				fits_close_file(fptrout, &status);
				fits_create_file(&fptrout,"colord.fits", &status);
				fits_create_img(fptrout, USHORT_IMG, naxis, naxes, &status);
				fits_write_img(fptrout, TUSHORT, 1, nelements, colord, &status);
				fits_close_file(fptrout, &status);
			}
			free(image);
			free(colora); free(colorb); free(colorc); free(colord);
		}
	}
	fits_close_file(fptr, &status);
	if (status) fits_report_error(stderr, status);
    return(status);
}
