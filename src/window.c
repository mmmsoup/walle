#include "window.h"

Atom ATOM_WM_PROTOCOLS;
Atom ATOM_WM_DELETE_WINDOW;

Atom ATOM_NET_WM_STRUT;
Atom ATOM_NET_WM_STRUT_PARTIAL;

Atom ATOM_WALLPAPER_PATH;
Atom ATOM_WALLPAPER_TRANSITION_DURATION;

Atom ATOM_CURRENT_WORKSPACE;
Atom ATOM_WINDOW_VISIBLE;

int props_init(Display *display) {
	ATOM_NET_WM_STRUT = XInternAtom(display, "_NET_WM_STRUT", 0);
	ATOM_NET_WM_STRUT_PARTIAL = XInternAtom(display, "_NET_WM_STRUT_PARTIAL", 0);

	ATOM_WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", 0);
	ATOM_WM_PROTOCOLS = XInternAtom(display, "WM_PROTOCOLS", 0);

	ATOM_WALLPAPER_PATH = XInternAtom(display, WALLPAPER_PATH_ATOM_NAME, 0);
	ATOM_WALLPAPER_TRANSITION_DURATION = XInternAtom(display, WALLPAPER_TRANSITION_DURATION_ATOM_NAME, 0);

	ATOM_CURRENT_WORKSPACE = XInternAtom(display, CURRENT_WORKSPACE_ATOM_NAME, 0);
	ATOM_WINDOW_VISIBLE = XInternAtom(display, WINDOW_VISIBLE_ATOM_NAME, 0);

	return EXIT_SUCCESS;
}

static char *xevent_name(int xevent_type) {
	switch (xevent_type) {
		case KeyPress: return "KeyPress";
		case KeyRelease: return "KeyRelease";
		case ButtonPress: return "ButtonPress";
		case ButtonRelease: return "ButtonRelease";
		case MotionNotify: return "MotionNotify";
		case EnterNotify: return "EnterNotify";
		case LeaveNotify: return "LeaveNotify";
		case FocusIn: return "FocusIn";
		case FocusOut: return "FocusOut";
		case Expose: return "Expose";
		case GraphicsExpose: return "GraphicsExpose";
		case NoExpose: return "NoExpose";
		case VisibilityNotify: return "VisibilityNotify";
		case CreateNotify: return "CreateNotify";
		case DestroyNotify: return "DestroyNotify";
		case UnmapNotify: return "UnmapNotify";
		case MapNotify: return "MapNotify";
		case MapRequest: return "MapRequest";
		case ReparentNotify: return "ReparentNotify";
		case ConfigureNotify: return "ConfigureNotify";
		case GravityNotify: return "GravityNotify";
		case ResizeRequest: return "ResizeRequest";
		case ConfigureRequest: return "ConfigureRequest";
		case CirculateNotify: return "CirculateNotify";
		case CirculateRequest: return "CirculateRequest";
		case PropertyNotify: return "PropertyNotify";
		case SelectionClear: return "SelectionClear";
		case SelectionRequest: return "SelectionRequest";
		case SelectionNotify: return "SelectionNotify";
		case ColormapNotify: return "ColormapNotify";
		case ClientMessage: return "ClientMessage";
		case MappingNotify: return "MappingNotify";
		case KeymapNotify: return "KeymapNotify";
		default: return "<unknown>";
	}
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
	for (unsigned int i = 0; i < nchildren; i++) {
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
		new_struts[3] = bottom == -1 ? old_struts[3] : bottom;
	}
	XFree(old_struts);
	XChangeProperty(display, window, ATOM_NET_WM_STRUT, XA_CARDINAL, 16, PropModeReplace, (unsigned char *)new_struts, 4);
	XFlush(display);
	return EXIT_SUCCESS;
}

