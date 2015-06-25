/*
 * syscall/ioctl.c
 *
 * Create by Le Min at 06/23/2015
 */

#include <os/kernel.h>
#include <os/errno.h>
#include <os/printk.h>
#include <os/syscall.h>
#include <os/syscall_nr.h>
#include <os/sched.h>

int sys_ioctl(int fd, int request, void *arg)
{
	return _sys_ioctl(fd, request, arg);
}
DEFINE_SYSCALL(ioctl, __NR_ioctl, (void *)sys_ioctl);
