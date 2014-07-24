#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

char *arg[] = { NULL};
char *env[] = { NULL};

void delay(int n)
{
	while (n--);
}

struct linux_dirent {
	long          d_ino;
	off_t         d_off;
	uint16_t      d_reclen;
	char          d_name[1];
};

int main(int argc, char **argv)
{
	int ret;
	int pid;
	void *start;
	void *start1;
	char *str = "1234567890";
	int fd;
	DIR *dir;
	struct dirent *dent;
	struct stat s_stat;

	printf("argc %d argv %s 0x%x\n", argc, argv[0], (unsigned int)argv[0]);

	start = mmap(NULL, 4096, 0, 0, -1, 0);
	if (!start) {
		printf(" can not map memory");
		while (1);
	}
	printf("start address is 0x%x\n", (unsigned int)start);

	start1 = mmap(NULL, 4 * 4096, 0, 0, -1, 0);
	if (!start1) {
		printf("can not map 4 pages\n");
		while (1);
	}

	memcpy(start1, str, 10);
	memcpy(start, str, 10);
	printf("str is %s\n", start);
	printf("str is %s\n", start1);

	munmap(start, 4096);
	munmap(start1, 4 * 4096);

	fd = open("/test", 0);
	if (fd >= 0) {
		printf("fd is %d\n", fd);
		start = mmap(NULL, 33, 0, 0, fd, 0);
		if (!start) {
			printf("can not map text.txt\n");
			while (1);
		}

		printf("%s\n", start);
		munmap(start, 33);
	}

	if (!stat("/test", &s_stat)) {
		printf("stat.mode is %x\n", s_stat.st_mode);
		printf("stat.ino is %x\n", s_stat.st_ino);
	}

	if (!fstat(fd, &s_stat)) {
		printf("stat.mode is %x\n", s_stat.st_mode);
		printf("stat.ino is %x\n", s_stat.st_ino);
	}

	while (1);
	printf("size of linux_dirent is %d\n", sizeof(struct linux_dirent));
	dir = opendir("/");
	if (!dir) {
		printf("can not open dir\n");
	}

	do {
		dent = readdir(dir);
		printf("%x-----\n", (unsigned int)dent);
		if (dent) {
			printf("%s\n", dent->d_name);
		}
	} while (dent);

	while(1);

	pid = fork();
	if (pid == 0) {
		while (1) {
			pid = getpid();
			printf("This is fater %d\n", pid);
			delay(6000000);
		}
	}
	else {
		printf("this is child %d\n", pid);
		ret = execve("/test", arg, env);
		printf("Failed to exec test\n");
	}

	return 0;
}
