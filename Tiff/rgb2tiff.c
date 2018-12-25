#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include "fitsio.h"
#include "tiffio.h"

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

int main(int argc, char* argv[]) {
	int i;
	unsigned short* r;
	unsigned short* g;
	unsigned short* b;
	unsigned char* tiff;

	tsize_t linewords;
	int nelements;
	
	int sampleperpixel = 3;
	
		//TIFF
	uint32 row;
	unsigned char *buf = NULL;  
	
	unsigned short maxi = 0;
	double factor;

	if (argc != 5){
		printf("Usage: %s <red.fits> <green.fits> <blue.fits> <result.tiff>\n",argv[0]);
		return -1;
	}
	
	if (read_fits(argv[1],&r) != 0) return -1;
	if (read_fits(argv[2],&g) != 0) return -1;
	if (read_fits(argv[3],&b) != 0) return -1;
	
	nelements = (int)width*(int)height;
	
	for (i=0;i <nelements; i++)
	{
		if (r[i] > maxi) maxi = r[i];
		if (g[i] > maxi) maxi = g[i];
		if (b[i] > maxi) maxi = b[i];
	}
	
	
	printf("maxi= %d\n",maxi);
	factor = (double)USHRT_MAX/(double)maxi;
	printf("factor= %f\n",factor);
	
	for (i=0;i <nelements; i++)
	{
		r[i] = (unsigned short)floor(factor*(double)r[i]+0.5);
		g[i] = (unsigned short)floor(factor*(double)g[i]+0.5);
		b[i] = (unsigned short)floor(factor*(double)b[i]+0.5);
	}
	
	
	linewords = sampleperpixel * (int)width;
	
	
	tiff = malloc(sizeof(unsigned char)*nelements*sampleperpixel*2);
	
	
	//printf("tiff = 0x%08x\n",tiff);
/*
	for (i=0; i < nelements; i++){
		tiff[6*i+1] = (unsigned char)(r[i] & 0x00ff);
		tiff[6*i+3] = (unsigned char)(g[i] & 0x00ff);
		tiff[6*i+5] = (unsigned char)(b[i] & 0x00ff);
		tiff[6*i+0] = (unsigned char)(r[i] >> 8);
		tiff[6*i+2] = (unsigned char)(g[i] >> 8);
		tiff[6*i+4] = (unsigned char)(b[i] >> 8);
	}
*/
	for (i=0; i < nelements; i++){
		tiff[6*i+0] = (unsigned char)(r[i] & 0x00ff);
		tiff[6*i+2] = (unsigned char)(g[i] & 0x00ff);
		tiff[6*i+4] = (unsigned char)(b[i] & 0x00ff);
		tiff[6*i+1] = (unsigned char)(r[i] >> 8);
		tiff[6*i+3] = (unsigned char)(g[i] >> 8);
		tiff[6*i+5] = (unsigned char)(b[i] >> 8);
	}
	
	//printf("%d\n",__LINE__);

	
	TIFF *out= TIFFOpen(argv[4], "w");
	TIFFSetField (out, TIFFTAG_IMAGEWIDTH, width);  // set the width of the image
	TIFFSetField(out, TIFFTAG_IMAGELENGTH, height);    // set the height of the image
	TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, sampleperpixel);   // set number of channels per pixel
	TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 16);    // set the size of the channels
	TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);    // set the origin of the image.
	//   Some other essential fields to set that you do not have to understand for now.
	TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
	//TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
	TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE);
	//    Allocating memory to store the pixels of current row
	printf("TIFFScanlineSize=%d\n",(int)TIFFScanlineSize(out));
	printf("linewords=%d\n",(int)linewords);
	if (TIFFScanlineSize(out)==0)
		buf =(unsigned char *)_TIFFmalloc(linewords);
	else {
		buf =(unsigned char *)_TIFFmalloc(TIFFScanlineSize(out));
		linewords = TIFFScanlineSize(out);
	}
		
	//printf("%d\n",__LINE__);

	// We set the strip size of the file to be size of one row of pixels
	//TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, 2*width*linewords));

	//Now writing image to the file one strip at a time
	for (row = 0; row < height; row++)
	{
		//memset(buf,0,linewords);
		memcpy(buf, &tiff[(height-row-1)*linewords], linewords);    // check the index here, and figure out why not using h*linebytes
		
		/*
		 * for(i=0;i<linewords;i++){
			buf[2*i]= (unsigned char)(tiff[i+(row)*linewords] & 0x00ff);
			buf[2*i+1] = (unsigned char)(tiff[i+(height-row)*linewords] >> 8);
		} */
		
		
		//printf("%d\n",__LINE__);
		
		if (TIFFWriteScanline(out, buf, row, 0) < 0)
			break;
	}
	
	//printf("%d\n",__LINE__);
		
	TIFFClose(out);
	if (buf)
		_TIFFfree(buf);
	
	free(tiff);
	free(r);
	free(g);
	free(b);
	
}
	
	
	
	
	
	
