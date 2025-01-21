#include "common.h"
#include "display.h"
#include "buffer.h"

/**
 * Vertex shader, first stage produce texture and render positions.
 * Shaders included in code for simplicity but could be external files.
 */
static const char passthrough_vertex_code[] =
	"#version 300 es\n"
	"//VERTEX_SHADER\n"
	"layout(location = 0) in vec4 a_position;\n"
	"layout(location = 1) in vec2 a_tex_coord;\n"
	"out vec2 v_tex_coord;\n"
	"void main()\n"
	"{\n"
	"   gl_Position = a_position;\n"
	"   v_tex_coord = a_tex_coord;\n"
	"}\n";
/**
 * fragment shader, second stage draw camera texture on each point in the window.
 * Shaders included in code for simplicity but could be external files.
 */
static const char nv24_fragment_code[] =
	"#version 300 es\n"
	"precision mediump float;\n"
	
	"in vec2 v_tex_coord;\n"
	"layout(location = 0) out vec4 out_color;\n"
	"const vec3 offset = vec3(0, -0.5, -0.5);\n"
	"\n"
	"const mat3 coeff = mat3(\n"
	"    1.0,      1.0,     1.0,\n"
	"    0.0,     -0.344,  1.722,\n"
	"    1.402,   -0.714,   0);\n"

	"vec3 yuv;\n"
	"vec3 rgb;\n"
	"uniform sampler2D s_luma_texture;\n"
	"uniform sampler2D s_chroma_texture;\n"
	"void main()\n"
	"{\n"
	"    // Lookup the texture at the  position in the render window.\n"
	"    // Take the two semi-planar textures assemble into single YUV pixel.\n"
	"    yuv.x = texture2D(s_luma_texture, v_tex_coord).x;\n"
	"    yuv.yz = texture2D(s_chroma_texture, v_tex_coord).xy;\n"
	"    // Then convert to YUV to RGB using offset and gain.\n"
	"    yuv += offset;\n"
	"    rgb = coeff * yuv;\n"
	"    out_color = vec4(rgb,1);\n"
	"}\n";


