
#include <stdio.h>

extern int cfg_expand_env_vars(const char *src, char **result);

int main(int argc, char *argv[])
{
	char *result;
	cfg_expand_env_vars(argv[1], &result);
	puts(result);
	argc = 0;
	return 0;
}
