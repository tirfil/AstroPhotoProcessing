

#include "image.h"

int usage(char* name){
	printf("Usage: %s reference|compare|translate ... :\n",name);
	printf("Usage: %s reference in.fits reference.dat\n",name);
	printf("Usage: %s compare in.fits reference.dat coefficient.dat\n",name);
	printf("Usage: %s translate in.fits coefficient.dat out.fits\n",name);
	exit(0);
}

int main(int argc, char* argv[])
{
	Triangle* ref3;
	double coefficients[6];
	Image* im;
	int rc;
	
	if (argc < 2){
		usage(argv[0]);
	}
	
	if (strcmp(argv[1],"reference")==0){
		if (argc != 4) usage(argv[0]);
		im = new Image(argv[2]);
		ref3 = im->get_reference_triangles();
		im->save_triangles(argv[3]);
	} else {
		if (strcmp(argv[1],"compare")==0){
			if (argc != 5) usage(argv[0]);
			im = new Image(argv[2]);
			ref3 = im->load_triangles(argv[3]);
			rc = im->get_coefficients(coefficients,ref3);
			if (rc ==0) im->save_coeff(argv[4]);
		} else {
			if (strcmp(argv[1],"translate")==0){
				if (argc != 5) usage(argv[0]);
				im = new Image(argv[2]);
				//printf("line %d\n",__LINE__);
				im->load_coeff(argv[3]);
				//printf("line %d\n",__LINE__);
				im->write_transform(argv[4]);
				//printf("line %d\n",__LINE__);
			} else {
				usage(argv[0]);
			}
		}
	}
	delete im;
}
	
	
