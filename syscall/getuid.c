/*
 * syscall/getuid.c
 *
 * Create by Le Min at 08/23/2014
 */

#include <os/kernel.h>
#include <os/errno.h>
#include <os/printk.h>
#include <os/syscall.h>
#include <os/syscall_nr.h>

static int sys_getuid(void)
{
	return 0;
}
DEFINE_SYSCALL(getuid, __NR_getuid, sys_getuid);

