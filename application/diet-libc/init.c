#include <stdio.h>
#include <stdlib.h>

char *arg[] = { NULL};
char *env[] = { NULL};

void delay(int n)
{
	while (n--);
}

int main(int argc, char **argv)
{
	int ret;
	int pid;

	printf("argc %d argv %s 0x%x\n", argc, argv[0], (unsigned int)argv[0]);

	pid = fork();
	if (pid == 0) {
		while (1) {
			printf("This is fater %d\n", pid);
			delay(6000000);
		}
	}
	else {
		printf("this is child %d\n", pid);
		ret = execve("test", arg, env);
		printf("Failed to exec test\n");
	}

	return 0;
}
