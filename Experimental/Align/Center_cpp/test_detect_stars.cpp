

#include "image.h"


int main(int argc, char* argv[])
{

	Image* im;
	int rc;
	
	im = new Image(argv[1]);
	im->test_detect_stars(argv[2]);
	
	delete im;
}
	
	
