/*
 * syscall/exit.c
 *
 * Create by Le Min at 06/23/2015
 */

#include <os/kernel.h>
#include <os/errno.h>
#include <os/printk.h>
#include <os/syscall.h>
#include <os/syscall_nr.h>
#include <os/sched.h>

void sys_exit(int ret)
{
	struct task_struct *task = current;

	task_kill_self(task);
}
DEFINE_SYSCALL(exit, __NR_exit, (void *)sys_exit);

