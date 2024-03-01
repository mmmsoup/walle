#include "bspwm.h"

int WM_IS_BSPWM = 0;
char *bspc;

int bspwm_init(Display *display) {
	Atom supporting_wm_check = XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", 0);
	if (supporting_wm_check == None) {
		WM_IS_BSPWM = 0;
		return EXIT_SUCCESS;
	}

	Atom type;
	int format;
	unsigned long nitems, bytes_after;
	unsigned char *data;
	XGetWindowProperty(display, DefaultRootWindow(display), supporting_wm_check, 0L, 1L, 0, XA_WINDOW, &type, &format, &nitems, &bytes_after, &data);
	if (nitems != 1) return EXIT_FAILURE;

	Window bspwm_window = *(Window*)data;
	XFree(data);

	Atom net_wm_name = XInternAtom(display, "_NET_WM_NAME", 0);
	if (net_wm_name == None) return EXIT_FAILURE;

	bytes_after = 0;
	XGetWindowProperty(display, bspwm_window, net_wm_name, 0L, 0L, 0, AnyPropertyType, &type, &format, &nitems, &bytes_after, &data);
	if (bytes_after == 0) {
		WM_IS_BSPWM = 0;
		return EXIT_SUCCESS;
	}
	XGetWindowProperty(display, bspwm_window, net_wm_name, 0L, bytes_after/4+1, 0, AnyPropertyType, &type, &format, &nitems, &bytes_after, &data);
	if (nitems != 5) {
		WM_IS_BSPWM = 0;
		return EXIT_SUCCESS;
	}

	if (strcmp((char*)data, "bspwm") == 0) {
		WM_IS_BSPWM = 1;
		XFree(data);
		return find_in_path("bspc", &bspc);
	} else {
		WM_IS_BSPWM = 0;
		XFree(data);
		return EXIT_SUCCESS;
	}
}

int find_in_path(char *filename, char **filepath) {
	char *pathstr = strdup(getenv("PATH"));
	if (pathstr == NULL) {
		ERR("PATH environment variable is empty");
		return EXIT_FAILURE;
	}

	struct dirent *entry;
	DIR *directory;

	char *start = pathstr;
	char *end = pathstr;

	while (*end != '\0') {
		if (*end == PATH_DELIM) {
			*end = '\0';

			directory = opendir(start);

			while (1) {
				entry = readdir(directory);
				if (entry == NULL) break;
				else if (strcmp(entry->d_name, filename) == 0) {
					*filepath = malloc(sizeof(char) * (end-start+2+strlen(filename)));
					sprintf(*filepath, "%s/%s", start, filename);
					return EXIT_SUCCESS;
				}
			}

			end++;
			start = end;
		}
		end++;
	}

	ERR("unable to find '%s' in PATH", filename);

	return EXIT_FAILURE;
}

void bspwm_set_struts(int left, int right, int top, int bottom) {
	char numstr[11]; // log_10(2^sizeof(uint32_t)) + 1
	sprintf(numstr, "%i", left);
	VA_SYSTEM(bspc, "config", "left_padding", numstr);
	sprintf(numstr, "%i", right);
	VA_SYSTEM(bspc, "config", "right_padding", numstr);
	sprintf(numstr, "%i", top);
	VA_SYSTEM(bspc, "config", "top_padding", numstr);
	sprintf(numstr, "%i", bottom);
	VA_SYSTEM(bspc, "config", "bottom_padding", numstr);
	return;
}
