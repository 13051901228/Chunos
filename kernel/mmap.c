#include <os/kernel.h>
#include <os/mmap.h>
#include <os/task.h>
#include <os/sched.h>
#include <os/file.h>

void *os_mmap(void *start, size_t length, int prot,
		int flags, int fd, offset_t offset)
{
	struct task_struct *task = current;
	int pages = 0;

	kernel_debug("%s %x %x %x %x %x %x\n", __func__,
			(unsigned long)start, length,
			prot, flags, fd, offset);

	if (!length)
		return NULL;
	
	if (fd < 0)
		fd = 0xff;

	pages = page_nr(length);

	return task_mm_mmap(&task->mm_struct, start,
			pages, flags, fd, offset);
}

static int __os_munmap(unsigned long addr, size_t length,
		int flags, int sync)
{
	if (!addr)
		return -EINVAL;

	if (!is_aligin(addr, PAGE_SIZE))
		return -EINVAL;

	return task_mm_munmap(current, addr, length, flags, sync);
}

int os_munmap(void *addr, size_t length)
{
	return __os_munmap((unsigned long)addr, length, 0, 0);
}

int os_msync(void *addr, size_t length, int flags)
{
	return __os_munmap((unsigned long)addr, length, flags, 1);
}
