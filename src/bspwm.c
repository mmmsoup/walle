#include "bspwm.h"

int WM_IS_BSPWM = 0;
char *bspc;

int bspwm_init() {
	// potentially assuming this is a bad idea (??)
	WM_IS_BSPWM = 1;
	return find_in_path("bspc", &bspc);
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
