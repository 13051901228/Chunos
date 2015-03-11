#include <os/kernel.h>
#include <os/mmap.h>
#include <os/task.h>
#include <os/sched.h>
#include <os/file.h>
#include <os/task_mm.h>

void *os_mmap(void *start, size_t length, int prot,
		int flags, int fd, offset_t offset)
{
	struct task_struct *task = current;

	kernel_debug("%s %x %x %x %x %x %x\n", __func__,
			(unsigned long)start, length,
			prot, flags, fd, offset);

	if (!length)
		return NULL;
	
	return (void *)task_mm_mmap(&task->mm_struct,
			(unsigned long)start,
			length, flags, fd, offset);
}

static int __os_munmap(unsigned long addr, size_t length,
		int flags, int sync)
{
	if (!addr)
		return -EINVAL;

	if (!is_aligin(addr, PAGE_SIZE))
		return -EINVAL;

	return task_mm_munmap(&current->mm_struct,
			addr, length, flags, sync);
}

int os_munmap(void *addr, size_t length)
{
	return __os_munmap((unsigned long)addr, length, 0, 0);
}

int os_msync(void *addr, size_t length, int flags)
{
	return __os_munmap((unsigned long)addr, length, flags, 1);
}
