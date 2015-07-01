#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <iostream>

#include <windows.h>
#include <GL/glew.h>
#include <GL/glut.h>

#include "gpumem.h"
#include "png-util.h"
#include "shader-util.h"

using namespace std;

struct vars
{
	vars(): 
		y_angle(0), 
		y_cam(0), 
		fbo(0), 
		backface_tex(0), backface_uniform(0), 
		frontface_tex(0), frontface_uniform(0),
		volume_uniform(0)
	{ }

	~vars() 
	{
		if (glIsTexture(backface_tex) == GL_TRUE)
			glDeleteTextures (1, &backface_tex);
		if (glIsTexture(frontface_tex) == GL_TRUE)
			glDeleteTextures (1, &frontface_tex);

		glDeleteFramebuffersEXT (1, &fbo);

		//free (volume);
		glDeleteProgram (program);
		glutDestroyWindow (window_handle);
	}

	static const int width = 1024; //256 ` 256
	static const int height = 1024;

	int window_handle;

	int y_angle;
	int y_cam;

	GLuint program;
	//GLubyte *volume;

	GLuint fbo;
	GLuint backface_tex, backface_uniform;
	GLuint frontface_tex, frontface_uniform;
	
	tex volume_tex;
	GLuint volume_uniform;

} env;

// meant to be statically allocated and overwritten w/ new slice data (serves as a buffer)
struct slice 
{
	size_t count; // no. of pixels in the current slice
	unsigned char colors[262144]; // 256 kb (512*512), [L L L ...] (luminance)
	int locs[786432]; // 3 mb (assuming 32-bit integer), [X Y Z X Y Z ...]
};


// = FUNCTIONS =================================================

static void checkErrors()
{
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		cout << "  " << gluErrorString (err) << endl;
	}
}

static void init(void) 
{
/*	GLint maxbuffers;
	glGetIntegerv (GL_MAX_COLOR_ATTACHMENTS_EXT, &maxbuffers);
	cout << "Max attachable color buffers: " << maxbuffers << endl;
*/
	env.y_angle = 90;
	env.y_cam = 0;

	// - state -------------------------------------------------
		glReadBuffer (GL_BACK);

	// - shaders -----------------------------------------------
		env.program = setShaders ("subtract.vert", "subtract.frag");

		env.backface_uniform = glGetUniformLocation (env.program, "backface");
		env.frontface_uniform = glGetUniformLocation (env.program, "frontface");
		env.volume_uniform = glGetUniformLocation (env.program, "volume");
	
	// - framebuffer -------------------------------------------
		glGenFramebuffersEXT (1, &env.fbo);
		glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, env.fbo);
			// - Backface ---
			glActiveTexture (GL_TEXTURE0); //select the 1st texture unit
				glGenTextures (1, &env.backface_tex);
				glBindTexture (GL_TEXTURE_2D, env.backface_tex);
					glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
					glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
					glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, env.width, env.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
				glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, env.backface_tex, 0);

			// - Frontface ---
			glActiveTexture (GL_TEXTURE1); //select the 2nd texture unit
				glGenTextures (1, &env.frontface_tex);
				glBindTexture (GL_TEXTURE_2D, env.frontface_tex);
					glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
					glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
					glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, env.width, env.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
				glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_TEXTURE_2D, env.frontface_tex, 0);
			
			// - ultrasound data ---
			glActiveTexture (GL_TEXTURE2);
				// env.volume_tex.allocTex (512, 512, 512);
				env.volume_tex.allocTex (128, 128, 128);
				// env.volume_tex.allocTex (64, 64, 64);
				//glFramebufferTexture3DEXT (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT2_EXT, GL_TEXTURE_3D, env.volume_tex, 0, 0);

		glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);

		//for some reason, w/e texture is last bound in texture unit 0 attaches to all the samplers, no matter what u give to glUniform
		//seems like i'm not correctly attaching the texture units to the uniforms, and it's just reverting to default behavior (i.e. 
		//  what's bound in texture unit 0 is what all the samplers read from)

		glUseProgram (env.program);
			glUniform1i (env.backface_uniform, 0);  //set the sampler to be the rendered backface texture
			glUniform1i (env.frontface_uniform, 1); //set the sampler to be the rendered frontface texture
			glUniform1i (env.volume_uniform, 2);    //set the sampler to be the mri data texture
		glUseProgram (0);

	// - fragment ops ------------------------------------------
		glClearColor (0.0, 0.0, 0.0, 1.0);

	// - transformations ---------------------------------------
		glMatrixMode (GL_PROJECTION);
		glLoadIdentity();
		gluPerspective (60.0, 1.0, 1.0, 100.0);
}

