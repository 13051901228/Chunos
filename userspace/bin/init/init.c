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

	printf("init process of system %d %s\n", argc, argv[0]);
	while (1);

	pid = fork();
	if (pid == 0) {
		while (1) {
			delay(60000);
	//		printf("this is father pid\n");
		}
	} else {
		execve("/bin/wshell", arg, env);
		printf("Failed to exec wshell\n");
	}

	return 0;
}
