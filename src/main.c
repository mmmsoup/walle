#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include "config.h"
#include "gl.h"			// for stbi_valid
#include "log.h"
#include "util.h"
#include "window.h"

#include "gperf.h"
#include "gperf/property.h"
#include "gperf/subcmd.h"

#define STR(x) #x

int set_struts(Display *display, short left, short right, short top, short bottom) {
	Window window = get_program_window(display);
	if (window == 0x0) {
		ERR("get_program_window(): unable to find window");
		XCloseDisplay(display);
		exit(EXIT_FAILURE);
	}

	return set_net_wm_strut(display, window, left, right, top, bottom);
}

#define CHECK_N_ARGS(prop, n) { \
	if (i + n >= argc) { \
		ERR("expected %i arguments for property '%s', found %i", n, prop, argc - i - 1); \
		return EXIT_FAILURE; \
	} \
}

#define USHORT_FROM_STR(uintptr, str) { \
	char *end; \
	long l = strtol(str, &end, 10); \
	if (*end == '\0') { \
		if (l >= 0 && l <= UINT_MAX) *uintptr = (unsigned short)l; \
		else { \
			ERR("could not interpret '%s' as unsigned short: out of range [0,%u]", str, UINT_MAX); \
			return EXIT_FAILURE; \
		} \
	} else { \
		ERR("could not interpret '%s' as unsigned short: invalid character '%c'", str, *end); \
		return EXIT_FAILURE; \
	} \
}

