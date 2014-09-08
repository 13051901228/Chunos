/*
 * syscall/getcwd.c
 *
 * Create by Le Min at 08/23/2014
 */

#include <os/kernel.h>
#include <errno.h>
#include <os/printk.h>
#include <os/syscall.h>
#include <os/syscall_nr.h>

static int sys_getcwd(char *buf, int size)
{
	strcpy(buf, "/home/mirror/");

	return 13;
}
DEFINE_SYSCALL(getcwd, __NR_getcwd, sys_getcwd);

