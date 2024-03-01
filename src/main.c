#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "config.h"
#include "log.h"
#include "window.h"

int set_struts(Display *display, short left, short right, short top, short bottom) {
	Window window = get_program_window(display);
	if (window == 0x0) {
		ERR("get_program_window(): unable to find window");
		XCloseDisplay(display);
		exit(EXIT_FAILURE);
	}

	return set_net_wm_strut(display, window, left, right, top, bottom);
}

// check whether `filename' refers to a file that stbi can load (using internal functions to test to avoid potentially parsing the full file as that is done later by the server)
int stbi_valid(char *filename) {
	FILE *file = stbi__fopen(filename, "rb");
	if (!file) return 0;

	stbi__context s;
	stbi__start_file(&s, file);

	int valid = (0 ||
#ifndef STBI_NO_PNG
		stbi__png_test(&s) ||
#endif
#ifndef STBI_NO_BMP
		stbi__bmp_test(&s) ||
#endif
#ifndef STBI_NO_GIF
		stbi__gif_test(&s) ||
#endif
#ifndef STBI_NO_PSD
		stbi__psd_test(&s) ||
#endif
#ifndef STBI_NO_PIC
		stbi__pic_test(&s) ||
#endif
#ifndef STBI_NO_JPEG
		stbi__jpeg_test(&s) ||
#endif
#ifndef STBI_NO_PNM
		stbi__pnm_test(&s) ||
#endif
#ifndef STBI_NO_HDR
		stbi__hdr_test(&s) ||
#endif
#ifndef STBI_NO_TGA
		stbi__tga_test(&s) ||
#endif
		0);

	fclose(file);

	return valid;
}

