#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include "common.h"
#include "buffer.h"


#define MAX_DISPLAY_OBJECTS 2

typedef struct
{
        //Meta
        int                     width;
        int                     height;

        //Egl
		EGLDisplay 				eglDpy;
		EGLSurface 				eglSurf;
		EGLContext 				eglCtx;
    	EGLConfig 				eglConfig;

		//X11
		Display 				*xdpy;
		Window 					xroot;
		int 					xscr;
		Window	 				win;
		Atom 					xa_wm_proto;
		Atom 					xa_wm_del_win;

        //Testture
        /** Handle to the vertex array, or collection of indices and vertices. */
        GLuint vertexArray;

        /** List of handles to vertex buffers, vertices and indices stored in GPU memory. */
        GLuint vertexBuffers[MAX_DISPLAY_OBJECTS];

        /** List of texture handles stored in GPU memory, video frames will be copied here. */
        GLuint texture[MAX_DISPLAY_OBJECTS];

        /** List of uniform reference locations for passing information to the shader program */
        GLint location[MAX_DISPLAY_OBJECTS];

        /** The handle to the compiled shader program */
        GLuint program;

		unsigned char *					frameData;
}Display_t;





Display_t *displayCreate(int width, int height);
CStatus_t displayInitBuffer(Display_t *d, Buffer_t *b);
CStatus_t displayDraw(Display_t *d, Buffer_t *b);

#endif