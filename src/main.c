#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>

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

int main(int argc, char **argv) {
	Display *display = XOpenDisplay(NULL);
	if (display == NULL) {
		ERR("XOpenDisplay(): unable to open display");
		exit(EXIT_FAILURE);
	}

	common_init(display);

	if (argc == 1 || strcmp(argv[1], "daemon") == 0) {
		if (get_program_window(display) == 0x0) {
			window_run(display);
		} else {
			ERR("get_program_window(): instance already running");
			XCloseDisplay(display);
			exit(EXIT_FAILURE);
		}
	} else if (strcmp(argv[1], "kill") == 0) {
			Window window = get_program_window(display);
			if (window == 0x0) {
				ERR("get_program_window(): unable to find window");
				XCloseDisplay(display);
				exit(EXIT_FAILURE);
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
		if (argc != 4) {
			ERR("expected 'name' 'value' after 'set' subcommand");
			XCloseDisplay(display);
			exit(EXIT_FAILURE);
		}

		if (strcmp(argv[2], "left") == 0) set_struts(display, atoi(argv[3]), -1, -1, -1);
		else if (strcmp(argv[2], "right") == 0) set_struts(display, -1, atoi(argv[3]), -1, -1);
		else if (strcmp(argv[2], "top") == 0) set_struts(display, -1, -1, atoi(argv[3]), -1);
		else if (strcmp(argv[2], "bottom") == 0) set_struts(display, -1, -1, -1, atoi(argv[3]));
		else if (strcmp(argv[2], "struts") == 0) {
			char *strs[4] = { argv[3], NULL, NULL, NULL };
			char *ptr = argv[3];
			int strindex = 0;
			while (*ptr != '\0') {
				if (*ptr == ',') {
					strindex++;
					*ptr = '\0';
					strs[strindex] = ptr + 1;
				}
				ptr++;
			}
			if (strs[3] == NULL || strs[3][0] == '\0') {
				ERR("expected format 'left,right,top,bottom' for config variable 'struts'");
				XCloseDisplay(display);
				exit(EXIT_FAILURE);
			} else {
				set_struts(display, atoi(strs[0]), atoi(strs[1]), atoi(strs[2]), atoi(strs[3]));
			}
		} else if (strcmp(argv[2], "wallpaper") == 0) {
			Window window = get_program_window(display);
			if (window == 0x0) {
				ERR("get_program_window(): unable to find window");
				XCloseDisplay(display);
				exit(EXIT_FAILURE);
			} else XChangeProperty(display, window, ATOM_WALLPAPER_PATH, XA_STRING, 8, PropModeReplace, (unsigned char *)argv[3], strlen(argv[3]));
		} else {
			ERR("unknown config key '%s'", argv[2]);
			XCloseDisplay(display);
			exit(EXIT_FAILURE);
		}

	} else {
		ERR("unknown subcommand '%s'", argv[1]);
	}

	XCloseDisplay(display);

	return EXIT_SUCCESS;
}
