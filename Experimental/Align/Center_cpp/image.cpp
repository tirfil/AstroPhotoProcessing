#include "image.h"


// helpers
int get_dist2(Pixel* pt1, Pixel* pt2){
	int n;
	int dist2;
		n= (pt1->x - pt2->x);
		dist2 = n*n;
		n= (pt1->y - pt2->y); 
		dist2 += n*n;
		return dist2;
}

int cramer3(int* m,int* s,double* abc){
	int detM, detA,detB,detC;
	detM = m[0]*(m[4]*m[8]-m[5]*m[7])-m[1]*(m[3]*m[8]-m[5]*m[6])+m[2]*(m[3]*m[7]-m[4]*m[6]);
	printf("detM=%d\n",detM);
	if (detM == 0) return -1;
	detA = s[0]*(m[4]*m[8]-m[5]*m[7])-m[1]*(s[1]*m[8]-m[5]*s[2])+m[2]*(s[1]*m[7]-m[4]*s[2]);
	detB = m[0]*(s[1]*m[8]-m[5]*s[2])-s[0]*(m[3]*m[8]-m[5]*m[6])+m[2]*(m[3]*s[2]-s[1]*m[6]);
	detC = m[0]*(m[4]*s[2]-s[1]*m[7])-m[1]*(m[3]*s[2]-s[1]*m[6])+s[0]*(m[3]*m[7]-m[4]*m[6]);
	abc[0]=(double)detA/(double)detM;
	abc[1]=(double)detB/(double)detM;
	abc[2]=(double)detC/(double)detM;
	return 0;
}
// methods
Image::Image(char* path) : m_width(0),m_height(0),m_pixels(NULL),m_triangles(NULL),m_reference(NULL),m_selected(NULL),
	m_image(NULL),m_image2(NULL)
{
	int rc;
	int level;
	rc = read_fits(path);
	//if (rc==0)
		//compute_stat(); // min/max
		//level = get_level(95);
		//detect_stars(level);
		////select_stars(20);
		//compute_triangle();
}

Image::~Image()
{
	Pixel* pix;
	Pixel* nextpix;
	Triangle* tri;
	Triangle* nexttri;
	if (m_image != NULL) free(m_image);
	if (m_image2 != NULL) free(m_image2);
	pix = m_pixels;
	nextpix = NULL;
	while(pix != NULL){
		nextpix = pix->ptr;
		free(pix);
		pix = nextpix;
	}
	tri = m_triangles;
	while(tri != NULL){
		nexttri = tri->ptr;
		free(tri);
		tri = nexttri;
	}
}

// public

// obtain reference triangles. 
// reference processing
Triangle* 
Image::get_reference_triangles()
{
	int level;
	int rc;
	compute_stat(); // min/max
	level = get_level(LEVEL);
	detect_stars(level);
	compute_triangle();
	return m_triangles;
}

// image processing. compute transformation coefficients. (green)
int
Image::get_coefficients(double* coefficients, Triangle* reference)
{
	int level;
	int rc;
	compute_stat(); // min/max
	level = get_level(LEVEL);
	detect_stars(level);
	//compute_triangle();
	rc = compare_triangles(reference);
	if (rc < 0 ) return -1;	
	rc = compute_coeff();
	if (rc < 0 ) return -1;
	coefficients[0] = m_xcoeff[0];
	coefficients[1] = m_xcoeff[1];
	coefficients[2] = m_xcoeff[2];
	coefficients[3] = m_ycoeff[0];
	coefficients[4] = m_ycoeff[1];
	coefficients[5] = m_ycoeff[2];
	return 0;
}

// force transformation coefficient (red, blue)
int
Image::set_coefficients(double* coefficients)
{
	m_xcoeff[0] = coefficients[0];
	m_xcoeff[1] = coefficients[1];
	m_xcoeff[2] = coefficients[2];
	m_xcoeff[0] = coefficients[0];
	m_xcoeff[1] = coefficients[1];
	m_xcoeff[2] = coefficients[2];
	return 0;
}

// write fits after transformation
int
Image::write_transform(char* path)
{
	translate();
	write_fits(path);
	return 0;
}

// private