static CStatus_t displaySetupTexture(Display_t *disp)
{
    int ret;
	GLenum error = 0;
	/**
	 * The triangle vetices for the render target and the texture co-ordinates are interleaved.
	 * The triangle co-ordinates are between -1.0 and 1.0 with (0.0, 0.0, 0.0) as the origin.
	 * A flat rectangle the size of the whole window will be drawn so the third dimension is not required.
	 * Two triangles are drawn with the indices shown below on the left and co-ordinates on the right.
	 * @verbatim
	   0 + -- + 3       (-1.0,  1.0, 0.0) + -- + (1.0,  1.0, 0.0)
	     | \  |                           | \  |
	     |  \ |                           |  \ |
	   1 + -- + 2       (-1.0, -1.0, 0.0) + -- + (1.0, -1.0, 0.0)
	  @endverbatim
	 *
	 * The texture co-ordinates use a the lower left as origin and go between 0.0 and 1.0.
	 * The texture co-ordinate boundaries are interleaved with the triangle data for compactness in GPU memory.
	 *
	 */
	GLfloat vertices[] = {
		-1.0f, 1.0f, 0.0f,  /* Triangle 0/1, index 0, upper left */
		0.0f, 0.0f,         /* Texture lower left */
		-1.0f, -1.0f, 0.0f, /* Triangle 0, index 1, lower left */
		0.0f, 1.0f,         /* Texture upper left */
		1.0f, -1.0f, 0.0f,  /* Triangle 0/1, index 2, lower right */
		1.0f, 1.0f,         /* Texture upper right */
		1.0f, 1.0f, 0.0f,   /* Triangle 1, index 3, upper right */
		1.0f, 0.0f          /* Texture lower right */
	};
	GLushort indices[] = {0, 1, 2, 0, 2, 3};


	disp->frameData = calloc(1, disp->width * disp->height * 4);

	
	glGenVertexArrays(1, &disp->vertexArray);
	/* Select the vertex array that was just created and bind the new vertex buffers to the array */
	glBindVertexArray(disp->vertexArray);


	/*
	 * Generate a vertex buffers in GPU memory
	 * The first buffer is for the vertex data and the second buffer is for the index data.
	 */
	glGenBuffers(2, disp->vertexBuffers);

	/* Bind the first vertex buffer with the vertices to the vertex array */
	glBindBuffer(GL_ARRAY_BUFFER, disp->vertexBuffers[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	/* Bind the second vertex buffer with the indices to the vertex array */
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, disp->vertexBuffers[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	/* Enable a_position (location 0) in the shader */
	glEnableVertexAttribArray(0);
	/* Enable a_tex_coord (location 1) in the shader */
	glEnableVertexAttribArray(1);

	/* Copy the vertices to the GPU to be used in a_position */
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);
    error = glGetError();
	OKAY_RETURN(error != GL_NO_ERROR, CSTATUS_FAIL, "Use glVertexAttribPointer 0 %s\n", string_gl_error(error));

	/* Copy the texture co-ordinates to the GPU to be used in a_tex_coord */
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (const void*)(3 * sizeof(GLfloat)));
	error = glGetError();
    OKAY_RETURN(error != GL_NO_ERROR, CSTATUS_FAIL,"Use glVertexAttribPointer 1 %s\n", string_gl_error(error));

	/* Select the default vertex array, allowing the application's array to be unbound */
	glBindVertexArray(0);

	/* Generate two textures, the first for luma data, the second for chroma data */
	glGenTextures(2, disp->texture);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, disp->texture[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, disp->width, disp->height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	error = glGetError();
	OKAY_RETURN(error != GL_NO_ERROR, CSTATUS_FAIL,"Unable to generate texture %s\n", string_gl_error(error));


	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, disp->texture[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, disp->width, disp->height, 0, GL_RG, GL_UNSIGNED_BYTE, NULL);
	error = glGetError();
	OKAY_RETURN(error != GL_NO_ERROR, CSTATUS_FAIL,"Unable to generate texture %s\n", string_gl_error(error));
	

	//Make shader program
	disp->program = gles_load_program(passthrough_vertex_code, nv24_fragment_code);
	OKAY_RETURN(!disp->program, CSTATUS_FAIL, "Unable to load program\n");

	disp->location[0] = glGetUniformLocation(disp->program, "s_luma_texture");
	OKAY_RETURN(disp->location[0] == -1, CSTATUS_FAIL, "Unable to get location program %s\n", string_gl_error(glGetError()));

	disp->location[1] = glGetUniformLocation(disp->program, "s_chroma_texture");
	OKAY_RETURN(disp->location[1] == -1, CSTATUS_FAIL, "Unable to get location program %s\n", string_gl_error(glGetError()));
	glClearColor ( 1.0f, 0.6f, 0.0f, 0.0f );

	glUseProgram(disp->program);
	error = glGetError();
    OKAY_RETURN(error != GL_NO_ERROR, CSTATUS_FAIL,"Use program %s\n", string_gl_error(error));

	//Setup render
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, disp->texture[0]);
	glUniform1i(disp->location[0], 0);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,disp->width, disp->height, GL_RED, GL_UNSIGNED_BYTE, disp->frameData);
	error = glGetError();
	OKAY_RETURN(error != GL_NO_ERROR, CSTATUS_FAIL,"Unable to generate texture %s\n", string_gl_error(error));

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, disp->texture[1]);
	glUniform1i(disp->location[1], 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,disp->width, disp->height, GL_RG, GL_UNSIGNED_BYTE, disp->frameData + (disp->width, disp->height));
	error = glGetError();
	OKAY_RETURN(error != GL_NO_ERROR, CSTATUS_FAIL,"Unable to generate texture %s\n", string_gl_error(error));
}

