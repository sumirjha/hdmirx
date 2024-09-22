#include "common.h"


const char* string_egl_error(EGLint error)
{
	static const char* errors[] =
	{
		"EGL_SUCCESS",
		"EGL_NOT_INITIALIZED",
		"EGL_BAD_ACCESS",
		"EGL_BAD_ALLOC",
		"EGL_BAD_ATTRIBUTE",
		"EGL_BAD_CONFIG",
		"EGL_BAD_CONTEXT",
		"EGL_BAD_CURRENT_SURFACE",
		"EGL_BAD_DISPLAY",
		"EGL_BAD_NATIVE_PIXMAP",
		"EGL_BAD_NATIVE_WINDOW",
		"EGL_BAD_PARAMETER",
		"EGL_BAD_SURFACE",
		"EGL_CONTEXT_LOST"
	};
	static const char unknown[] = "UNKNOWN";

	if (error >= EGL_SUCCESS && error <= EGL_CONTEXT_LOST)
	{
		return errors[error - EGL_SUCCESS];
	}
	return unknown;
}

const char* string_gl_error(GLenum error)
{
	static const char no_error[] = "GL_NO_ERROR";
	static const char* errors[] =
	{

		"GL_INVALID_ENUM",
		"GL_INVALID_VALUE",
		"GL_INVALID_OPERATION",
		"GL_STACK_OVERFLOW",
		"GL_STACK_UNDERFLOW",
		"GL_OUT_OF_MEMORY",
	};
	static const char unknown[] = "UNKNOWN";

	if (error == GL_NO_ERROR)
	{
		return no_error;
	}
	if (error >= GL_INVALID_ENUM && error <= GL_OUT_OF_MEMORY)
	{
		return errors[error - GL_INVALID_ENUM];
	}
	return unknown;
}

GLuint gles_load_shader(GLenum shader_type, const char *code)
{
	GLuint shader = 0;
	GLint status;

	shader = glCreateShader(shader_type);
	if (!shader)
	{
		printf("Unable to create shader %s", string_gl_error(glGetError()));
		return 0;
	}
	glShaderSource(shader, 1, &code, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status)
	{
		GLint length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		if (length)
		{
			char* error = malloc(length);
			if (error)
			{
				glGetShaderInfoLog(shader, length, NULL, error);
				printf("Unable to compile shader: '%s'", error);
			}

			free(error);
		}
		else
		{
			printf("Unable to compile shader, unknown error");
		}
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

GLuint gles_load_program(const char *vertex_code, const char *fragment_code)
{
	GLuint vertex_shader;
	GLuint fragment_shader;
	GLuint program;
	GLint status;
	vertex_shader = gles_load_shader(GL_VERTEX_SHADER, vertex_code);
	fragment_shader = gles_load_shader(GL_FRAGMENT_SHADER, fragment_code);
	if (!vertex_shader || !fragment_shader)
	{
		printf("Unable to create shader");
		return 0;
	}

	program = glCreateProgram();
	if (!program)
	{
		printf("Unable to create opengl program");
		return 0;
	}
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE)
	{
		printf("Unable to link program");
		glDeleteProgram(program);
		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);
		return 0;
	}
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	return program;
}


void* gles_load_extension(const char* extension, const char* procedure_name)
{
	const char* ext_list;
	const char* available;
	void *address = 0;

	ext_list = (char*)glGetString(GL_EXTENSIONS);
	printf("Available Extensions GL %s", ext_list);
	available = strstr(ext_list, extension);

	if (available)
	{
		address = eglGetProcAddress(procedure_name);
	}
	return address;
}

void *egl_load_extension(EGLDisplay display, const char* extension, const char* procedure_name)
{
	const char* ext_list;
	const char* available;
	void *address = 0;

	ext_list = eglQueryString(display, EGL_EXTENSIONS);
	printf("Available Extensions EGL %s", ext_list);

	available = strstr(ext_list, extension);

	if (available)
	{
		address = eglGetProcAddress(procedure_name);
	}
	return address;
}