static void recvSlice (slice &sliceBuf)
{
	// - maintain accumulated image in pixel buffer (object) ---
	static pbo oBuffer (env.volume_tex.w, env.volume_tex.h, env.volume_tex.d); // avoid mangling the namespace by allowing only the recv func access to the buffer
		oBuffer.makeActive();
// cout << "-Allocated pixel buffer object" << endl;
checkErrors();

	
	// - write data to gpu-controlled memory ---
	GLubyte *buffer;
	buffer = (GLubyte *) glMapBufferARB (GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
	if (buffer!= NULL) 	{
		// begin writing data (copy each slice pixel to a buffer location)
		for (int i = 0; i < sliceBuf.count; i++) { // for each pixel in the slice
/*
if (i % 256 == 0)
{
	printf ("\n");
}
if (i % 65536 == 0)
{
	printf ("\n");
}
// printf ("[%02d %02d %02d] ", sliceBuf.locs[i*3], sliceBuf.locs[i*3+1], sliceBuf.locs[i*3+2]);
printf ("[%03d %03d %03d] ", sliceBuf.locs[i*3], sliceBuf.locs[i*3+1], sliceBuf.locs[i*3+2]);
*/

			// a slice may contain pixels that are out of bounds for the current pbo
			// will implement memory expansion, until then we throw away out-of-bounds pixels
			
			if (0 <= sliceBuf.locs[i*3] && sliceBuf.locs[i*3] < env.volume_tex.w) { // if x & y & z of the current pixel are within the buffer/tex bounds
				if (0 <= sliceBuf.locs[i*3+1] && sliceBuf.locs[i*3+1] < env.volume_tex.h) {
					if (0 <= sliceBuf.locs[i*3+2] && sliceBuf.locs[i*3+2] < env.volume_tex.d) {
						// for each slice pixel's location, write that pixel's color to the corresponding location in the mapped buffer
						// modify this to actually composite instead of overwrite
						
						buffer [(sliceBuf.locs[i*3+2] * env.volume_tex.w * env.volume_tex.h) // slice::locs is in gl-tex coords, [z*w*h + y*w + x] <=> (x,y,z)
							+ (sliceBuf.locs[i*3+1] * env.volume_tex.w) // y
							+ sliceBuf.locs[i*3]] // z
							= sliceBuf.colors[i];
					}
				}
			}
			
			// will need a 2nd buffer object to hold hit counts for compositing (divide by counts)
		}
	}
// cout << "-Wrote data to buffer" << endl;
checkErrors();
	glUnmapBufferARB (GL_PIXEL_UNPACK_BUFFER_ARB); // return control to the GPU
	
	
	// - copy voxels from pbo to texture object ---
	
	env.volume_tex.makeActive();
	// don't need to reallocate the drawn texture for each update, rather overwrite it
	// subImage uses DMA, so this call shouldn't involve the cpu
	glTexSubImage3D (GL_TEXTURE_3D, 0, 0, 0, 0, env.volume_tex.w, env.volume_tex.h, env.volume_tex.d, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);
	
	glutPostRedisplay();
}

// every 2 seconds, generates a slice (line) from an image and sends it to recvSlice, 
// where it is joined w/ the current image region on the GPU
void generateSlice (int sliceIdx)
{
#if defined(READ_IMG)
	// sliceIdx is the index of the current slice (not of the current image)
	static char imgName[64]; 
	static int imgIdx = 0;
	if (sliceIdx % 5 == 0) { // every five slices we choose a new image
		sprintf (imgName, "axial/axial_%03d.png", imgIdx); // magic filename!!!
		imgIdx++;
		imgIdx %= 27;
	}
//cout << "Loading: " << imgName << endl;
	static png img;
		img.loadImage (imgName);
	
	// read image as each slice (temporary)
	static int w, h, wh;
		w = img.getWidth();
		h = img.getHeight();
		wh = w * h;
	static int z = 0;
	static slice sliceBuf;
		sliceBuf.count = w * h;
	double invW = 1.0 / w;
	double invWH = 1.0 / (w*h);
		
	for (int i = 0; i < sliceBuf.count; i++) {
		// .png index from upper left, gl from lower left
		// convert to gl indexing, and lay out pixels flat in the slice
	#ifdef IDX
		sliceBuf.colors[i] = img.pixelAt (i%w, h - ((i - i%w) * invW) - 1);
		
		sliceBuf.locs[i*3] = i%w;
		// may be partially wrong, may be writing some coords to be mirror-images of where they should be
		sliceBuf.locs[i*3+1] = (i - i%w) * invW; // flip y-axis to match opengl texture origin (bottom left)		
		sliceBuf.locs[i*3+2] = z;

	#elseif defined(IDX_2)
		sliceBuf.colors[i] = img.pixelAt (i%w, (int) (h - ((i - i%w) * invW) - 1) % h); // how do modulo & the complex nos. relate?
		sliceBuf.locs[i*3] = i%w;
		sliceBuf.locs[i*3+1] = (int) ((i - i%w) * invW) % h; // flip y-axis to match opengl texture origin (bottom left)		
		sliceBuf.locs[i*3+2] = (int) ((i - i%wh) * invWH) % 135; // % depth
	#else
		sliceBuf.colors[i] = img.pixelAt (i%w, (int) (h - ((i - i%w) * invW) - 1) % h);
		sliceBuf.locs[i*3] = i%w;
		sliceBuf.locs[i*3+1] = (int) ((i - i%w) * invW) % h; // flip y-axis to match opengl texture origin (bottom left)		
		sliceBuf.locs[i*3+2] = sliceIdx;
	#endif
	}
	z++;
	z %= 135; // 27*5 slices, 27 slices, each repeated 5 times
	
	// send slice to compositor
	recvSlice (sliceBuf);
	
	// glut timers do not repeat, so we create a recursive timer loop to emulate receiving data
	// sliceIdx++;
	// sliceIdx %= 135;
	glutTimerFunc (500, generateSlice, (sliceIdx+1) % 135); // 27 ultrasound images to read, each counts 5 times

#elseif defined(CUBE)
	// write black cube to slice
	// recvSlice maps the gray pbo to client memory, where the slice is written, then the pbo is copied to the volume texture
	int cubePix = 32;
	double invCP = 1.0 / cubePix;
	double invSqCP = 1.0 / (cubePix*cubePix);
	static slice sliceBuf;
		sliceBuf.count = cubePix * cubePix * cubePix;
	
	for (int i = 0; i < sliceBuf.count; i++) {
		// problem is here ???, possibly w/ fact that i=0 in .png space != 0 in pbo space
		sliceBuf.colors[i] = 0x00;
		sliceBuf.locs[i*3] = i%cubePix; // % w
		// sliceBuf.locs[i*3+1] = (i - i%cubePix) * invCP; // bad bad, no way to guarantee it wraps vertically
		sliceBuf.locs[i*3+1] = (int) ((i - i%cubePix) * invCP) % cubePix; // % h
		sliceBuf.locs[i*3+2] = (int) ((i - i%(cubePix*cubePix)) * invSqCP); // % d, not needed cause index never gets that high
	}
	recvSlice (sliceBuf);
#else // rect
	// write black rect to slice
	// recvSlice maps the gray pbo to client memory, where the slice is written, then the pbo is copied to the volume texture
		
	int w, h, d;
		w = 32;
		h = 64;
		d = 16;
	double invW = 1.0 / w;
	double invH = 1.0 / h;
			
	static slice sliceBuf;
		sliceBuf.count = w * h * d;
	
	for (int i = 0; i < sliceBuf.count; i++) {
		// problem is here ???, possibly w/ fact that i=0 in .png space != 0 in pbo space
		sliceBuf.colors[i] = 0x00;
		sliceBuf.locs[i*3] = i%w;
		sliceBuf.locs[i*3+1] = (int) ((i - i%w) * invW) % h;
		sliceBuf.locs[i*3+2] = (int) ((i - i%(w*h)) * invW * invH) % d;
	}
	recvSlice (sliceBuf);
#endif
}


static void vertex(float x, float y, float z)
{
	glColor3f(x,y,z);
	glVertex3f(x,y,z);
}

static void drawQuads(float x, float y, float z)
{
	glBegin(GL_QUADS);
	/* Back side */
//	glNormal3f(0.0, 0.0, -1.0);
	vertex(0.0, 0.0, 0.0);
	vertex(0.0, y, 0.0);
	vertex(x, y, 0.0);
	vertex(x, 0.0, 0.0);

	/* Front side */
//	glNormal3f(0.0, 0.0, 1.0);
	vertex(0.0, 0.0, z);
	vertex(x, 0.0, z);
	vertex(x, y, z);
	vertex(0.0, y, z);

	/* Top side */
//	glNormal3f(0.0, 1.0, 0.0);
	vertex(0.0, y, 0.0);
	vertex(0.0, y, z);
    vertex(x, y, z);
	vertex(x, y, 0.0);

	/* Bottom side */
//	glNormal3f(0.0, -1.0, 0.0);
	vertex(0.0, 0.0, 0.0);
	vertex(x, 0.0, 0.0);
	vertex(x, 0.0, z);
	vertex(0.0, 0.0, z);

	/* Left side */
//	glNormal3f(-1.0, 0.0, 0.0);
	vertex(0.0, 0.0, 0.0);
	vertex(0.0, 0.0, z);
	vertex(0.0, y, z);
	vertex(0.0, y, 0.0);

	/* Right side */
//	glNormal3f(1.0, 0.0, 0.0);
	vertex(x, 0.0, 0.0);
	vertex(x, y, 0.0);
	vertex(x, y, z);
	vertex(x, 0.0, z);
	glEnd();
	
}

static void draw_scene (void) 
{
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
    gluPerspective (60.0, 1.0, 1.0, 100.0);

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef (0.0f, 0.0f, -2.5f);
	glRotatef (env.y_angle, 0.0f, 1.0f, 0.0f);
	glTranslatef (-0.5f, -0.5f, -0.5f);

	glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, env.fbo);
	glUseProgram (0); //fixed functionality
	glEnable (GL_CULL_FACE);

//- Draw back face ---
		glDrawBuffer (GL_COLOR_ATTACHMENT0_EXT); //render to the 1st color attachment
			glClear (GL_COLOR_BUFFER_BIT);
			glCullFace (GL_FRONT);
			drawQuads (1.0f, 1.0f, 1.0f);

//- Draw front face ---
		glDrawBuffer (GL_COLOR_ATTACHMENT1_EXT); //render to the 2nd color attachment
			glClear (GL_COLOR_BUFFER_BIT);
			glCullFace (GL_BACK);
			drawQuads (1.0f, 1.0f, 1.0f);
			
	glDisable (GL_CULL_FACE);
	glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0); //revert back to standard framebuffer
	glDrawBuffer (GL_BACK); //revert back to drawing on the back buffer

