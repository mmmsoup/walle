#ifndef GL_H
#define GL_H

#include <GL/gl3w.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include "config.h"
#include "log.h"
#include "util.h"

#include SHADER_HEADER

#define CLOCKS_PER_MSEC (CLOCKS_PER_SEC / 1000)

typedef struct {
	GLuint id;
	int width, height;
} gl_texture_t;

typedef struct {
	int active;
	double finish, duration;
} gl_texture_transition_t;

typedef struct {
	Display *display;
	Window window;
	unsigned int window_width, window_height;

	GLuint shader_program;
	GLuint vertex_array_object;

	gl_texture_t textures[2];
	gl_texture_transition_t texture_transition;
	int current_texture;
	GLfloat mixing;
} gl_data_t;

int gl_set_texture_multiplier(gl_data_t *, int);

int gl_load_texture(gl_data_t *, int, char *);

int gl_load_shaders(GLuint*, char*, int, char*, int);

int gl_init(gl_data_t *, Display *, XVisualInfo *, Window);

int gl_show_texture(gl_data_t *, int, double);

int gl_resize(gl_data_t *);

int gl_render(gl_data_t *);

// check whether `filename' refers to a file that stbi can load (using internal functions to test to avoid potentially parsing the full file as that is done later by the server)
int stbi_valid(char *);

#endif
