/*
 * syscall/seek.c
 *
 * Create by Le Min at 06/23/2015
 */

#include <os/kernel.h>
#include <os/errno.h>
#include <os/printk.h>
#include <os/syscall.h>
#include <os/syscall_nr.h>
#include <os/sched.h>

int sys_seek(int fd, offset_t offset, int whence)
{
	return _sys_seek(fd, offset, whence);
}
DEFINE_SYSCALL(seek, __NR_lseek, (void *)sys_seek);