int
Image::read_fits(char* path)
{
	fitsfile *fptr;    
    int status = 0;
    int bitpix, naxis;
    long naxes[2];
    long nelements;
    int anynul;
    unsigned short nullval=0;
    
	if (!fits_open_file(&fptr, path, READONLY, &status))
    {
		if (!fits_get_img_param(fptr, 2, &bitpix, &naxis, naxes, &status) )
        {
			printf("bitpix=%d naxes=%d width=%ld height=%ld\n",bitpix,naxis,naxes[0],naxes[1]);
			nelements = naxes[0]*naxes[1];
			if (m_width==0 && m_height==0){
				m_width = naxes[0];
				m_height = naxes[1];
			} else {
				if (m_width != naxes[0] || m_height != naxes[1]){
					printf("naxes error: %ldx%ld instead of %ldx%ld\n",naxes[0],naxes[1],m_width,m_height);
					return -1;
				}
			}
			
			m_image = (unsigned short*)malloc(sizeof(unsigned short)*nelements);
			
			if (bitpix == 16){
				fits_read_img(fptr,TUSHORT,1,nelements, &nullval, m_image, &anynul, &status);
				fits_close_file(fptr, &status);
				if (status != 0) printf("status = %d\n",status);
				return status;
			} else {
				printf("Process only 16 bit fits\n");
				fits_close_file(fptr, &status); 
				return -1;
			} 
			
		  }
     }
     return -1;		
}

int
Image::write_fits(char* path)
{
	fitsfile *fptrout;
	long naxis = 2;
    long naxes[2];
    int status = 0;
    unsigned long nelements = (unsigned long)m_width*(unsigned long)m_height;
    naxes[0] = (long)m_width;
	naxes[1] = (long)m_height;
	fits_create_file(&fptrout,path, &status);
	fits_create_img(fptrout, USHORT_IMG, naxis, naxes, &status);
	fits_write_img(fptrout, TUSHORT, (long long)1L, (long long)nelements, m_image2, &status);
	fits_close_file(fptrout, &status);
}


int
Image::compute_stat()
{
	int nelements = m_height * m_width;
	int us;
	// detect maxi/mini
	m_mini = USHRT_MAX;
	m_maxi = 0;
	for (int i=0;i<nelements;i++){
		us = m_image[i];
		//printf("%d\n",us);
		if ( us < m_mini ) m_mini = us;
		if ( us > m_maxi ) m_maxi = us;
	}
	printf("mini=%d\nmaxi=%d\n",m_mini,m_maxi);
	
	return 0;
}

int
Image::get_level(int percent)
{
	int us;
	int high;
	int low;
	int limit;
	long score;
	long nelements = m_height * m_width;
	//printf("nelements=%ld\n",nelements);
	high = m_maxi;
	low = m_mini;
	// 8 iterations
	for(int i=0; i<8; i++){
		score = 0L;
		limit = (high + low)/2;
		
		for (long i=0L;i<nelements;i++){
			us = m_image[i];
			if (us < limit) score++;
		}
		//printf("score=%ld\n",score);
		if (100L*score/nelements > (long)percent) 
		{
			high = limit;
		} else {
			low = limit;
		}
		//printf("limit=%d - %ld%%\n",limit,100L*score/nelements);
	}
	printf("star level=%d\n",limit);
	return limit;
}

//Image::detect_stars(int level)
//{
	////unsigned short* m_stars;
	//int n = 0;
	//unsigned short us;
	//long nelements = m_height * m_width;
	//int i;
	
	//m_stars = malloc(sizeof(unsigned short)*nelements);
	//for (i=0;i<nelements;i++){
		//m_stars[i] = 0;
	//}
	//for (int y=1;y<(int)m_height-1;y++)
		//for(int x=1;x<(int)m_width-1;x++){
			//i = y*(int)m_width+x;
			//us = m_image[i];
			//if (us > level)
			//{
				//m_stars[i] = us;
				//for(int y0=y-1;y0<=y+1;y0++){
					//for(int x0=x-1;x0<=x+1;x0++){
						//if (m_image[y0*(int)m_width+x0] > us)
						//{
							//m_stars[i] = 0;
							//break;
						//}
					//}
				//}
			//}
			//else
				//m_stars[i] = 0;
			

			//if (m_stars[i]!=0){
				////printf("(%d,%d)\n",x,y);
				//n++;
			//}	
		//}
						
	//printf("detect %d/%ld\n",n,nelements);
