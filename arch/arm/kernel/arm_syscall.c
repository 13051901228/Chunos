#include <os/types.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/bitops.h>
#include <os/list.h>
#include <os/mm.h>
#include <os/slab.h>
#include <os/errno.h>
#include <os/printk.h>
#include <os/mmu.h>
#include <os/mm.h>
#include <os/task.h>
#include <os/syscall.h>

int sys_execve(char __user *filename,
	       char __user **argv,
	       char __user **envp)
{
	int ret = 0;
	pt_regs *regs = get_pt_regs();

	ret = do_exec(filename, argv, envp, regs);
	if (!ret)
		ret = regs->r0;

	return ret;
}
DEFINE_SYSCALL(execve, __NR_execve, (void *)sys_execve);

pid_t sys_fork(void)
{
	u32 flag = 0;
	pt_regs *regs = get_pt_regs();

	flag |= PROCESS_TYPE_USER | PROCESS_FLAG_FORK;

	return do_fork(NULL, regs, regs->sp, flag);
}
DEFINE_SYSCALL(fork, __NR_fork, (void *)sys_fork);

void sys_exit(int ret)
{
	struct task_struct *task = current;

	task_kill_self(current);
}
DEFINE_SYSCALL(exit, __NR_exit, (void *)sys_exit);

int sys_write(int fd, const void *buf, size_t count)
{
	if (fd == 1)
		return uart_put_char(buf, count);
}
DEFINE_SYSCALL(write, __NR_write, (void *)sys_write);
