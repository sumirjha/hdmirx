#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "common.h"

typedef struct {
	int					index;
	int             	dmafd[NUM_PLANES];
	char				*ptr[NUM_PLANES];
	int					offset[NUM_PLANES];
	int					size[NUM_PLANES];
	int					len[NUM_PLANES];

	//OpenGL
	GLuint 				texture;
	EGLImage 			image;
}Buffer_t;



#endif