//}

int
Image::debug()
{
	Pixel* curr;
	curr = m_pixels;
	while (curr != NULL){
		printf("x %d, y %d, z %d\n",curr->x,curr->y,curr->z);
		curr = curr->ptr;
	}
}

int 
Image::correct_pix(Pixel* in, int level)
{
	int i;
	int x,y;
	unsigned short us;
	int xmin, xmax, ymin, ymax;
	int dx,dy;
	x = in->x;
	y = in->y;
	
	//printf("\r(%d,%d) -> ",x,y);
	i = y*(int)m_width+x;
	us = m_image[i];
	while(us > level){
		if (x>0){
			x--;
			i = y*(int)m_width+x;
			us = m_image[i];
		} else {
			break;
		}
	}
	xmin = x;
	x = in->x;
	i = y*(int)m_width+x;
	us = m_image[i];
	while(us > level){
		if (x<(m_width-1)){
			x++;
			i = y*(int)m_width+x;
			us = m_image[i];
		} else {
			break;
		}			
	}
	xmax = x;
	
	in->x = (xmax+xmin)/2;
	
	x = in->x;
	y = in->y;
	i = y*(int)m_width+x;
	us = m_image[i];
	while(us > level){
		if (y>0){
			y--;
			i = y*(int)m_width+x;
			us = m_image[i];
		} else {
			break;
		}
	}
	ymin = y;
	y = in->y;
	i = y*(int)m_width+x;
	us = m_image[i];
	while(us > level){
		if (y<(m_height-1)){
			y++;
			i = y*(int)m_width+x;
			us = m_image[i];
		} else {
			break;
		}			
	}
	ymax = y;	
	in->y = (ymax+ymin)/2;
	//printf("(%d,%d)",in->x,in->y);
	// diameter min
	dx = xmax-xmin;
	dy = ymax-ymin;
	if (dx > dy)
		in->z = dy;
	else
		in->z = dx;
	
}

int
Image::detect_stars(int level)
{
	Pixel* root;
	Pixel* curr;
	unsigned short us;
	unsigned short us0;
	Pixel* ptr1;
	Pixel* ptr2;
	Pixel* prev;
	
	bool match;
	int i;
	int n=0;
	root = NULL;
	curr = NULL;

	for (int y=1;y<(int)m_height-1;y++){
		for(int x=1;x<(int)m_width-1;x++){
			i = y*(int)m_width+x;
			us = m_image[i];
			if (us > level)
			{
				match = true;
				for(int y0=y-1;y0<=y+1;y0++){
					for(int x0=x-1;x0<=x+1;x0++){
						us0 = m_image[y0*(int)m_width+x0];
						if ((us0 > us) || (us0 < level))
						{
							match = false;
							break;
						}
					}
					if (match == false) break;
				}
				if (match == true)
				{
					if (root == NULL){
						curr = (Pixel*)malloc(sizeof(Pixel));
						root = curr;
					} else {
						curr->ptr = (Pixel*)malloc(sizeof(Pixel));
						curr = curr->ptr;
					}
					curr->x = x;
					curr->y = y;
					curr->ptr = NULL;
					n++;
					//printf("\r(%d,%d) %d",x,y,n);
				}
			}	
		}	
	}
	printf("\ndetect %d raw point(s)\n",n);
	
	// correct
	//printf("-");
	curr = root;
	while(curr != NULL){
		correct_pix(curr,level);
		curr = curr->ptr;
	}
	//printf("\n");
	// too small
	curr = root;
	prev = root;
	while(curr != NULL){
		if ((curr->z < MIN_PIX)||(curr->z > MAX_PIX)){
			n--;
			if (prev == root){
				root = curr->ptr;
				free(curr);
				curr = root;
				prev = root;
			} else {
				prev->ptr = curr->ptr;
				free(curr);
				curr = prev->ptr;
				//prev = prev;
			}
		} else {
			prev = curr;
			curr = curr->ptr;
		}
	}
	
	printf("\ndetect %d point(s)\n",n);	
	
	// clean
	//printf("-");
	ptr1 = root;
	while(ptr1 != NULL){
		prev = ptr1;
		ptr2 = ptr1->ptr;
		while(ptr2 != NULL){
			if (get_dist2(ptr1,ptr2) < 2){
				// remove 
				prev->ptr = ptr2->ptr;
				free(ptr2);
				ptr2 = prev->ptr;
				n--;
				//printf("\r%d",n);
			} else {
				prev = ptr2;
				ptr2 = ptr2->ptr;
			}
		}
		ptr1 = ptr1->ptr;
	}
	printf("\ndetect %d point(s)\n",n);	
	
	m_pixels = root;
	//debug();
}

