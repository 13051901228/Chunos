/*
 * syscall/fork.c
 *
 * Create by Le Min at 06/23/2015
 */

#include <os/kernel.h>
#include <os/errno.h>
#include <os/printk.h>
#include <os/syscall.h>
#include <os/syscall_nr.h>
#include <os/sched.h>

pid_t sys_fork(void)
{
	unsigned long flag = 0;
	pt_regs *regs = get_pt_regs();

	/* indicate fork will sucessed to generate
	 * child process */
	flag |= (PROCESS_TYPE_USER | PROCESS_FLAG_FORK);

	return do_fork(current->name, regs, flag);
}
DEFINE_SYSCALL(fork, __NR_fork, (void *)sys_fork);

