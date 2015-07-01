#ifndef SHADER_UTIL_H_
#define SHADER_UTIL_H_

#include <windows.h>
#include <GL/glew.h>

// - Thanks to lighthouse3d.com! ---

extern GLuint v, f, p;

char *readSource (char *filename);
GLuint setShaders (char *vert_fn, char *frag_fn);

//void printShaderInfoLog (GLuint obj);
//void printProgramInfoLog (GLuint obj);

#endif //SHADER_UTIL_H_
