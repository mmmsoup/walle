#ifndef GL_H
#define GL_H

#include <stdlib.h>
#include <GL/gl3w.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include "log.h"

typedef struct {
	Display *display;
	Window window;
	GLuint shader_program;
	GLuint vertex_array_object;
	GLuint textures[2];
	GLfloat mixing;
} gl_data_t;

int gl_load_texture(gl_data_t *gl_data, int index, char *image_path);

int gl_init(gl_data_t *, Display *, XVisualInfo *, Window);

int gl_resize(gl_data_t *);

int gl_render(gl_data_t *);

#endif