//- Draw screen filling quad, textured w/ difference
	glUseProgram (env.program); //start GLSL
		glMatrixMode (GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D (-1.0f, 1.0f, -1.0f, 1.0f);
		
		glMatrixMode (GL_MODELVIEW);
		glClear (GL_COLOR_BUFFER_BIT);
		glLoadIdentity();

		glBegin (GL_QUADS);
/*			glMultiTexCoord2d (GL_TEXTURE0, 0.0f, 0.0f); glMultiTexCoord2d (GL_TEXTURE1, 0.0f, 0.0f); glVertex2f (-1.0f, -1.0f);
			glMultiTexCoord2d (GL_TEXTURE0, 0.0f, 1.0f); glMultiTexCoord2d (GL_TEXTURE1, 0.0f, 1.0f); glVertex2f (-1.0f, 1.0f);
			glMultiTexCoord2d (GL_TEXTURE0, 1.0f, 1.0f); glMultiTexCoord2d (GL_TEXTURE1, 1.0f, 1.0f); glVertex2f (1.0f, 1.0f);
			glMultiTexCoord2d (GL_TEXTURE0, 1.0f, 0.0f); glMultiTexCoord2d (GL_TEXTURE1, 1.0f, 0.0f); glVertex2f (1.0f, -1.0f);
*/
			glTexCoord2f (0.0f, 0.0f); glVertex2f (-1.0f, -1.0f);
			glTexCoord2f (0.0f, 1.0f); glVertex2f (-1.0f, 1.0f);
			glTexCoord2f (1.0f, 1.0f); glVertex2f (1.0f, 1.0f);
			glTexCoord2f (1.0f, 0.0f); glVertex2f (1.0f, -1.0f);

		glEnd();

		//printShaderInfoLog (f);
/*		static int shader_status = 0;
		glGetShaderiv (f, GL_COMPILE_STATUS, &shader_status);
		if (shader_status == GL_FALSE)
			cout << "Compile of fragment shader unsuccessful." << endl;
*/
	glUseProgram(0);
	
	glutSwapBuffers();
}