//int
//Image::detect_stars(int level)
//{
	//Pixel px;
	////m_pixels = malloc(sizeof(Pixel)*NB_STARS);
	
	//Pixel* curr = NULL;
	//Pixel* ptr = NULL;
	//int pp=0;
	//int n=0;
	//int pt;
	//long dist2;
	//int x,y;
	//unsigned short us;
	//int store = 0;
	//bool out = false;
	
	//int nelements = m_height * m_width;
	//for (int i=0;i<nelements;i++){
		//us = m_image[i];
		//if (i%m_width==0)
			//n=0;
		//if (us>level){
			//n++;
		//} else {
			//if ((n>0) && (n>store)) {
				//store = n;
			//}
			//n=0;
		//}
	//}

	//do {
		////printf("store=%d\n",store);
		//for (int i=0;i<nelements;i++){
			//us = m_image[i];
			//if (i%m_width==0) n=0;
			//if (us>level){
				//n++;
				//if(store==n){
					//pt=i-(store/2);
					//px.y = pt / m_width;
					//px.x = pt % m_width;
					//px.z = store; 
					//if (pp==0){
						//m_pixels = (Pixel*)malloc(sizeof(Pixel));
						////curr = new(Pixel);
						//curr = m_pixels;
						//curr->x=px.x;
						//curr->y=px.y;
						//curr->z=px.z;
						//curr->ptr=NULL;
						//printf("0: (%d,%d) - %d\n",px.x,px.y,px.z);
						////debug();
						//pp++;
					//} else {
						//ptr = m_pixels;
						////for(int j=0;j<pp;j++)
						//while(ptr != NULL)
						//{
							//// compute distance
							////printf("compute distance: %d %d\n",i,pp);
							//dist2 = get_dist2(&px,ptr);
							
							////n=(px.x-(ptr->x));
							////dist2 = n*n;
							////n=(px.y-(ptr->y)); 
							////dist2 += n*n;
							
							//if (dist2<=1000)
							//{
								//dist2=0;
								//break;
							//}
							//ptr = ptr->ptr;
						//}
						
						//if (dist2>0){
							////printf("next point\n");
							//if ( px.z < MIN_PIX) {
								////printf("Go out\n");
								//out = true;
								//curr->ptr = NULL;
								//break;
							//} 
							////debug();
							//if (curr->ptr == NULL) curr->ptr = (Pixel*)malloc(sizeof(Pixel));
							////curr->ptr = new(Pixel);
							//curr = curr->ptr;
							//curr->x=px.x;
							//curr->y=px.y;
							//curr->z=px.z;
							//curr->ptr=NULL;
							//printf("%d: (%d,%d) - %d\n",pp,px.x,px.y,px.z);
							//pp++;
						//}						
					//}
				//}
			//} else {
				//n=0;
			//}
		//}
		//store--;
	//} while((pp < NB_STARS) && (out == false));
//}

//Image::select_stars(int size)
//{
	//// filter 
	//unsigned short us;
	//int pos;
	//long nelements = m_height * m_width;
	//int upper = INT_MAX;
	//int j = 0;
	//int maxi = 0;
	
	//for (int i=0;i<size;i++){
		//for(int k=0;k<nelements;k++){
			//us = m_stars[k];
			//if ((maxi < us) && (us < upper)){
				//maxi = us;
				//pos = k;
			//}
		//}
		//upper=maxi;
		//maxi = 0;
		//if (upper == 0) break;
		
		////pxa[j].x = pos % width;
		////pxa[j].y = pos / width;
		//j++;
		//printf("%d:\t(%d,%d)\t- %d\n",j,pos % m_width, pos / m_width,upper);
	//}
