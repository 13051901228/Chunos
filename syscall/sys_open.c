/*
 * syscall/open.c
 *
 * Create by Le Min at 06/23/2015
 */

#include <os/kernel.h>
#include <os/errno.h>
#include <os/printk.h>
#include <os/syscall.h>
#include <os/syscall_nr.h>
#include <os/sched.h>

int sys_open(char *name, int flag)
{
	return _sys_open(name, flag);
}
DEFINE_SYSCALL(open, __NR_open, (void *)sys_open);

