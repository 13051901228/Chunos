/*
 * syscall/write.c
 *
 * Create by Le Min at 06/23/2015
 */

#include <os/kernel.h>
#include <os/errno.h>
#include <os/printk.h>
#include <os/syscall.h>
#include <os/syscall_nr.h>
#include <os/sched.h>

int sys_write(int fd, const void *buf, size_t count)
{
//	return _sys_write(fd, buf, count);
	xgold_uart_puts(buf);

	return count;
}
DEFINE_SYSCALL(write, __NR_write, (void *)sys_write);