int absolute_path(char **abs_path, char *rel_path) {
	size_t rel_path_len = strlen(rel_path);

	switch (rel_path[0]) {
		case '/':
			*abs_path = malloc(sizeof(char)*(rel_path_len+1));
			strcpy(*abs_path, rel_path);
			break;
		case '~':
			char *home = getenv("HOME");
			size_t home_len = strlen(home);
			*abs_path = malloc(sizeof(char)*(home_len+rel_path_len-1));
			memcpy(*abs_path, home, home_len);
			memcpy(*abs_path+home_len, rel_path+1, rel_path_len-1);
			break;
		case '.':
			if (rel_path[1] == '/') {
				rel_path += 2;
				rel_path_len -= 2;
			}
		default:
			char *pwd = getcwd(NULL, 0);
			size_t pwd_len = strlen(pwd);
			*abs_path = malloc(sizeof(char)*(pwd_len+rel_path_len+2));
			memcpy(*abs_path, pwd, pwd_len);
			(*abs_path)[pwd_len] = '/';
			memcpy(*abs_path+pwd_len+1, rel_path, rel_path_len+1);
			free(pwd);
			break;
	}

	return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
	Display *display = XOpenDisplay(NULL);
	if (display == NULL) {
		ERR("XOpenDisplay(): unable to open display");
		exit(EXIT_FAILURE);
	}

	int prog_return = EXIT_SUCCESS;

	if (props_init(display) != EXIT_SUCCESS) {
		ERR("props_init(): unable to initialise atoms");
		XCloseDisplay(display);
		exit(EXIT_FAILURE);
	}

	if (argc == 1) {
		ERR("no subcommand provided");
		prog_return = EXIT_FAILURE;
	} else if (argc == 2 && (strcmp(argv[1], "server") == 0 || strcmp(argv[1], "daemon") == 0)) {
		if (get_program_window(display) == 0x0) {
			startup_properties_t startup_properties;

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
	} else if (strcmp(argv[1], "kill") == 0) {
			Window window = get_program_window(display);
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
	} else if (strcmp(argv[1], "set") == 0) {
		if (strcmp(argv[2], "bgimg") == 0) {
			Window window = get_program_window(display);
			if (window == 0x0) {
				ERR("get_program_window(): unable to find window");
				prog_return = EXIT_FAILURE;
			} else {
				switch (argc) {
					case 5:
						short duration = (short)atoi(argv[4]);
						XChangeProperty(display, window, ATOM_WALLPAPER_TRANSITION_DURATION, XA_CARDINAL, 16, PropModeReplace, (unsigned char *)&duration, 1);
					case 4:
						char *abs_path;
						absolute_path(&abs_path, argv[3]);
						if (stbi_valid(abs_path)) {
							XChangeProperty(display, window, ATOM_WALLPAPER_PATH, XA_STRING, 8, PropModeReplace, (unsigned char *)abs_path, strlen(abs_path));
						} else {
							ERR("stbi_valid(): invalid file");
							prog_return = EXIT_FAILURE;
						}
						free(abs_path);
						break;
					default:
						ERR("expected 1 or 2 value(s) after 'set %s'", argv[2]);
						prog_return = EXIT_FAILURE;
						break;
				}
			}
		} else if (strcmp(argv[2], "struts") == 0) {
			if (argc != 7) {
				ERR("expected 4 values after 'set %s'", argv[2]);
				prog_return = EXIT_FAILURE;
			} else {
				set_struts(display, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
			}
		} else if (argc != 4) {
			ERR("expected 1 value after 'set %s'", argv[2]);
			prog_return = EXIT_FAILURE;
		} else if (strcmp(argv[2], "left") == 0) set_struts(display, atoi(argv[3]), -1, -1, -1);
		else if (strcmp(argv[2], "right") == 0) set_struts(display, -1, atoi(argv[3]), -1, -1);
		else if (strcmp(argv[2], "top") == 0) set_struts(display, -1, -1, atoi(argv[3]), -1);
		else if (strcmp(argv[2], "bottom") == 0) set_struts(display, -1, -1, -1, atoi(argv[3]));
		else {
			ERR("unknown config key '%s'", argv[2]);
			prog_return = EXIT_FAILURE;
		}
	} else if (strcmp(argv[1], "subscribe") == 0) {
		enum atoms {
			struts		= (1 << 0),
			visibility	= (1 << 1),
			wallpaper	= (1 << 2),
			workspace	= (1 << 3)
		};
		short subscribed_atoms = 0;
		if (argc > 2) {
			for (int i = 2; i < argc; i++) {
				if (strcmp(argv[i], "struts") == 0) subscribed_atoms |= struts;
				else if (strcmp(argv[i], "visibility") == 0) subscribed_atoms |= visibility;
				else if (strcmp(argv[i], "wallpaper") == 0) subscribed_atoms |= wallpaper;
				else if (strcmp(argv[i], "workspace") == 0) subscribed_atoms |= workspace;
			}
		} else subscribed_atoms = 0xff;
		Window window = get_program_window(display);
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
				if (event.xproperty.atom == ATOM_NET_WM_STRUT) changed_atom = struts;
				else if (event.xproperty.atom == ATOM_WINDOW_VISIBLE) changed_atom = visibility;
				else if (event.xproperty.atom == ATOM_WALLPAPER_PATH) changed_atom = wallpaper;
				else if (event.xproperty.atom == ATOM_CURRENT_WORKSPACE) changed_atom = workspace;
				
				if (!(subscribed_atoms & changed_atom)) continue;

				Atom type;
				int format;
				unsigned long nitems, bytes_after;
				unsigned char *data;
				if (changed_atom == struts) {
					XGetWindowProperty(display, window, ATOM_NET_WM_STRUT, 0L, 12L, 0, XA_CARDINAL, &type, &format, &nitems, &bytes_after, &data);
					printf("struts %i %i %i %i\n", ((short*)data)[0], ((short*)data)[1], ((short*)data)[2], ((short*)data)[3]);
				} else if (changed_atom == visibility) {
					XGetWindowProperty(display, window, ATOM_WINDOW_VISIBLE, 0L, 1L, 0, XA_CARDINAL, &type, &format, &nitems, &bytes_after, &data);
					printf("visibility %i\n", *data);
				} else if (changed_atom == wallpaper) {
					XGetWindowProperty(display, window, ATOM_WALLPAPER_PATH, 0L, 0L, 0, XA_STRING, &type, &format, &nitems, &bytes_after, &data);
					XGetWindowProperty(display, window, ATOM_WALLPAPER_PATH, 0L, bytes_after/4+1, 0, XA_STRING, &type, &format, &nitems, &bytes_after, &data);
					printf("wallpaper %s\n", data);
				} else if (changed_atom == workspace) {
					XGetWindowProperty(display, window, ATOM_CURRENT_WORKSPACE, 0L, 1L, 0, XA_CARDINAL, &type, &format, &nitems, &bytes_after, &data);
					printf("workspace %i\n", *data);
				}

				fflush(stdout);
			}
		}
	} else {
		ERR("unknown subcommand '%s'", argv[1]);
		prog_return = EXIT_FAILURE;
	}

	XCloseDisplay(display);

	return prog_return;
}
