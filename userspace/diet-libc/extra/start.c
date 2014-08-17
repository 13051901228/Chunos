#include <stdio.h>
#include <stdlib.h>

void __start(int argc, char **argv)
{
	int ret = 0;

	open("/dev/ttyS0", O_RDWR);
	open("/dev/ttyS0", O_RDWR);
	open("/dev/ttyS0", O_RDWR);

	ret = main(argc, argv);

	exit(ret);
}
