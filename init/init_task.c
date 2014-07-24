#include <os/kernel.h>

int init_task(void *arg)
{
	kernel_exec("/init");

	return 0;
}
