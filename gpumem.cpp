#include <cassert>
#include "gpumem.h"

tex::tex (int w, int h, int d) 
{	
	this->allocTex (w, h, d);
}

tex::tex()
{
	hTex = NULL;
	w = 0; h = 0; d = 0;
}

void tex::allocTex (int w, int h, int d)
{
	//- Allocate graphics card memory
	assert (0 <= w);
	assert (0 <= h);
	assert (0 <= d);
	this->w = w; this->h = h; this->d = d;
	 
	 
	glGenTextures (1, &this->hTex);
	this->makeActive();

//- Specify filtering and edge actions
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE); //GL_MODULATE, GL_REPLACE, consider using modulate for slice compositing
	glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // vs. GL_LINEAR
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
	
//- Load (empty) texture data
	GLubyte *tex_data;	
		tex_data = (GLubyte *) malloc (w * h * d * sizeof(GLubyte));
		memset (tex_data, 0xff, w * h * d * sizeof(GLubyte));	

//- Create the texture
	glTexImage3D (GL_TEXTURE_3D, 0, 1, this->w, this->h, this->d, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, tex_data); // allocate GPU texture memory
		
//- Deallocate raw data	
	free (tex_data);
}

tex::~tex() 
{
	glBindTexture (GL_TEXTURE_3D, 0);
	glDeleteTextures (1, &this->hTex);
}
	
void tex::makeActive() 
{
	glBindTexture (GL_TEXTURE_3D, this->hTex);
}


// = PBO =======================================================
// pixel buffer object!
pbo::pbo (int width, int height, int depth) { // - allocate and identify a (gpu-controlled) buffer
	glGenBuffersARB (1, &this->hPbo); // no. of objs, mem to store objs
	this->makeActive();
	
	// reserve space, but don't copy anything (will do that via a memmap, i.e. mapbuffer)
	GLubyte *buf_data;	
		buf_data = (GLubyte *) malloc (width * height * depth * sizeof(GLubyte));
		memset (buf_data, 0x7f, width * height * depth * sizeof(GLubyte));	
		
	//glBufferDataARB (GL_PIXEL_UNPACK_BUFFER_ARB, width * height * depth * sizeof(GLubyte), NULL, GL_STREAM_DRAW_ARB);
	glBufferDataARB (GL_PIXEL_UNPACK_BUFFER_ARB, width * height * depth * sizeof(GLubyte), buf_data, GL_STREAM_DRAW_ARB);
	
	free (buf_data);
}

pbo::~pbo() {
	// - cleanup ---
	glBindBufferARB (GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	glDeleteBuffersARB (1, &this->hPbo);
}

void pbo::makeActive() {
	glBindBufferARB (GL_PIXEL_UNPACK_BUFFER_ARB, this->hPbo);
}