
1) Install dcraw executable
(apt install dcraw)

2) Create pgm file (portable gray map)

dcraw -d -4 <file>.cr2 

pgm file contains the bayer matrix.

3) Create fits file

./pgm2fits <file>.pgm <file>.fits

=====================

bayer2abcd: color map 

colorc = red
colora = colord = green
colorb = blue

=====================
 
