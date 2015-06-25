/*
 * syscall/stat.c
 *
 * Create by Le Min at 06/23/2015
 */

#include <os/kernel.h>
#include <os/errno.h>
#include <os/printk.h>
#include <os/syscall.h>
#include <os/syscall_nr.h>
#include <os/sched.h>

int sys_stat(char *name, struct stat *stat)
{
	return _sys_stat(name, stat);
}
DEFINE_SYSCALL(stat, __NR_stat, (void *)sys_stat);
DEFINE_SYSCALL(lstat, __NR_lstat, (void *)sys_stat);

int sys_fstat(int fd, struct stat *stat)
{
	return _sys_fstat(fd, stat);
}
DEFINE_SYSCALL(fstat, __NR_fstat, (void *)sys_fstat);

