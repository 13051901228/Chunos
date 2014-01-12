#include <os/types.h>
#include <os/mm.h>
#include <os/init.h>
#include <os/string.h>
#include <os/mirros.h>
#include <os/slab.h>
#include <os/printk.h>
#include <os/panic.h>
#include <os/task.h>
#include <os/sched.h>
#include <os/mutex.h>

extern int system_killer(void *arg);

int init_task(void *arg)
{
	kernel_exec("init");

	return 0;
}
