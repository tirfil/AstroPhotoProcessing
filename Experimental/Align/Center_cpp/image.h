#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include "fitsio.h"
#include <limits.h>
#include <math.h>

#define NB_STARS 20
#define MIN_PIX 5+2
#define MAX_PIX 15+2
#define DLEN 5
#define LEVEL 98

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
	
private:

	long m_width, m_height;
	int m_mini, m_maxi;
	unsigned short* m_image;
	unsigned short* m_image2;
	//unsigned short* m_stars;
	Pixel* m_pixels;
	Triangle* m_triangles;
	Triangle* m_reference;
	Triangle* m_selected;
	double m_xcoeff[3];
	double m_ycoeff[3];

	int read_fits(char* path);
	int write_fits(char* path);
	int compute_stat();
	int get_level(int percent);
	int detect_stars(int level);
	int correct_pix(Pixel* in, int level);
	//int select_stars(int size);
	int compute_triangle();
	int compare_triangles(Triangle* other);
	Pixel* search_third_pixel(Pixel* pt1, Pixel* pt2, Triangle* tri);
	int compute_coeff();
	int translate();

	
	int debug();

};
