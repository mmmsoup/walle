#ifndef WINDOW_H
#define WINDOW_H

#include <GL/gl3w.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include "config.h"
#include "gl.h"
#include "log.h"
#include "util.h"

// https://github.com/baskerville/bspwm/issues/1382
#include "bspwm.h"

#define MIN(a,b) (a < b ? a : b)
#define MAX(a,b) (a > b ? a : b)

extern Atom ATOM_WM_PROTOCOLS;
extern Atom ATOM_WM_DELETE_WINDOW;

extern Atom ATOM_NET_WM_STRUT;
extern Atom ATOM_NET_WM_STRUT_PARTIAL;

extern Atom ATOM_WALLPAPER_PATH;
extern Atom ATOM_WALLPAPER_TRANSITION_DURATION;

extern Atom ATOM_CURRENT_WORKSPACE;
extern Atom ATOM_WINDOW_VISIBLE;

typedef struct {
	char *bgimg;
	unsigned short struts[4]; // left right top bottom
} startup_properties_t;

int props_init(Display *);

Window get_program_window(Display *);

int set_net_wm_strut(Display *, Window, short, short, short, short);
int set_net_wm_strut_partial(Display *, Window, short, short, short, short);

int window_run(Display *, startup_properties_t, int fd);

/* keeping track of windows obscuring wallpaper window */
#define WINDOW_LIST_BLOCK_SIZE 64
typedef struct {
	Window *windows;
	size_t maxsize;
	size_t length;
} window_list_t;
int window_list_new(window_list_t*);
int window_list_resize(window_list_t*, size_t);
int window_list_add(window_list_t*, Window);
int window_list_remove(window_list_t*, Window);
int window_list_delete(window_list_t*);

#endif
