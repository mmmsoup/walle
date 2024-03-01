#ifndef BSPWM_H
#define BSPWM_H

/*	Hello! This little workaround is here because of https://github.com/baskerville/bspwm/issues/1382 and was annoying since I use bspwm
 *	hopefully this will get fixed so this will be able to be removed:)
 */

#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include "log.h"

#define PATH_DELIM ':'

#define VA_SYSTEM(path, ...) { \
	pid_t pid = fork(); \
	if (pid == 0) { \
		fclose(stdout); \
		execl(path, path, ##__VA_ARGS__, (char *) NULL); \
		ERR_ERRNO("execl()"); \
		exit(EXIT_FAILURE); \
	} else if (pid < 0) { \
		ERR_ERRNO("fork()"); \
		exit(EXIT_FAILURE); \
	} \
}

extern int WM_IS_BSPWM;
extern char *bspc;

int bspwm_init(Display *);

int find_in_path(char *, char **);

void bspwm_set_struts(int, int, int, int);

#endif