//}

int	
Image::compute_triangle()
{
	Pixel* ptr1;
	Pixel* ptr2; 
	Pixel* ptr3;
	Pixel* pt1;
	Pixel* pt2;
	Pixel* pt3;
	int sum;
	int val0;
	int val1;
	int base;
	int side0, side1;
	int sumax;
	int swap;
	Pixel* swapp;
	Triangle* tri; 
	
	ptr1 = m_pixels;
	while(ptr1 != NULL)
	{	
		//ptr2 = m_pixels;
		pt1 = NULL;
		pt2 = NULL;
		pt3 = NULL;
		//while((ptr2 != NULL) && (ptr1 != ptr2))
		//{
			//ptr2 = ptr2->ptr;
		//}
		ptr2 = ptr1->ptr;
		if (ptr2 == NULL) break;
		base = 0;
		sumax = 0;
		while(ptr2 != NULL)
		{
			//if (ptr1 == ptr2) continue;
			val0 = get_dist2(ptr1,ptr2);
			if (val0 > base){
				base = val0;
				pt1 = ptr1;
				pt2 = ptr2;
			}
			ptr2 = ptr2->ptr;
		}

		base = (int)sqrt(base);
		//printf("base=%d (%d,%d)(%d,%d)\n",base,pt1->x,pt1->y,pt2->x,pt2->y);
		//printf("base OK\n");
		ptr3 = m_pixels;
		while(ptr3 != NULL)
		{				
			if ((ptr3 == pt1) || (ptr3 == pt2)){
				ptr3 = ptr3->ptr;
				continue; 
			}
			val0 = get_dist2(pt1,ptr3);
			val1 = get_dist2(pt2,ptr3);
			val0 = (int)sqrt(val0);
			val1 = (int)sqrt(val1);
			sum = val0 + val1;
			if (120*base > 100*sum) {
				ptr3 = ptr3->ptr;
				continue; 
			}
			if (sumax < sum){
				sumax = sum;
				pt3 = ptr3;
				side0 = val0;
				side1 = val1;				
			}
			ptr3 = ptr3->ptr;
		}
		if (pt3 != NULL) {
			//if (base < side0){
				//swap = base;
				//base = side0;
				//side0 = swap;
			//}
			if (m_triangles == NULL) {
				m_triangles = (Triangle*)malloc(sizeof(Triangle));
				tri = m_triangles;

			
			} else {
				tri->ptr = (Triangle*)malloc(sizeof(Triangle));
				tri = tri->ptr;
			}
			
			//printf("%d,%d,%d (%d,%d)(%d,%d)(%d,%d)\n",base,side0,side1,pt1->x,pt1->y,pt2->x,pt2->y,pt3->x,pt3->y);
			// rearrange (x)

			
			if ((pt1->x) > (pt2->x)){
				swap = side0;
				side0 = side1;
				side1 = swap;
				swapp = pt1;
				pt1 = pt2;
				pt2 = swapp;
			}	
			if ((pt2->x) > (pt3->x)){
				swap = base;
				base = side0;
				side0 = swap;
				swapp = pt2;
				pt2 = pt3;
				pt3 = swapp;
			}							
			if ((pt1->x) > (pt2->x)){
				swap = side0;
				side0 = side1;
				side1 = swap;
				swapp = pt1;
				pt1 = pt2;
				pt2 = swapp;
			}								

			tri->ptr = NULL;
			tri->da = base; tri->db = side0; tri->dc = side1;
			tri->pa = pt1; tri->pb = pt2; tri->pc = pt3;

			
			printf("%d,%d,%d (%d,%d)(%d,%d)(%d,%d)\n",base,side0,side1,pt1->x,pt1->y,pt2->x,pt2->y,pt3->x,pt3->y);
			//printf("check: %d,%d,%d\n",(int)sqrt((double)get_dist2(pt1,pt2)),(int)sqrt((double)get_dist2(pt1,pt3)),(int)sqrt((double)get_dist2(pt2,pt3)));
		}
		ptr1= ptr1->ptr;
		
	}
	
	//printf("out compute triangle\n");
}


	
//int
//Image::compare_triangles(Triangle* other){
	//Triangle* ptr1;
	//Triangle* ptr2;
	//m_reference = NULL;
	//m_selected = NULL;
	//ptr1 = other; // reference
	//ptr2 = m_triangles;
	//bool match = false;
	//while((ptr1 != NULL) && (match == false)){
		//while((ptr2 != NULL) && (match == false)){
			//if ((abs((ptr1->da)-(ptr2->da)) > DLEN) ||
			    //(abs((ptr1->db)-(ptr2->db)) > DLEN) ||
			    //(abs((ptr1->dc)-(ptr2->dc)) > DLEN))
			//{
				//ptr2 = ptr2->ptr;
				//continue;
			//}
			//match = true;
			//m_reference = ptr1;
			//m_selected = ptr2;
		//}
		//ptr1 = ptr1->ptr;
	//}
	//if (match == false) return -1;
	//printf("Mapping:\n");
	//printf("(%d,%d) -> (%d,%d)\n",m_selected->pa->x,m_selected->pa->y,m_reference->pa->x,m_reference->pa->y);
	//printf("(%d,%d) -> (%d,%d)\n",m_selected->pb->x,m_selected->pb->y,m_reference->pb->x,m_reference->pb->y);
	//printf("(%d,%d) -> (%d,%d)\n",m_selected->pc->x,m_selected->pc->y,m_reference->pc->x,m_reference->pc->y);
	//return 0;
