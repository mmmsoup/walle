#include <util.h>

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

char hex_char_val(char hex) {
	return (hex >= 0x30 && hex <= 0x39) ? (hex - 0x30) : ((hex >= 0x61 && hex <= 0x66) ? (hex - 0x57) : ((hex >= 0x41 && hex <= 0x46) ? (hex - 0x37) : -1));
}