int set_net_wm_strut_partial(Display *display, Window window, short left, short right, short top, short bottom) {
#ifdef ENABLE_RESIZE_HACK
	Atom type;
	int format;
	unsigned long nitems, bytes_after;
	short *old_struts;
	XGetWindowProperty(display, window, ATOM_NET_WM_STRUT_PARTIAL, 0L, 4L, 0, XA_CARDINAL, &type, &format, &nitems, &bytes_after, (unsigned char **)&old_struts);
#endif

	short struts[] = { left, right, top, bottom, 0, 0, 0, 0, 0, 0, 0, 0};
	XChangeProperty(display, window, ATOM_NET_WM_STRUT_PARTIAL, XA_CARDINAL, 16, PropModeReplace, (unsigned char *)struts, 12);
	XFlush(display);

	if (WM_IS_BSPWM) bspwm_set_struts(left, right, top, bottom);

	DEBUG("struts set to (%i, %i, %i, %i)", left, right, top, bottom);

	int screen_no = XDefaultScreen(display);
	int width = DisplayWidth(display, screen_no) - left - right;
	int height = DisplayHeight(display, screen_no) - top - bottom;
	XMoveResizeWindow(display, window, left, top, width, height);
	DEBUG("moved to (%i, %i)", left, top);
	DEBUG("resized to %ix%ipx", width, height);
	XFlush(display);
	//DEBUG("struts set to (%i, %i, %i, %i), window moved to (%i, %i) & resized to %ix%ipx", left, right, top, bottom, left, top, width, height);
	
#ifdef ENABLE_RESIZE_HACK
	if (nitems >= 4) {
		if (old_struts[0] != left || old_struts[1] != right) {
			system(RESIZE_HACK_CMD);
		}
	}
	if (nitems != 0) XFree(old_struts);
#endif
	
	return EXIT_SUCCESS;
}

// pass exit code back to parent process (if daemonised) and return exit code (if not daemonised)
#define WR_RET(code) { \
	if (fd != 0) { \
		int c = code; \
		write(fd, (char*)&c, sizeof(int)); \
		close(fd); \
	} \
	return code; \
}

