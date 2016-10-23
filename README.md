# AstroPhotoProcessing

### methodology

1) From offset files, you create an offset master :   

median(offset files) => offset master

2) From dark files , you create a dark master :       

substract(median(dark files),offset master) = > dark master

3) From flat files , you create a flat master :       

substract(median(flat files),offset master) = > flat master

4) For each raw file, create a processed image :      

divide(substract(substract(raw file,offset master), dark master), flat master) => processed image

5) Align processed images (*) and stack them :

median(aligned processed image) => final image

(*) I would like a good algorithm to align deep sky images

## Stacking

**median.c**

Stack fits files located into a directory and create a new fits file from median sample values.

This method enables to improve S/N.

syntax: median < fits files directorry > < output fits file >

## Offset and Dark

**substract.c**

Substract an offset or a dark from a raw image.

syntax: substract < raw image > < offset or dark image >

output = diff.fits

## Flat

**divide.c**

Equalize an image using a flat image.

syntax: divide < image > < flat image >

output = div.fits


