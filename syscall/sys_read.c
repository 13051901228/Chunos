/*
 * syscall/read.c
 *
 * Create by Le Min at 06/23/2015
 */

#include <os/kernel.h>
#include <os/errno.h>
#include <os/printk.h>
#include <os/syscall.h>
#include <os/syscall_nr.h>
#include <os/sched.h>

int sys_read(int fd, char *buffer, size_t size)
{
	return _sys_read(fd, buffer, size);
}
DEFINE_SYSCALL(read, __NR_read, (void *)sys_read);

