#include "window.h"

Atom ATOM_NET_WM_STRUT;
Atom ATOM_NET_WM_STRUT_PARTIAL;
Atom ATOM_WALLPAPER_PATH;
Atom ATOM_WM_DELETE_WINDOW;
Atom ATOM_WM_PROTOCOLS;

int common_init(Display *display) {
	ATOM_NET_WM_STRUT = XInternAtom(display, "_NET_WM_STRUT", 0);
	ATOM_NET_WM_STRUT_PARTIAL = XInternAtom(display, "_NET_WM_STRUT_PARTIAL", 0);

	ATOM_WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", 0);
	ATOM_WM_PROTOCOLS = XInternAtom(display, "WM_PROTOCOLS", 0);

	ATOM_WALLPAPER_PATH = XInternAtom(display, WALLPAPER_PATH_ATOM_NAME, 0);

	bspwm_init();

	return EXIT_SUCCESS;
}

Window get_program_window(Display *display) {
	Window window = 0x0;

	Window root, parent, *children;
	unsigned int nchildren;
	if (XQueryTree(display, DefaultRootWindow(display), &root, &parent, &children, &nchildren) == 0) {
		ERR("XQueryTree(): failed");
		XCloseDisplay(display);
		return window;
	}

	Atom ATOM_WM_CLASS = XInternAtom(display, "WM_CLASS", 0);
	Atom type;
	int format;
	unsigned long nitems, bytes_after;
	unsigned char *prop;
	for (int i = 0; i < nchildren; i++) {
		XGetWindowProperty(display, *(children+i), ATOM_WM_CLASS, 0L, strlen(WINDOW_CLASS)/4 + 1, 0, XA_STRING, &type, &format, &nitems, &bytes_after, &prop);
		if (prop != NULL && strcmp((char *)prop, WINDOW_CLASS) == 0) {
			window = *(children+i);
			XFree(prop);
			break;
		}
		XFree(prop);
	}

	XFree(children);

	return window;
}

int set_net_wm_strut(Display *display, Window window, short left, short right, short top, short bottom) {
	Atom type;
	int format;
	unsigned long nitems, bytes_after;
	short *old_struts;
	XGetWindowProperty(display, window, ATOM_NET_WM_STRUT, 0L, 4L, 0, XA_CARDINAL, &type, &format, &nitems, &bytes_after, (unsigned char **)&old_struts);
	short new_struts[4] = { left, right, top, bottom };
	if (nitems == 0) {
		new_struts[0] = left == -1 ? 0 : left;
		new_struts[1] = right == -1 ? 0 : right;
		new_struts[2] = top == -1 ? 0 : top;
		new_struts[3] = bottom == -1 ? 0 : bottom;
	} else if (nitems == 4) {
		new_struts[0] = left == -1 ? old_struts[0] : left;
		new_struts[1] = right == -1 ? old_struts[1] : right;
		new_struts[2] = top == -1 ? old_struts[2] : top;
		new_struts[3] = bottom == -1 ? old_struts[0] : bottom;
	}
	XFree(old_struts);
	XChangeProperty(display, window, ATOM_NET_WM_STRUT, XA_CARDINAL, 16, PropModeReplace, (unsigned char *)new_struts, 4);
	XFlush(display);
	return EXIT_SUCCESS;
}

int set_net_wm_strut_partial(Display *display, Window window, short left, short right, short top, short bottom) {
	if (WM_IS_BSPWM) bspwm_set_struts(left, right, top, bottom);
	else {
		short struts[] = { left, right, top, bottom, 0, 0, 0, 0, 0, 0, 0, 0};
		XChangeProperty(display, window, ATOM_NET_WM_STRUT_PARTIAL, XA_CARDINAL, 16, PropModeReplace, (unsigned char *)struts, 12);
		XFlush(display);
	}
	DEBUG("struts set to %i (left), %i (right), %i (top), %i (bottom)", left, right, top, bottom);
	int screen_no = XDefaultScreen(display);
	int width = DisplayWidth(display, screen_no) - left - right;
	int height = DisplayHeight(display, screen_no) - top - bottom;
	XMoveResizeWindow(display, window, left, top, width, height);
	XFlush(display);
	DEBUG("window moved to (%i, %i) and resized to %ix%ipx", left, top, width, height);
	return EXIT_SUCCESS;
}

