/*
 * syscall/getcwd.c
 *
 * Create by Le Min at 08/23/2014
 */

#include <os/kernel.h>
#include <os/errno.h>
#include <os/printk.h>
#include <os/syscall.h>
#include <os/syscall_nr.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/file.h>

static int sys_getcwd(char *buf, int size)
{
	strcpy(buf, current->file_desc->cdir_name);

	return strlen(current->file_desc->cdir_name);
}
DEFINE_SYSCALL(getcwd, __NR_getcwd, sys_getcwd);

