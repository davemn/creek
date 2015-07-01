#ifndef PNG_UTIL_H
#define PNG_UTIL_H

#include "windows.h"
#include "GL/gl.h"
#include "png.h"

struct png
{
	png (const char *filename);
	png();
	void loadImage(const char *filename);
	
	~png();
	
	GLubyte pixelAt (int x, int y);
	
	int getWidth();
	
	int getHeight();
	
	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info;
};

#endif
