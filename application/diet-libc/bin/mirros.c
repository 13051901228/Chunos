#include <stdio.h>
#include <stdlib.h>

char *buf[1024] = { 0 };

int mirros_compile(char **argv)
{
	
}

/* mirros arm-linux-gcc -nostdlib -o test test.c */
int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("no command\n");
		return 1;
	}

	return mirros_compile(argv);
}