//}

Pixel*
Image::search_third_pixel(Pixel* pt1, Pixel* pt2, Triangle* tri)
{
	Pixel* ptr3 = m_pixels;
	Pixel* pt3;
	int dist2 = (tri->db);
	int dist3 = (tri->dc);
	while(ptr3 != NULL){
		if (abs(dist2-(int)sqrt((double)get_dist2(pt1,ptr3))) > 0){
			ptr3 = ptr3->ptr;
		} else {
			pt3 = ptr3;
			// check
			if (abs(dist3-(int)sqrt((double)get_dist2(pt2,pt3))) > 0){
				ptr3 = ptr3->ptr;
				continue;
			} else {
				//printf("OK\n");
				return pt3;
			}
		}		
	}
	return NULL;
}

int
Image::compare_triangles(Triangle* other){
	Triangle* tri;
	Pixel* ptr1;
	Pixel* ptr2;
	//Pixel* ptr3;
	//Pixel* tmp;
	tri = other;
	bool match = false;
	int dist2;

	Pixel* pt1 = NULL;
	Pixel* pt2 = NULL;
	Pixel* pt3 = NULL;
	
	//int aa;

	
	while (tri != NULL)
	{
		// base matching
		//printf("base matching\n");
		dist2=(tri->da);
		ptr1 = m_pixels;
		match = false;
		while (ptr1 != NULL)
		{
			ptr2 = ptr1->ptr;
			//printf("\n");
			//aa = 0;
			while(ptr2 != NULL)
			{
				//printf("\r%d",aa++);
				//printf("dist2=%d (%d,%d)(%d,%d) %d\n",dist2,ptr1->x,ptr1->y,ptr2->x,ptr2->y,get_dist2(ptr1,ptr2));
				if (abs(dist2-(int)sqrt((double)get_dist2(ptr1,ptr2))) > 0){
					ptr2 = ptr2->ptr;
				} else {
					if ((ptr1->x) > (ptr2->x)) {
						pt1 = ptr2;
						pt2 = ptr1;
					} else {
						pt1 = ptr1;
						pt2 = ptr2;
					}
					// side0 matching
					//printf("base :(%d,%d) -> (%d,%d) - %d\n",pt1->x,pt1->y,tri->pa->x,tri->pa->y,(int)sqrt((double)get_dist2(pt1,pt2)));
					//printf("     :(%d,%d) -> (%d,%d) - %d\n",pt2->x,pt2->y,tri->pb->x,tri->pb->y,(int)sqrt((double)get_dist2(tri->pa,tri->pb)));
					pt3 = search_third_pixel(pt1,pt2,tri);
					if (pt3 == NULL){
						ptr2 = ptr2->ptr;
						continue;
					} else {
						match=true;
						break;
					}
				}
			}
			if (match == false){
				//printf("next origin\n");
				ptr1 = ptr1->ptr; // next origin
				continue;
			} else {
				break;
			}
		}
		if (match == false){
			//printf("next triangle\n");
			tri = tri->ptr; // next triangle
			continue;
		} else {
			match=true;
			break;
		}
		//printf("base :(%d,%d) -> (%d,%d)\n",pt1->x,pt1->y,tri->pa->x,tri->pa->y);
		//printf("     :(%d,%d) -> (%d,%d)\n",pt2->x,pt2->y,tri->pb->x,tri->pb->y);
		//// side0 matching	
		////printf("side0 matching\n");
		//ptr3 = m_pixels;
		//dist2 = (tri->db)*(tri->db);
		//match = false;
		//while(ptr3 != NULL){
			//if (abs(dist2-get_dist2(pt1,ptr3)) > DLEN){
				//ptr3 = ptr3->ptr;
			//} else {
				//pt3 = ptr3;
				//// check
				//dist2 = (tri->dc)*(tri->dc);
				//if (abs(dist2-get_dist2(pt2,pt3)) > DLEN){
					//ptr3 = ptr3->ptr;
					//continue;
				//} else {
					//match = true;
					//break;
				//}
			//}		
		//}
		//if (match == false){
			//tri = tri->ptr; // next triangle
			//continue;
		//} else {
			//break;
		//}		
	}
	if (match == false){
		printf("No match\n");
		return -1;
	} else {
		m_reference = tri;
		m_selected = (Triangle*)malloc(sizeof(Triangle));
		m_selected->pa = pt1;
		m_selected->pb = pt2;
		m_selected->pc = pt3;
		m_selected->da = (int)sqrt((double)get_dist2(pt1,pt2));
		m_selected->db = (int)sqrt((double)get_dist2(pt1,pt3));
		m_selected->dc = (int)sqrt((double)get_dist2(pt2,pt3));
		printf("Mapping:\n");
		printf("(%d,%d) -> (%d,%d)\n",m_selected->pa->x,m_selected->pa->y,m_reference->pa->x,m_reference->pa->y);
		printf("(%d,%d) -> (%d,%d)\n",m_selected->pb->x,m_selected->pb->y,m_reference->pb->x,m_reference->pb->y);
		printf("(%d,%d) -> (%d,%d)\n",m_selected->pc->x,m_selected->pc->y,m_reference->pc->x,m_reference->pc->y);
		//printf("sel: %d-%d-%d\n",m_selected->da,m_selected->db,m_selected->dc);
		//printf("ref: %d-%d-%d\n",m_reference->da,m_reference->db,m_reference->dc);
		return 0;		
	}
}


