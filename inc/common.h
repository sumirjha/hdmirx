#ifndef __CAP_COMMON_H__
#define __CAP_COMMON_H__

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <linux/videodev2.h>
#include <libdrm/drm.h>
#include <libdrm/drm_mode.h>

#include <stdbool.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <assert.h>
#include <libdrm/drm_fourcc.h>

/*
 * ---------------------------------------------------------------------------
 * Capture Defines
 * ---------------------------------------------------------------------------
 */

#define Capture_status_codes(X)                                 \
    X( 0,  CSTATUS_SUCCESS,             "No error")             \
    X(-1,  CSTATUS_FAIL,                "Undefined error")      \
    X(-2,  CSTATUS_BAD_PARAM,           "Bad parameter")        \
    X(-3,  CSTATUS_MEMORY,              "Memory error")         \
    X(-4,  CSTATUS_SYSCALL,             "System call error")    \
    X(-5,  CSTATUS_CONTEXT,             "Context error")        \
    X(-6,  CSTATUS_PKT_LOSS,            "Packet loss")          \
    X(-7,  CSTATUS_AGAIN,               "No data available")

#define Capture_status_codes_enum(ID, NAME, TEXT) NAME = ID,
#define Capture_status_codes_text(ID, NAME, TEXT) case ID: return TEXT;


/*
* ---------------------------------------------------------------------------
* Capture code data types.
* ---------------------------------------------------------------------------
*/

/** @brief Capture status codes. */
typedef enum
{
    Capture_status_codes(Capture_status_codes_enum)
} CStatus_t;


/*
 * ---------------------------------------------------------------------------
 * Capture rc enum to string conversion.
 * ---------------------------------------------------------------------------
 */

/**
 * @brief Convert a Capture status code into a string.
 *
 * @param [in]  rc  A Capture status code.
 *
 * @return Capture status code string.
 */
static inline const char *CStatus_string(CStatus_t rc)
{
     switch (rc)
     {
         Capture_status_codes(Capture_status_codes_text)
     }
     return "*** unknown error ***";
 }


/*
 * ---------------------------------------------------------------------------
 * Global defines
 * ---------------------------------------------------------------------------
 */

#define V4L2_DEVICE 	"/dev/video0"
#define DRM_DEVICE		"/dev/dri/card0"

#define DMA_BUFF_COUNT	3
#define NUM_PLANES		1

#define IMG_WIDTH           1920
#define IMG_HEIGHT           1080

#define ERRSTR strerror(errno)

#define OKAY_RETURN(cond, ret, ...) \
if (cond) { \
	int errsv = errno; \
	fprintf(stderr, "ERROR(%s:%d) : ", \
		__FILE__, __LINE__); \
	errno = errsv; \
	fprintf(stderr,  __VA_ARGS__); \
	return ret; \
}

#define OKAY_STOP(cond, ...) \
if (cond) { \
	int errsv = errno; \
	fprintf(stderr, "ERROR(%s:%d) : ", \
		__FILE__, __LINE__); \
	errno = errsv; \
	fprintf(stderr,  __VA_ARGS__); \
	break; \
}

#define TS_PACKET_SIZE 	188
#define TS_TOTAL_PACKET	2048
#define NANO_PER_SEC 1000000000.0

#define UNUSED_PARAMETER(x) (void)x



const char* string_egl_error(EGLint error);
const char* string_gl_error(GLenum error);
GLuint gles_load_shader(GLenum shader_type, const char *code);
GLuint gles_load_program(const char *vertex_code, const char *fragment_code);

void *gles_load_extension(const char* extension, const char* procedure_name);
void *egl_load_extension(EGLDisplay display, const char* extension, const char* procedure_name);

#endif