int main(int argc, char **argv) {
	const gsiv_t *subcmd;

	if (argc == 1) {
		ERR("no subcommand provided - try '%s help'", argv[0]);
		return EXIT_FAILURE;
	} else {
		 subcmd = gperf_subcmd_lookup(argv[1], strlen(argv[1]));
	}
	
	if (subcmd->value == SUBCMD_HELP) {
		printf("%s [COMMAND] (ARGS)\n\n", argv[0]);
		printf(
				"COMMANDS\n"
				"\tserver (PROPERTIES_SET)\t\t-> start the server with optional properties given as '--property value1 (value 2 ...)'\n"
				"\tdaemon (PROPERTIES_SET)\t\t-> start the server and daemonise with optional properties like the non-daemonised subcommand\n"
				"\tkill\t\t\t\t-> kill the current instance\n"
				"\tset [PROPERTY_SET] [VALUE]\t-> set PROPERTY_SET of running instance to VALUE\n"
				"\tsetroot \"#XXXXXX\"\t\t-> set root window to solid hex colour #XXXXXX\n"
				"\tsubscribe (PROPERTIES_GET)\t-> subscribe to events when values of space-separated PROPERTIES_GET list change, or subscribe to all events if PROPERTIES_GET is omitted\n"
				"\thelp\t\t\t\t-> show this help message:)\n"
		);
		printf("\n");
		printf(
				"PROPERTIES_GET\n"
				"\tstruts\t\t-> empty space at edges of screen in px as the space-separated values [TOP] [BOTTOM] [LEFT] [RIGHT]\n"
				"\tvisibility\t-> whether or not the "PROJECT_NAME" window is obscured by one or more fullscreen or maximised windows (not perfect)\n"
				"\twallpaper\t-> full path of current wallpaper\n"
				"\tworkspace\t-> current workspace number\n"
		);
		printf("\n");
		printf(
				"PROPERTIES_SET\n"
				"\tbgcol \"#XXXXXX\" (TRANSITION)\t\t-> set wallpaper to single hex colour #XXXXXX with optional transition duration of TRANSITION milliseconds\n"
				"\tbgimg [PATH] (TRANSITION)\t\t-> set wallpaper to image at PATH with optional transition duration of TRANSITION milliseconds\n"
				"\tstruts [TOP] [BOTTOM] [LEFT] [RIGHT]\t-> set empty space at edges of screen in pixels\n"
				"\ttop [SIZE]\t\t\t\t-> set top strut to SIZE pixels\n"
				"\tbottom [SIZE]\t\t\t\t-> set bottom strut to SIZE pixels\n"
				"\tleft [SIZE]\t\t\t\t-> set left strut to SIZE pixels\n"
				"\tright [SIZE]\t\t\t\t-> set right strut to SIZE pixels\n"
		);
		return EXIT_SUCCESS;
	}

	Display *display = XOpenDisplay(NULL);
	if (display == NULL) {
		ERR("XOpenDisplay(): unable to open display");
		exit(EXIT_FAILURE);
	}

	int prog_return = EXIT_SUCCESS;

	if (subcmd->value == SUBCMD_SETROOT) {
		int screen = DefaultScreen(display);
		Visual *visual = DefaultVisual(display, screen);
		int width = DisplayWidth(display, screen);
		int height = DisplayHeight(display, screen);
		int depth = 24;
		Window root = RootWindow(display, screen);

		short valid_colour = 1;
		if (strlen(argv[2]) != 7) valid_colour = 0;
		else for (int i = 1; i < 7; i++) valid_colour *= (hex_char_val(argv[2][i]) != -1);

		if (!valid_colour) {
			ERR("invalid colour '%s'", argv[2]);
			prog_return = EXIT_FAILURE;
			goto shutdown;
		}

		/*
		// unforunately doesn't work with composite window manager... have to create a pixmap instead >:(
		XColor colour = {
			.red = (hex_char_val(argv[2][1]) * 16 + hex_char_val(argv[2][2])) << 8,
			.green = (hex_char_val(argv[2][3]) * 16 + hex_char_val(argv[2][4])) << 8,
			.blue = (hex_char_val(argv[2][5]) * 16 + hex_char_val(argv[2][6])) << 8,
			.flags = DoRed | DoBlue | DoGreen
		};
		XAllocColor(display, DefaultColormap(display, screen), &colour);

		XSetWindowBackground(display, root, colour.pixel);
		XClearWindow(display, root);
		*/

		// _XROOTPMAP_ID and ESETROOT_PMAP_ID stuff taken from https://github.com/himdel/hsetroot
		Atom prop_root = XInternAtom(display, "_XROOTPMAP_ID", True);
		Atom prop_eroot = XInternAtom(display, "ESETROOT_PMAP_ID", True);

		Atom type;
		int format;
		unsigned long length, after;
		unsigned char *data_root, *data_eroot;

		if ((prop_root != None) && (prop_eroot != None)) {
			XGetWindowProperty(display, root, prop_root, 0L, 1L, False, AnyPropertyType, &type, &format, &length, &after, &data_root);
			if (type == XA_PIXMAP) {
				XGetWindowProperty(display, root, prop_eroot, 0L, 1L, False, AnyPropertyType, &type, &format, &length, &after, &data_eroot);
				if (data_root && data_eroot && type == XA_PIXMAP && *((Pixmap*)data_root) == *((Pixmap*)data_eroot)) {
					XKillClient(display, *((Pixmap*)data_eroot));
					XFree(data_eroot);
				}
			}
			if (data_root != NULL) XFree(data_root);
		}

		XKillClient(display, AllTemporary);
		XSetCloseDownMode(display, RetainTemporary);

		prop_root = XInternAtom(display, "_XROOTPMAP_ID", False);
		prop_eroot = XInternAtom(display, "ESETROOT_PMAP_ID", False);

		unsigned char r = hex_char_val(argv[2][1]) * 16 + hex_char_val(argv[2][2]);
		unsigned char g = hex_char_val(argv[2][3]) * 16 + hex_char_val(argv[2][4]);
		unsigned char b = hex_char_val(argv[2][5]) * 16 + hex_char_val(argv[2][6]);

		char *img_data = malloc(sizeof(char)*(width*height*4));
		for (int i = 0; i < width*height*4; i += 4) {
			img_data[i] = b;
			img_data[i+1] = g;
			img_data[i+2] = r;
			img_data[i+3] = 0;
		}

		Pixmap pixmap = XCreatePixmap(display, root, width, height, depth);
		GC gc = XCreateGC(display, pixmap, 0, NULL);
		XImage *image = XCreateImage(display, visual, depth, ZPixmap, 0, img_data, width, height, 32, 0);
		XPutImage(display, pixmap, gc, image, 0, 0, 0, 0, width, height);
		XFree(gc);
		XFree(image);
		free(img_data);

		XSetWindowBackgroundPixmap(display, root, pixmap);
		XClearWindow(display, root);

		XChangeProperty(display, root, prop_root, XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pixmap, 1);
		XChangeProperty(display, root, prop_eroot, XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pixmap, 1);

		goto shutdown;
	}

	if (props_init(display) != EXIT_SUCCESS) {
		ERR("props_init(): unable to initialise atoms");
		prog_return = EXIT_FAILURE;
		goto shutdown;
	}

	Window window;

	switch (subcmd->value) {
		case SUBCMD_SERVER:
		case SUBCMD_DAEMON:
			if (get_program_window(display) == 0x0) {
				startup_properties_t startup_properties;
				memset(&startup_properties, 0, sizeof(startup_properties_t));

				short i = 2;
				while (i < argc) {
					if (argv[i][0] == '-' && argv[i][1] == '-') {
						const char *stropts[] = { "struts", "left", "right", "top", "bottom", "bgimg" };
						enum { none = -1, struts, left, right, top, bottom, bgimg } opt = none;
						for (size_t j = 0; j < sizeof(stropts)/sizeof(char*); j++) {
							if (strcmp(argv[i]+2, stropts[j]) == 0) {
								opt = j;
								break;
							}
						}

						switch (opt) {
							case struts:
								CHECK_N_ARGS("struts", 4);
								for (int j = 0; j < 4; j++) USHORT_FROM_STR(&(startup_properties.struts[j]), argv[i+j+1]);
								i += 4;
								break;
							case left:
							case right:
							case top:
							case bottom:
								CHECK_N_ARGS(argv[i]+2, 1);
								// this relies on left,right,top,bottom being contiguous and in order in the above enum so probably not great practice??
								USHORT_FROM_STR(&(startup_properties.struts[opt-left]), argv[i+1]);
								i++;
								break;
							case bgimg:
								CHECK_N_ARGS(argv[i]+2, 1);
								absolute_path(&(startup_properties.bgimg), argv[i+1]);
								if (!stbi_valid(startup_properties.bgimg)) {
									ERR("invalid path for bgimg: '%s'", argv[i+1]);
									return EXIT_FAILURE;
								}
								i++;
								break;
							case none:
								ERR("unknown option '%s'", argv[i]+2);
								return EXIT_FAILURE;
							default:
								ERR("not sure how this happened??");
								return EXIT_FAILURE;
						}
					} else {
						ERR("expected property option, got '%s'", argv[i]);
						return EXIT_FAILURE;
					}

					i++;
				}

				if (argv[1][0] == 'd') { // daemonise
					int fildes[2];
					if (pipe(fildes) == 0) {
						int pid = fork();
						if (pid > 0) { // parent
							close(fildes[1]);
							read(fildes[0], (char*)&prog_return, sizeof(int));
							return prog_return;
						} else if (pid == 0) { // child
							close(fildes[0]);
							prog_return = window_run(display, startup_properties, fildes[1]);
						} else { // error
							ERR_ERRNO("fork()");
							prog_return = errno;
						}
					} else {
						ERR_ERRNO("pipe()");
						prog_return = errno;
					}
				} else { // run in foreground
					prog_return = window_run(display, startup_properties, 0);
				}
			} else {
				ERR("get_program_window(): instance already running");
				prog_return = EXIT_FAILURE;
			}
			break;
		case SUBCMD_KILL:
			window = get_program_window(display);
			if (window == 0x0) {
				ERR("get_program_window(): unable to find window");
				prog_return = EXIT_FAILURE;
			} else {
				XEvent event;
				event.xclient.type         = ClientMessage;
				event.xclient.message_type = ATOM_WM_PROTOCOLS;
				event.xclient.format       = 32;
				event.xclient.data.l[0]    = ATOM_WM_DELETE_WINDOW;
				event.xclient.data.l[1]    = CurrentTime;
				XSendEvent(display, window, 0, NoEventMask, &event);
			}
			break;
		case SUBCMD_GET:
			break;
		case SUBCMD_SET:
			const gsiv_t *property = gperf_property_lookup(argv[2], strlen(argv[2]));
			char *abs_path = NULL;
			switch (property->value) {
				case PROPERTY_BGCOL:
					short valid_colour = 1;
					if (strlen(argv[3]) != 7) valid_colour = 0;
					else for (int i = 1; i < 7; i++) valid_colour *= (hex_char_val(argv[3][i]) != -1);

					if (!valid_colour) {
						ERR("invalid colour '%s'", argv[3]);
						prog_return = EXIT_FAILURE;
						break;
					}

					abs_path = malloc(sizeof(char)*8);
					strcpy(abs_path, argv[3]);
				case PROPERTY_BGIMG:
					window = get_program_window(display);
					if (window == 0x0) {
						ERR("get_program_window(): unable to find window");
						prog_return = EXIT_FAILURE;
						if (abs_path != NULL) free(abs_path);
					} else {
						switch (argc) {
							case 5:
								short duration = (short)atoi(argv[4]);
								XChangeProperty(display, window, ATOM_WALLPAPER_TRANSITION_DURATION, XA_CARDINAL, 16, PropModeReplace, (unsigned char *)&duration, 1);
							case 4:
								if (abs_path == NULL) {
									absolute_path(&abs_path, argv[3]);
									if (!stbi_valid(abs_path)) {
										ERR("stbi_valid(): invalid file");
										prog_return = EXIT_FAILURE;
									}
								}
								XChangeProperty(display, window, ATOM_WALLPAPER_PATH, XA_STRING, 8, PropModeReplace, (unsigned char *)abs_path, strlen(abs_path));
								free(abs_path);
								break;
							default:
								ERR("expected 1 or 2 value(s) after 'set %s'", argv[2]);
								prog_return = EXIT_FAILURE;
								break;
						}
					}
					break;
				case PROPERTY_STRUTS:
					if (argc != 7) {
						ERR("expected 4 values after 'set %s'", argv[2]);
						prog_return = EXIT_FAILURE;
					} else {
						set_struts(display, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
					}
					break;
				case PROPERTY_LEFT:
					set_struts(display, atoi(argv[3]), -1, -1, -1);
					break;
				case PROPERTY_RIGHT:
					set_struts(display, -1, atoi(argv[3]), -1, -1);
					break;
				case PROPERTY_TOP:
					set_struts(display, -1, -1, atoi(argv[3]), -1);
					break;
				case PROPERTY_BOTTOM:
					set_struts(display, -1, -1, -1, atoi(argv[3]));
					break;
				default:
					ERR("unknown config key '%s'", argv[2]);
					prog_return = EXIT_FAILURE;
					break;
			}
			break;
		case SUBCMD_SUBSCRIBE:
			int subscribed_atoms = 0;
			if (argc > 2) {
				for (int i = 2; i < argc; i++) {
					const gsiv_t *gsiv = gperf_property_lookup(argv[i], strlen(argv[i]));
					if (gsiv == NULL) {
						ERR("not a subscribable property: '%s'", argv[i]);
						prog_return = EXIT_FAILURE;
						goto shutdown;
					}
					subscribed_atoms |= gsiv->value;
				}
			} else subscribed_atoms = 0xffff;
			subscribed_atoms &= 0xff;

			window = get_program_window(display);
			if (window == 0x0) {
				ERR("get_program_window(): unable to find window");
				XCloseDisplay(display);
				exit(EXIT_FAILURE);
			}
			XSelectInput(display, window, PropertyChangeMask);
			if (props_init(display) != EXIT_SUCCESS) {
				ERR("props_init(): unable to initialise atoms");
				XCloseDisplay(display);
				exit(EXIT_FAILURE);
			}
			XEvent event;
			while (1) {
				XNextEvent(display, &event);
				if (event.type == PropertyNotify) {
					short changed_atom = 0;
					if (event.xproperty.atom == ATOM_NET_WM_STRUT) changed_atom = PROPERTY_STRUTS;
					else if (event.xproperty.atom == ATOM_WINDOW_VISIBLE) changed_atom = PROPERTY_VISIBILITY;
					else if (event.xproperty.atom == ATOM_WALLPAPER_PATH) changed_atom = PROPERTY_BGIMG;
					else if (event.xproperty.atom == ATOM_CURRENT_WORKSPACE) changed_atom = PROPERTY_WORKSPACE;
					
					if (!(subscribed_atoms & changed_atom)) continue;

					Atom type;
					int format;
					unsigned long nitems, bytes_after;
					unsigned char *data;

					switch (changed_atom) {
						case PROPERTY_BGIMG:
							XGetWindowProperty(display, window, ATOM_WALLPAPER_PATH, 0L, 0L, 0, XA_STRING, &type, &format, &nitems, &bytes_after, &data);
							XGetWindowProperty(display, window, ATOM_WALLPAPER_PATH, 0L, bytes_after/4+1, 0, XA_STRING, &type, &format, &nitems, &bytes_after, &data);
							printf("wallpaper %s\n", data);
							break;
						case PROPERTY_STRUTS:
							XGetWindowProperty(display, window, ATOM_NET_WM_STRUT, 0L, 12L, 0, XA_CARDINAL, &type, &format, &nitems, &bytes_after, &data);
							printf("struts %i %i %i %i\n", ((short*)data)[0], ((short*)data)[1], ((short*)data)[2], ((short*)data)[3]);
							break;
						case PROPERTY_VISIBILITY:
							XGetWindowProperty(display, window, ATOM_WINDOW_VISIBLE, 0L, 1L, 0, XA_CARDINAL, &type, &format, &nitems, &bytes_after, &data);
							printf("visibility %i\n", *data);
							break;
						case PROPERTY_WORKSPACE:
							XGetWindowProperty(display, window, ATOM_CURRENT_WORKSPACE, 0L, 1L, 0, XA_CARDINAL, &type, &format, &nitems, &bytes_after, &data);
							printf("workspace %i\n", *data);
							break;
					}
						
					XFree(data);
					fflush(stdout);
				}
			}
			break;
		default:
			ERR("unknown subcommand '%s' - try '%s help'", argv[1], argv[0]);
			prog_return = EXIT_FAILURE;
			break;
	}

shutdown:
	XCloseDisplay(display);

	return prog_return;
}
