/*
 * syscall/geteuid.c
 *
 * Create by Le Min at 08/23/2014
 */

#include <os/kernel.h>
#include <errno.h>
#include <os/printk.h>
#include <os/syscall.h>
#include <os/syscall_nr.h>

static int sys_geteuid(void)
{
	return 0;
}
DEFINE_SYSCALL(geteuid, __NR_geteuid, sys_geteuid);