Display_t *displayCreate(int width, int height)
{

    Display_t *d = calloc(1, sizeof(Display_t));
    OKAY_RETURN(NULL == d, NULL, "failed to allocated display object\n");

    do
    {
        d->width = width;
        d->height = height;

        //X11 Open and Initialise
        d->xdpy = XOpenDisplay(0);
        OKAY_STOP(NULL == d->xdpy, "failed to open display.\n");

        d->xscr = DefaultScreen(d->xdpy);
        d->xroot = RootWindow(d->xdpy, d->xscr);

        d->xa_wm_proto = XInternAtom(d->xdpy, "WM_PROTOCOLS", False);
        d->xa_wm_del_win = XInternAtom(d->xdpy, "WM_DELETE_WINDOW", False);


        //Egl Initialise and Open
        d->eglDpy = eglGetPlatformDisplay(EGL_PLATFORM_X11_EXT, (void *)d->xdpy, NULL);
        OKAY_STOP(NULL == d->eglDpy, "failed to open egl platform display.\n");

        bool eglRet = eglInitialize(d->eglDpy, NULL, NULL);
        OKAY_STOP(!eglRet, "failed to initialise egl.\n");

        //EGL Configure
        EGLint attr_list[] = {
            EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_STENCIL_SIZE, 0,
            EGL_DEPTH_SIZE, 0,
            EGL_NONE
        };

        EGLConfig config;
        EGLint num_configs;
        eglRet = eglChooseConfig(d->eglDpy, attr_list, &config, 1, &num_configs);
        OKAY_STOP(eglRet == false,  "Failed to find a suitable EGL config.\n");

        d->eglConfig = config;

        EGLint visId;
        eglGetConfigAttrib(d->eglDpy, d->eglConfig, EGL_NATIVE_VISUAL_ID, &visId);

        //Create X11 Window
        XVisualInfo visInfoMatch = {0};
        visInfoMatch.visualid = visId;

        XVisualInfo *visInfo;
        int numVisuals;

        visInfo = XGetVisualInfo(d->xdpy, VisualIDMask, &visInfoMatch, &numVisuals);
        OKAY_STOP(NULL == visInfo,  "Failed to get visual info (X11).\n");

        XSetWindowAttributes xattr;
        xattr.background_pixel = WhitePixel(d->xdpy, d->xscr);
        xattr.colormap = XCreateColormap(d->xdpy, d->xroot, visInfo->visual, AllocNone);

        // create an X window
        d->win = XCreateWindow(d->xdpy, d->xroot,
                                0, 0, width, height,
                                0, visInfo->depth,
                                InputOutput, visInfo->visual,
                                CWBackPixel | CWColormap, &xattr);

        // X events we receive
        XSelectInput(d->xdpy, d->win,
                    ExposureMask | StructureNotifyMask | KeyPressMask);

        // Window title
        XTextProperty texProp;
        const char *title = "HDMI Viewer";
        XStringListToTextProperty((char**)&title, 1, &texProp);
        XSetWMName(d->xdpy, d->win, &texProp);
        XFree(texProp.value);

        // Window manager protocols
        XSetWMProtocols(d->xdpy, d->win, &d->xa_wm_del_win, 1);
        XMapWindow(d->xdpy, d->win);
        XSync(d->xdpy, 0);

        EGLint ctxAtts[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };

        d->eglCtx = eglCreateContext(d->eglDpy, d->eglConfig, EGL_NO_CONTEXT, ctxAtts);
        OKAY_STOP(NULL == d->eglCtx, "Failed to create EGL context.\n");

        d->eglSurf = eglCreateWindowSurface(d->eglDpy, d->eglConfig, d->win, 0);
        OKAY_STOP(d->eglSurf == EGL_NO_SURFACE, "failed to return egl surface\n");

        OKAY_STOP(eglGetError() != EGL_SUCCESS, "egl error after context creation.\n");

        eglMakeCurrent(d->eglDpy, d->eglSurf, d->eglSurf, d->eglCtx);

        displaySetupTexture(d);

        // EGLint formats[100] = {0};
        // EGLint formatCount = 0;
        // PFNEGLQUERYDMABUFFORMATSEXTPROC eglQueryDmaBufFormatsEXT  = (PFNEGLQUERYDMABUFFORMATSEXTPROC)eglGetProcAddress(
        //     "eglQueryDmaBufFormatsEXT");
        // if(eglQueryDmaBufFormatsEXT)
        // {
        //     EGLBoolean res = eglQueryDmaBufFormatsEXT(d->eglDpy, (EGLint)100, formats, &formatCount);
        //     if(res)
        //     {
        //         printf("Format Supported\n");
        //         for(int i = 0; i< formatCount; i++)
        //         {
        //             int format = (int)formats[i];
        //             printf("%c%c%c%c %s-endian (0x%08x)\n",
        //                 (format & 0xff),
        //                 ((format >> 8) & 0xff),
        //                 ((format >> 16) & 0xff),
        //                 ((format >> 24) & 0x7f),
        //                 format & DRM_FORMAT_BIG_ENDIAN ? "big" : "little",
        //                 format);
        //         }
        //     }
        // }
    } while (0);

    return d;
}