int 
Image::compute_coeff() {
	int m3x3[9];
	int m3x1[3];
	int rc;
	
	m3x3[0]=m_selected->pa->x;
	m3x3[1]=m_selected->pa->y;
	m3x3[2] = 1;
	m3x1[0]=m_reference->pa->x;
	
	m3x3[3]=m_selected->pb->x;
	m3x3[4]=m_selected->pb->y;
	m3x3[5] = 1;
	m3x1[1]=m_reference->pb->x;
	
	m3x3[6]=m_selected->pc->x;
	m3x3[7]=m_selected->pc->y;
	m3x3[8] = 1;
	m3x1[2]=m_reference->pc->x;
	
	rc = cramer3(m3x3,m3x1,m_xcoeff);
	if (rc < 0) return -1;
	
	printf("x'= %f*x + %f*y + %f\n",m_xcoeff[0],m_xcoeff[1],m_xcoeff[2]);
	
	if (m_xcoeff[0] < 0.9) return -1;
	if (m_xcoeff[0] > 1.1) return -1;
	
	//m3x3[0]=m_selected->pa->x;
	//m3x3[1]=m_selected->pa->y;
	//m3x3[2] = 1;
	m3x1[0]=m_reference->pa->y;
	
	//m3x3[3]=m_selected->pb->x;
	//m3x3[4]=m_selected->pb->y;
	//m3x3[5] = 1;
	m3x1[1]=m_reference->pb->y;
	
	//m3x3[6]=m_selected->pc->x;
	//m3x3[7]=m_selected->pc->y;
	//m3x3[8] = 1;
	m3x1[2]=m_reference->pc->y;
	
	rc = cramer3(m3x3,m3x1,m_ycoeff);
	if (rc < 0) return -1;
	
	printf("y'= %f*x + %f*y + %f\n",m_ycoeff[0],m_ycoeff[1],m_ycoeff[2]);
	
	if (m_ycoeff[1] < 0.9) return -1;
	if (m_ycoeff[1] > 1.1) return -1;
	
	sleep(1);
	
	return 0;

}

