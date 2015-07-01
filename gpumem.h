#ifndef GPUMEM_H
#define GPUMEM_H

#include <windows.h>
#include <GL/glew.h>
#include <GL/glut.h>

struct tex
{
	tex (int w, int h, int d); // create an empty (white) 3d texture object on GPU
	tex();
	void allocTex (int w, int h, int d); // delayed init if client needs to use empty tex() ctor
	
	~tex();
		
	void makeActive();
	
	GLuint hTex; // probably don't want to access this directly
	int w, h, d;
};

struct pbo // pixel buffer object!
{
	pbo (int width, int height, int depth);
	
	~pbo();
	
	void makeActive();

	GLuint hPbo; // pbo handle, prob shouldn't use unless you are modifying recvSlice()
};

#endif
