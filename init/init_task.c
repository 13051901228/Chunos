/*
 * init/init_task.c
 *
 * Created by Le Min(lemin9538@163.com)
 */

#include <os/kernel.h>

int init_task(void *arg)
{
	kernel_exec("/bin/init");

	return 0;
}
