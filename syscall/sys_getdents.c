/*
 * syscall/getdents.c
 *
 * Create by Le Min at 06/23/2015
 */

#include <os/kernel.h>
#include <os/errno.h>
#include <os/printk.h>
#include <os/syscall.h>
#include <os/syscall_nr.h>
#include <os/sched.h>

int sys_getdents(int fd, char *buf, size_t count)
{
	return _sys_getdents(fd, buf, count);
}
DEFINE_SYSCALL(getdents, __NR_getdents, (void *)sys_getdents);
DEFINE_SYSCALL(getdents64, __NR_getdents64, (void *)sys_getdents);

