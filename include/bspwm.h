#ifndef BSPWM_H
#define BSPWM_H

#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

int bspwm_init();

int find_in_path(char *, char **);

void bspwm_set_struts(int, int, int, int);

#endif