// fd: if should be daemonised, 'fd' will refer to the write end of a pipe, which will be written to when the window is created, so the parent process can return
int window_run(Display *display, startup_properties_t startup_properties, int fd) {
	if (bspwm_init(display) != EXIT_SUCCESS) WR_RET(EXIT_FAILURE);

	//printf("%s\n", startup_properties.bgimg);
	//return 0;

	Screen *screen = DefaultScreenOfDisplay(display);
	unsigned int screen_width = WidthOfScreen(screen);
	unsigned int screen_height = HeightOfScreen(screen);

	unsigned short struts[4];
	memcpy(struts, startup_properties.struts, sizeof(startup_properties.struts));

	unsigned int maximised_width = screen_width - struts[0] - struts[1];
	unsigned int maximised_height = screen_height - struts[2] - struts[3];

	Window root_win = DefaultRootWindow(display);

	GLint visual_attrs[] = {
		GLX_RGBA,
		GLX_DEPTH_SIZE, 24,
		GLX_DOUBLEBUFFER,
		None
	};
	XVisualInfo *visual_info = glXChooseVisual(display, 0, visual_attrs);
	if (visual_info == NULL) {
		ERR("glXChooseVisual(): no appropriate visual found");
		WR_RET(EXIT_FAILURE);
	}

	Colormap colourmap = XCreateColormap(display, root_win, visual_info->visual, AllocNone);
	long event_mask = StructureNotifyMask | ExposureMask | PropertyChangeMask;
	unsigned long value_mask = CWColormap | CWEventMask | CWBitGravity;
	XSetWindowAttributes window_attrs = {
		.colormap = colourmap,
		.event_mask = event_mask,
		.bit_gravity = CenterGravity
	};
    Window window = XCreateWindow(display, root_win, 0, 0, screen_width, screen_height, 0, visual_info->depth, InputOutput, visual_info->visual, value_mask, &window_attrs);

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

	// set initial background
	if (startup_properties.bgimg != NULL) {
		gl_load_texture(&gl_data, 0, startup_properties.bgimg);
		gl_load_texture(&gl_data, 1, startup_properties.bgimg);
		XChangeProperty(display, window, ATOM_WALLPAPER_PATH, XA_STRING, 8, PropModeReplace, (unsigned char *)(startup_properties.bgimg), strlen(startup_properties.bgimg));
		if (startup_properties.historyfile != NULL) {
			FILE *fp = fopen(startup_properties.historyfile, "w");
			fwrite(startup_properties.bgimg, sizeof(char), strlen(startup_properties.bgimg), fp);
			fclose(fp);
		}
		free(startup_properties.bgimg);
	} else {
		gl_load_texture(&gl_data, 0, START_COLOUR);
		gl_load_texture(&gl_data, 1, START_COLOUR);
		if (startup_properties.historyfile != NULL) {
			FILE *fp = fopen(startup_properties.historyfile, "w");
			fwrite(START_COLOUR, sizeof(char), strlen(startup_properties.bgimg), fp);
			fclose(fp);
		}
	}
	gl_show_texture(&gl_data, 0, 0);

	XMapWindow(display, window);
	XFlush(display);

	// get new connection for monitoring visibility of wallpaper window
	Display *dpy_for_root = XOpenDisplay(NULL);
	if (display == NULL) {
		ERR("XOpenDisplay(): unable to open display");
		WR_RET(EXIT_FAILURE);
	}
	XSelectInput(dpy_for_root, root_win, SubstructureNotifyMask | PropertyChangeMask);
	Atom ATOM_NET_CURRENT_DESKTOP = XInternAtom(dpy_for_root, "_NET_CURRENT_DESKTOP", 0);
	Atom ATOM_NET_WM_DESKTOP = XInternAtom(dpy_for_root, "_NET_WM_DESKTOP", 0);
	struct pollfd pollfds[2] = {
		{
			.fd = ConnectionNumber(display),
			.events = POLLIN,
			.revents = 0
		},
		{
			.fd = ConnectionNumber(dpy_for_root),
			.events = POLLIN,
			.revents = 0
		}
	};
	window_list_t obscuring_windows;
	window_list_new(&obscuring_windows);
	unsigned int current_desktop = 0;

	// tell parent process to return: DO NOT use WR_RET() after this
	if (fd != 0) {
		int code = EXIT_SUCCESS;
		write(fd, (char*)&code, sizeof(int));
		close(fd);
		freopen("/dev/null", "w", stdout);
		freopen("/dev/null", "w", stderr);
	}

	// for subsequent XGetWindowProperty calls
	Atom type;
	int format;
	unsigned long nitems, bytes_after;
	unsigned char *data;

	XEvent event;
	int pollret = 0;
	while (1) {
		gl_render(&gl_data);
		usleep(1000);

		// block indefinitely if not animating wallpaper, otherwise just check with timeout 0 for events
		pollret = poll(pollfds, 2, gl_data.texture_transition.active - 1);
		if (pollret == 0) continue;
		else if (pollret < 0) {
			ERR_ERRNO("poll()");
			set_net_wm_strut_partial(display, window, 0, 0, 0, 0);
			XDestroyWindow(display, window);
			goto FINISH;
		}

		// monitor events for wallpaper window
		while (XPending(display)) {
			XNextEvent(display, &event);

			DEBUG("%s event received", xevent_name(event.type));
			
			switch (event.type) {
				case ClientMessage:
					if (event.xclient.message_type == ATOM_WM_PROTOCOLS && (Atom)(event.xclient.data.l[0]) == ATOM_WM_DELETE_WINDOW) {
						set_net_wm_strut_partial(display, window, 0, 0, 0, 0);
						//glXMakeCurrent(display, None, NULL);
						//glXDestroyContext(display, glx_context);
						XDestroyWindow(display, window);
						free(startup_properties.historyfile);
						DEBUG("killed");
						goto FINISH;
					}
				case Expose:
					if (event.xexpose.count != 0) break;
				case ConfigureNotify:
					set_net_wm_strut(display, window, struts[0], struts[1], struts[2], struts[3]);
					gl_resize(&gl_data);
					break;
				case PropertyNotify:
					if (event.xproperty.atom == ATOM_WALLPAPER_PATH) {
						short transition_duration = DEFAULT_WALLPAPER_TRANSITION_DURATION;
						XGetWindowProperty(display, window, ATOM_WALLPAPER_TRANSITION_DURATION, 0L, 1L, 1, XA_CARDINAL, &type, &format, &nitems, &bytes_after, &data);
						if (nitems == 1) {
							transition_duration = *(short*)data;
							XFree(data);
						}

						XGetWindowProperty(display, window, ATOM_WALLPAPER_PATH, 0L, 0L, 0, XA_STRING, &type, &format, &nitems, &bytes_after, &data);
						XGetWindowProperty(display, window, ATOM_WALLPAPER_PATH, 0L, bytes_after/4+1, 0, XA_STRING, &type, &format, &nitems, &bytes_after, &data);

						if (gl_load_texture(&gl_data, 1 - gl_data.current_texture, (char*)data) == EXIT_SUCCESS) {
							if (startup_properties.historyfile != NULL) {
								FILE *fp = fopen(startup_properties.historyfile, "w");
								fwrite(data, sizeof(char), strlen((char*)data), fp);
								fclose(fp);
							}
							DEBUG("setting wallpaper to '%s' (transition %ims)", data, transition_duration);
							gl_show_texture(&gl_data, 1 - gl_data.current_texture, transition_duration);
						}

						XFree(data);
					} else if (event.xproperty.atom == ATOM_NET_WM_STRUT) {
						XGetWindowProperty(display, window, ATOM_NET_WM_STRUT, 0L, 12L, 0, XA_CARDINAL, &type, &format, &nitems, &bytes_after, &data);
						memcpy(struts, data, 4*sizeof(short));
						set_net_wm_strut_partial(display, window, struts[0], struts[1], struts[2], struts[3]);
						maximised_width = screen_width - struts[0] - struts[1];
						maximised_height = screen_height - struts[2] - struts[3];
						XFree(data);
					}
					break;
				default:
					break;
			}
		}

		// monitor events for root window
		short visible = obscuring_windows.length == 0;
		while (XPending(dpy_for_root)) {
			XNextEvent(dpy_for_root, &event);

			DEBUG("%s event received", xevent_name(event.type));

			switch (event.type) {
				case ConfigureNotify:
					if (((event.xconfigure.x <= 0 && event.xconfigure.y <= 0 && (unsigned int)(event.xconfigure.width) >= screen_width && (unsigned int)(event.xconfigure.height) >= screen_height) || (event.xconfigure.x <= struts[0] && event.xconfigure.y <= struts[2] && (unsigned int)(event.xconfigure.width) >= maximised_width && (unsigned int)(event.xconfigure.height) >= maximised_height)) && (event.xconfigure.window != window)) {
						/* checking workspace of window in question fixes an issue arising from fullscreen windows on other workspaces being reconfigured */
						XGetWindowProperty(dpy_for_root, event.xconfigure.window, ATOM_NET_WM_DESKTOP, 0L, 1L, 0, XA_CARDINAL, &type, &format, &nitems, &bytes_after, &data);
						if (nitems > 0) {
							if (*(uint32_t*)data == current_desktop) window_list_add(&obscuring_windows, event.xconfigure.window);
							XFree(data);
						}
					} else window_list_remove(&obscuring_windows, event.xconfigure.window);
					break;
				case DestroyNotify:
					window_list_remove(&obscuring_windows, event.xdestroywindow.window);
					break;
				case MapNotify:
					Window r;
					int x, y;
					unsigned int w, h, bw, d;
					XGetGeometry(dpy_for_root, event.xmap.window, &r, &x, &y, &w, &h, &bw, &d);
					if (((x <= 0 && y <= 0 && w >= screen_width && h >= screen_height) || (x <= struts[0] && y <= struts[2] && w >= maximised_width && h >= maximised_height)) && (event.xmap.window != window)) {
						window_list_add(&obscuring_windows, event.xmap.window);
					}
					break;
				case PropertyNotify:
					if (event.xproperty.atom == ATOM_NET_CURRENT_DESKTOP) {
						XGetWindowProperty(dpy_for_root, root_win, ATOM_NET_CURRENT_DESKTOP, 0L, 1L, 0, XA_CARDINAL, &type, &format, &nitems, &bytes_after, &data);
						current_desktop = *(uint32_t*)data;
						XFree(data);
						XChangeProperty(display, window, ATOM_CURRENT_WORKSPACE, XA_CARDINAL, 8, PropModeReplace, (unsigned char*)&current_desktop, 1);
					}
					break;
				case UnmapNotify:
					window_list_remove(&obscuring_windows, event.xunmap.window);
					break;
				default:
					break;
			}
		}
		
		if ((obscuring_windows.length == 0) != visible) {
			visible = !visible;
			XChangeProperty(display, window, ATOM_WINDOW_VISIBLE, XA_CARDINAL, 8, PropModeReplace, (unsigned char*)&visible, 1);
		}
	}

FINISH:
	return EXIT_SUCCESS;
}

