#ifndef WINDOW_H
#define WINDOW_H

#include <GL/gl3w.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include "config.h"
#include "gl.h"
#include "log.h"

// https://github.com/baskerville/bspwm/issues/1382
#include "bspwm.h"

extern Atom ATOM_WM_PROTOCOLS;
extern Atom ATOM_WM_DELETE_WINDOW;

extern Atom ATOM_NET_WM_STRUT;
extern Atom ATOM_NET_WM_STRUT_PARTIAL;

extern Atom ATOM_WALLPAPER_PATH;
extern Atom ATOM_WALLPAPER_TRANSITION_DURATION;

int common_init(Display *);

Window get_program_window(Display *);

int set_net_wm_strut(Display *, Window, short, short, short, short);
int set_net_wm_strut_partial(Display *, Window, short, short, short, short);

int window_run(Display *, int fd);

#endif
