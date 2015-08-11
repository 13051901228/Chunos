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
	int pid;
	void *buf;

	printf("init process of system\n");

	buf = malloc(100);
	printf("buf is %x\n", (unsigned int)buf);

	pid = fork();
	if (pid == 0) {
		while (1) {
			delay(60000);
			printf("this is father pid\n");
		}
	} else {
		execve("/bin/wshell", arg, env);
		printf("Failed to exec wshell\n");
	}

	return 0;
}