int window_run(Display *display) {
	Screen *screen = DefaultScreenOfDisplay(display);
	int screen_width = WidthOfScreen(screen);
	int screen_height = HeightOfScreen(screen);

	Window root = DefaultRootWindow(display);

	GLint visual_attrs[] = {
		GLX_RGBA,
		GLX_DEPTH_SIZE, 24,
		GLX_DOUBLEBUFFER,
		None
	};
	XVisualInfo *visual_info = glXChooseVisual(display, 0, visual_attrs);
	if (visual_info == NULL) {
		ERR("glXChooseVisual(): no appropriate visual found");
		return EXIT_FAILURE;
	}

	Colormap colourmap = XCreateColormap(display, root, visual_info->visual, AllocNone);
	XSetWindowAttributes window_attrs = {
		.colormap = colourmap,
		.event_mask = ExposureMask | PropertyChangeMask
	};
    Window window = XCreateWindow(display, root, 0, 0, screen_width, screen_height, 0, visual_info->depth, InputOutput, visual_info->visual, CWColormap | CWEventMask, &window_attrs);

	XStoreName(display, window, WINDOW_NAME);
	XClassHint *class_hint = XAllocClassHint();
	class_hint->res_name = WINDOW_CLASS;
	class_hint->res_class = WINDOW_CLASS;
	XSetClassHint(display, window, class_hint);
	Atom window_type_atom = XInternAtom(display, WINDOW_TYPE, 0);
	XChangeProperty(display, window, XInternAtom(display, "_NET_WM_WINDOW_TYPE", 0), XA_ATOM, 32, PropModeReplace, (unsigned char *)&window_type_atom, 1L);
	Atom wm_protocols[] = { ATOM_WM_DELETE_WINDOW };
	XSetWMProtocols(display, window, wm_protocols, sizeof(wm_protocols)/sizeof(Atom));

	gl_data_t gl_data;
	gl_init(&gl_data, display, visual_info, window);

	XMapWindow(display, window);
	XFlush(display);

	XEvent event;
	XWindowAttributes window_attributes;
	int rightpanel = 200;
	int toppanel = 40;
	while (1) {
		XNextEvent(display, &event);
		switch (event.type) {
			case PropertyNotify:
				Atom type;
				int format;
				unsigned long nitems, bytes_after;
				unsigned char *data;
				if (event.xproperty.atom == ATOM_WALLPAPER_PATH) {
					XGetWindowProperty(display, window, ATOM_WALLPAPER_PATH, 0L, 0L, 0, XA_STRING, &type, &format, &nitems, &bytes_after, &data);
					XGetWindowProperty(display, window, ATOM_WALLPAPER_PATH, 0L, bytes_after/4+1, 0, XA_STRING, &type, &format, &nitems, &bytes_after, &data);
					gl_load_texture(&gl_data, 0, (char*)data);
					gl_render(&gl_data);
					DEBUG("wallpaper set to '%s'", data);
					XFree(data);
				} else if (event.xproperty.atom == ATOM_NET_WM_STRUT) {
					XGetWindowProperty(display, window, ATOM_NET_WM_STRUT, 0L, 12L, 0, XA_CARDINAL, &type, &format, &nitems, &bytes_after, &data);
					set_net_wm_strut_partial(display, window, ((short*)data)[0], ((short*)data)[1], ((short*)data)[2], ((short*)data)[3]);
					XFree(data);
				}
				break;
			case Expose:
				gl_resize(&gl_data);
				gl_render(&gl_data);
				break;
			case ClientMessage:
				if (event.xclient.message_type == ATOM_WM_PROTOCOLS && event.xclient.data.l[0] == ATOM_WM_DELETE_WINDOW) {
					DEBUG("h");
					set_net_wm_strut_partial(display, window, 0, 0, 0, 0);
					//glXMakeCurrent(display, None, NULL);
					//glXDestroyContext(display, glx_context);
					XDestroyWindow(display, window);
					XCloseDisplay(display);

					goto FINISH;
				}
			default:
				break;
		}
	}

FINISH:
	return EXIT_SUCCESS;
}
