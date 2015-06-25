/*
 * syscall/close.c
 *
 * Create by Le Min at 06/23/2015
 */

#include <os/kernel.h>
#include <os/errno.h>
#include <os/printk.h>
#include <os/syscall.h>
#include <os/syscall_nr.h>
#include <os/sched.h>

int sys_close(int fd)
{
	return _sys_close(fd);
}
DEFINE_SYSCALL(close, __NR_close, (void *)sys_close);