CStatus_t displayInitBuffer(Display_t *d, Buffer_t *b)
{
    // EGL (extension: EGL_EXT_image_dma_buf_import): Create EGL image from file descriptor (texture_dmabuf_fd) and storage
	// data (texture_storage_metadata)
	const EGLint attributeList[] = {
		EGL_LINUX_DRM_FOURCC_EXT, DRM_FORMAT_NV12,
		EGL_WIDTH, d->width,
		EGL_HEIGHT, d->height,

		EGL_DMA_BUF_PLANE0_FD_EXT, b->dmafd[0],
		EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
		EGL_DMA_BUF_PLANE0_PITCH_EXT, d->width,

		EGL_DMA_BUF_PLANE1_FD_EXT, b->dmafd[0],
		EGL_DMA_BUF_PLANE1_OFFSET_EXT, d->width * d->height,
		EGL_DMA_BUF_PLANE1_PITCH_EXT, d->width,
	
		// EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, (uint32_t)(texture_storage_metadata.modifiers & ((((uint64_t)1) << 33) - 1)),
		// EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, (uint32_t)((texture_storage_metadata.modifiers>>32) & ((((uint64_t)1) << 33) - 1)),
		EGL_NONE};

	PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)(eglGetProcAddress("eglCreateImageKHR"));
	b->image = eglCreateImageKHR(d->eglDpy,
									EGL_NO_CONTEXT,
									EGL_LINUX_DMA_BUF_EXT,
									(EGLClientBuffer)NULL,
									attributeList);
	GLint glError =  eglGetError();
	OKAY_RETURN(glError != EGL_SUCCESS || b->image == NULL, CSTATUS_FAIL, "egl image failed to create.\n");

	// // GLES (extension: GL_OES_EGL_image_external): Create GL texture from EGL image
	glBindTexture(GL_TEXTURE_2D,0);
	glGenTextures(1, &b->texture);
	glError =  eglGetError();

	glBindTexture(GL_TEXTURE_2D, b->texture);
	glError =  eglGetError();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, b->image);
	glError =  eglGetError();

	return CSTATUS_SUCCESS;
}


CStatus_t displayDraw(Display_t *disp, Buffer_t *b)
{
    int ret = 0;
    GLenum error = 0;

	//memcpy(disp->frameData, b->ptr[0], b->size[0]);

    glViewport(0, 0, disp->width, disp->height);
	/*
	 * Only the color buffer is used. (The depth, and stencil buffers are unused.)
	 * Set it to the background color before rendering.
	 */
	glClear(GL_COLOR_BUFFER_BIT);
	/** Select the NV12 Shader program compiled in the setup routine */

	// glUseProgram(disp->program);
	// error = glGetError();
    // OKAY_RETURN(error != GL_NO_ERROR, CSTATUS_FAIL,"Use program %s\n", string_gl_error(error));


	/* Select the vertex array which includes the vertices and indices describing the window rectangle. */
	glBindVertexArray(disp->vertexArray);
	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, disp->vertexBuffers[1]);


	/*
	 * Copy the Luma data from plane 0 to the s_luma_texture texture in the GPU.
	 * Each texture has four components, x,y,z,w alias r,g,b,a alias s,r,t,u.
	 * Luma will be replicated in x, y, and z using type GL_LUMINCANCE. Component w is set to 1.0.
	 * The resolution of the texture matches the number of active pixels.
	 */
	glActiveTexture(GL_TEXTURE0);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, disp->width, disp->height, GL_RED, GL_UNSIGNED_BYTE, b->ptr[0]);
	error = glGetError();
	//glBindTexture(GL_TEXTURE_2D, disp->texture[0]);
	/* Indicate that GL_TEXTURE0 is s_luma_texture from previous lookup */
	//glUniform1i(disp->location[0], 0);

	/*
	 * Copy the chroma data from plane 1 to the s_chroma_texture texture in the GPU.
	 * Each texture has four components, x,y,z,w alias r,g,b,a alias s,r,t,u.
	 * Cb is will be replicated in x, y, and z. Cr will be assigned to w. with type GL_LUMINANCE_ALPHA.
	 * There is one pair of chroma values for every four luma pixels.
	 * Half the vertical resoulution and half the horizontal resolution.
	 * Each GL_LUMINCANCE_ALPHA lookup will contain one pixel of chroma.
	 * The texture lookup will replicate the chroma values up to the total resolution.
	 */
	glActiveTexture(GL_TEXTURE1);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
		0, 0, disp->width, disp->height,
		GL_RG, GL_UNSIGNED_BYTE,  b->ptr[0] + (disp->width * disp->height));
	error = glGetError();
	//glBindTexture(GL_TEXTURE_2D, disp->texture[1]);
	/* Indicate that GL_TEXTURE1 is s_chroma_texture from previous lookup */
	//glUniform1i(disp->location[1], 1);

	/*
	 * Draw the two triangles from 6 indices to form a rectangle from the data in the vertex array.
	 * The fourth parameter, indices value here is passed as null since the values are already
	 * available in the GPU memory through the vertex array
	 * GL_TRIANGLES - draw each set of three vertices as an individual trianvle.
	 */
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
	/** Select the default vertex array, allowing the applications array to be unbound */
	glBindVertexArray(0);

	/*
	 * display the new camera frame after render is complete at the next vertical sync
	 * This is drawn on the EGL surface which matches the full screen native window.
	 */
	ret = eglSwapBuffers(disp->eglDpy, disp->eglSurf);

    return CSTATUS_SUCCESS;
}


void displayDestroy(Display_t *display)
{

}