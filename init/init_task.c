#include <os/kernel.h>

int init_task(void *arg)
{
	kernel_exec("/bin/init");

	return 0;
}
