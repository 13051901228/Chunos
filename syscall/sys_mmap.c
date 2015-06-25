/*
 * syscall/mmap.c
 *
 * Create by Le Min at 06/23/2015
 */

#include <os/kernel.h>
#include <os/errno.h>
#include <os/printk.h>
#include <os/syscall.h>
#include <os/syscall_nr.h>
#include <os/sched.h>

void *sys_mmap(void *addr, size_t length, int prot,
	       int flags, int fd, offset_t offset)
{
	return os_mmap(addr, length, prot, flags, fd, offset);
}
DEFINE_SYSCALL(mmap, __NR_mmap, (void *)sys_mmap);
DEFINE_SYSCALL(mmap2, __NR_mmap2, (void *)sys_mmap);

int sys_munmap(void *addr, size_t length)
{
	return os_munmap(addr, length);
}
DEFINE_SYSCALL(munmap, __NR_munmap, (void *)sys_munmap);

