#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include "fitsio.h"
#include <limits.h>
#include <math.h>

#define D2MAX 2500
#define MIN_PIX 5+2
#define MAX_PIX 15+2
#define DLEN 5
#define LEVEL 98
#define PRECISION 0.01

typedef struct Pixel {
	int x;
	int y;
	int z;
	struct Pixel* ptr;
} Pixel;

typedef struct Triangle {
	int da;
	int db;
	int dc;
	Pixel* pa;
	Pixel* pb;
	Pixel* pc;
	struct Triangle* ptr;
} Triangle;

typedef struct Pair {
	int x0;
	int y0;
	int x1;
	int y1;
} Pair;

class Image 
{
public:
	Image(char* path);
	~Image();
	Triangle* get_reference_triangles();
	int get_coefficients(double* coeff,Triangle* reference);
	int set_coefficients(double* coeff);
	int write_transform(char* path);
	int save_triangles(char* path);
	Triangle* load_triangles(char* path);
	int save_coeff(char* path);
	int load_coeff(char* path);
	
	int test_detect_stars(char* path);
	
private:

	long m_width, m_height;
	int m_mini, m_maxi;
	unsigned short* m_image;
	unsigned short* m_image2;
	Pixel* m_pixels;
	Triangle* m_triangles;
	Triangle* m_reference;
	Triangle* m_selected;
	double m_xcoeff[3];
	double m_ycoeff[3];
	int m_level;

	int read_fits(char* path);
	int write_fits(char* path);
	int compute_stat();
	int get_level(int percent);
	int detect_stars();
	int recenter_pix(Pixel* in, unsigned short* mask);
	int correct_pix(Pixel* in, int level);
	int detect_stars2();
	Pair rectangle(Pixel* in, unsigned short* mask);
	int psf(Pixel* out, Pair p);
	//int select_stars(int size);
	int compute_triangle();
	int compare_triangles(Triangle* other);
	Pixel* search_third_pixel(Pixel* pt1, Pixel* pt2, Triangle* tri);
	int compute_coeff();
	int translate();

	
	int debug();

};
