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

	task_kill_self(task);
}
DEFINE_SYSCALL(exit, __NR_exit, (void *)sys_exit);

int sys_write(int fd, const void *buf, size_t count)
{
	if (fd == 1)
		return uart_put_char(buf, count);
}
DEFINE_SYSCALL(write, __NR_write, (void *)sys_write);

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

int sys_open(char *name, int flag)
{
	return _sys_open(name, flag);
}
DEFINE_SYSCALL(open, __NR_open, (void *)sys_open);

int sys_read(int fd, char *buffer, size_t size)
{
	return _sys_read(fd, buffer, size);
}
DEFINE_SYSCALL(read, __NR_read, (void *)sys_read);

int sys_close(int fd)
{
	return _sys_close(fd);
}
DEFINE_SYSCALL(close, __NR_close, (void *)sys_close);

int sys_seek(int fd, offset_t offset, int whence)
{
	return _sys_seek(fd, offset, whence);
}
DEFINE_SYSCALL(seek, __NR_lseek, (void *)sys_seek);

int sys_getdents(int fd, char *buf, size_t count)
{
	return _sys_getdents(fd, buf, count);
}
DEFINE_SYSCALL(getdents, __NR_getdents, (void *)sys_getdents);
DEFINE_SYSCALL(getdents64, __NR_getdents64, (void *)sys_getdents);

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