int window_list_new(window_list_t *list) {
	list->windows = malloc(sizeof(Window)*WINDOW_LIST_BLOCK_SIZE);
	list->length = 0;
	list->maxsize = WINDOW_LIST_BLOCK_SIZE;
	return EXIT_SUCCESS;
}

int window_list_resize(window_list_t *list, size_t newsize) {
	list->windows = realloc(list->windows, sizeof(Window)*newsize);
	list->maxsize = newsize;
	list->length = MIN(list->length, list->maxsize);
	return EXIT_SUCCESS;
}

int window_list_add(window_list_t *list, Window win) {
	if (list->length == list->maxsize) window_list_resize(list, list->maxsize+WINDOW_LIST_BLOCK_SIZE);
	for (size_t i = 0; i < list->length; i++) {
		if (list->windows[i] == win) return list->length;
	}
	list->windows[list->length] = win;
	list->length++;
	return list->length;
}

int window_list_remove(window_list_t *list, Window win) {
	for (size_t i = 0; i < list->length; i++) {
		if (list->windows[i] == win) {
			for (size_t j = i+1; j < list->length; j++) {
				list->windows[j-1] = list->windows[j];
			}
			list->length--;
			return list->length;
		}
	}
	return -1;
}

int window_list_delete(window_list_t *list) {
	free(list->windows);
	return EXIT_SUCCESS;
}
