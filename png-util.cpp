#include <iostream>
#include <cstdlib>
#include "png-util.h"

using namespace std;

png::png (const char *filename) 
:png_ptr(0),
 info_ptr(0),
 end_info(0)
{
	this->loadImage (filename);
}

png::png()
:png_ptr(0),
 info_ptr(0),
 end_info(0)
{ }

void png::loadImage(const char *filename)
{
	// in case we are loading a new image on top of an old one
	if (png_ptr != NULL || info_ptr != NULL || end_info != NULL)
	{
		png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);
	}
	
	FILE *fp = fopen (filename, "rb");
	if (!fp) {
		cout << "Could not open file." << endl;
		exit(1);
	}
	
		// - setup reading structures ---
	png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		cout << "Could not initialize .png library." << endl;
		exit(1);
	}
	
	info_ptr = png_create_info_struct (png_ptr);
	if (info_ptr == NULL) {
		png_destroy_read_struct (&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
		exit(1);
	}

	end_info = png_create_info_struct (png_ptr);
	if (end_info == NULL) {
		png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp) NULL);
		exit(1);
	}
	
	// - setup io ---
	png_init_io (png_ptr, fp);
	
	// - read image into memory ---
	png_read_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
	fclose (fp);
}
	
png::~png() 
{
	png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);
}
	
GLubyte png::pixelAt (int x, int y) 
{
	assert (0 <= x && x < info_ptr->width);
	assert (0 <= y && y < info_ptr->height);

	// index from top left, [r][c*3+offset] == (r,c)		
	static GLubyte r, g, b;
	r = info_ptr->row_pointers[y][x*4];
	g = info_ptr->row_pointers[y][x*4+1];
	b = info_ptr->row_pointers[y][x*4+2];
	
	// return r << 24 | g << 16 | b << 8 | 0xff; // assumes 4-byte integers
	return r;
}
	
int png::getWidth() 
{
	return info_ptr->width;
}
	
int png::getHeight() 
{
	return info_ptr->height;
}