void keyboard (unsigned char key, int x, int y)
{
	if (key == 27) //esc
	{
		exit(0);
	}
	if (key == 13) //enter
	{ }
	if (key == 32) //space
	{ }

	glutPostRedisplay();
}

void specialKeys (int key, int x, int y)
{		
	switch (key)
	{
	case GLUT_KEY_RIGHT:
		// env.y_angle -= 10;
		env.y_angle -= 5;
		break;
	case GLUT_KEY_LEFT:
		// env.y_angle += 10;
		env.y_angle += 5;
		break;
	case GLUT_KEY_UP:
		env.y_cam += 2;
		break;
	case GLUT_KEY_DOWN:
		env.y_cam -= 2;
		break;
	default:
		break;	
	}

	glutPostRedisplay();
}

void onExit()
{
	cout << endl;
	cout << "Run complete." << endl;
}

int main ( int argc, char * argv[] )
{	
	glutInit(&argc, argv);
	glutInitDisplayMode (GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize (env.width, env.height);
	glutInitWindowPosition (0, 0);
			
	env.window_handle = glutCreateWindow ("GPU volume rendering w/ GLSL");

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		cerr << "Glew not initialized." << endl;
		exit(1);
	}

	glutDisplayFunc (draw_scene);
	glutKeyboardFunc (keyboard);
	glutSpecialFunc (specialKeys);
	glutTimerFunc (500, generateSlice, 0);
	
	init();
	
	atexit (onExit);

	glutMainLoop();
	
	return 0;
}