int
Image::translate()
{	
	int nelements;
	int i;
	int x,y;
	int u,v;
	
	nelements = (int)m_width*(int)m_height;
	m_image2 = (unsigned short*)malloc(sizeof(unsigned short)*nelements);
	// init
	for(i=0;i<nelements;i++)
		m_image2[i]=0;
	// convert
	for(y=0;y<m_height;y++)
	for(x=0;x<m_width;x++){
		u = floor(0.5+m_xcoeff[0]*(double)x+m_xcoeff[1]*(double)y+m_xcoeff[2]);
		if (u<0 || u>=m_width) continue;
		v = floor(0.5+m_ycoeff[0]*(double)x+m_ycoeff[1]*(double)y+m_ycoeff[2]);
		if (v<0 || v>=m_height) continue;
		m_image2[u+v*m_width]=m_image[x+y*m_width];
	}
	return 0;
}

int
Image::save_triangles(char* path)
{
	Triangle* tri;
	FILE* fd;
	tri = m_triangles;
	fd=fopen(path,"w+");
	while (tri != NULL){
		fprintf(fd,"%d|%d|%d|%d|%d|%d|%d|%d|%d\n",tri->da,tri->db,tri->dc,tri->pa->x,tri->pa->y,tri->pb->x,tri->pb->y,tri->pc->x,tri->pc->y);
		tri = tri->ptr;
	}
	fclose(fd);
}

Triangle*
Image::load_triangles(char* path){
	Triangle* tri;
	Triangle* root;
	int da,db,dc,pax,pay,pbx,pby,pcx,pcy;
	char data[256];
	FILE* fd;
	root = NULL;
	tri = root;
	fd=fopen(path,"r");
	while (fgets(data,256,fd) != NULL){
		sscanf(data,"%d|%d|%d|%d|%d|%d|%d|%d|%d",&da,&db,&dc,&pax,&pay,&pbx,&pby,&pcx,&pcy);
		if (root == NULL){
			root = (Triangle*)malloc(sizeof(Triangle));
			tri = root;
		} else {
			tri->ptr = (Triangle*)malloc(sizeof(Triangle));
			tri = tri->ptr;
			
		}
		tri->da=da; tri->db=db; tri->dc=dc;
		tri->pa = (Pixel*)malloc(sizeof(Pixel));
		tri->pb = (Pixel*)malloc(sizeof(Pixel));
		tri->pc = (Pixel*)malloc(sizeof(Pixel));
		tri->pa->x = pax; tri->pa->y = pay;
		tri->pb->x = pbx; tri->pb->y = pby;
		tri->pc->x = pcx; tri->pc->y = pcy;		
		tri->ptr = NULL;
	}
	fclose(fd);
	return root;
}

int 
Image::save_coeff(char* path){
	FILE* fd;
	fd=fopen(path,"w");
	fprintf(fd,"%f|%f|%f|%f|%f|%f\n",m_xcoeff[0],m_xcoeff[1],m_xcoeff[2],m_ycoeff[0],m_ycoeff[1],m_ycoeff[2]);
	fclose(fd);
}

int
Image::load_coeff(char* path){
	char data[256];
	FILE* fd;
	double x1,x2,x3,y1,y2,y3;
	fd=fopen(path,"r");	
	while (fgets(data,256,fd) != NULL){
		sscanf(data,"%lf|%lf|%lf|%lf|%lf|%lf",&x1,&x2,&x3,&y1,&y2,&y3);
		m_xcoeff[0]=x1; m_xcoeff[1]=x2; m_xcoeff[2]=x3;
		m_ycoeff[0]=y1; m_ycoeff[1]=y2; m_ycoeff[2]=y3;
	}
	fclose(fd);
}
	

