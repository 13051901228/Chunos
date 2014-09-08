#include <os/kernel.h>
#include <os/mmap.h>
#include <os/task.h>
#include <os/sched.h>
#include <os/file.h>

void *os_mmap(void *start, size_t length, int prot,
		int flags, int fd, offset_t offset)
{
	struct task_struct *task = current;
	unsigned long __start = 0;
	unsigned long tmp;
	int pages = 0;
	char *buffer;
	int i;
	offset_t off = offset;
	size_t size;

	kernel_debug("%s %x %x %x %x %x %x\n", __func__, (u32)start,
			length, prot, flags, fd, offset);

	if (!length)
		return NULL;
	
	if (fd < 0)
		fd = 0xff;

	pages = page_nr(length);

	if (start) {
		if (!is_aligin((unsigned long)start, PAGE_SIZE)) {
			kernel_error("mmap: start size is not page aligin\n");
			return NULL;
		}

		/* check wether this addr is vaild now, TBD */
	} else {
		/* get the current vaild user space's address to map */
		__start = get_mmap_user_base(task, pages);
		if (!__start) {
			kernel_error("No more user memeory space for mmap\n");
			return NULL;
		}
	}

	/* begin to map */
	tmp = __start;
	for (i = 0; i < pages; i++) {
		buffer = get_free_page(GFP_KERNEL);
		if (!buffer)
			goto out;

		if (mmap(task, tmp, (unsigned long)buffer, flags, fd, off))
			goto out;

		/* if the fd is required and opened , we read the data */
		if (fd != 0xff) {
			size = length > PAGE_SIZE ? PAGE_SIZE : length;
			if (fmmap(fd, buffer, size, off) != size)
				goto out;
			length -= size;
			off += size;
		}

		tmp += PAGE_SIZE;
	}

	return ((void *)__start);

out:
	/* release the buffer which is arealdy maped */
	os_munmap((void *)__start, tmp - __start);
	return NULL;
}

static int __os_munmap(unsigned long addr, size_t length,
		int flags, int sync)
{
	if (!addr)
		return -EINVAL;

	if (!is_aligin(addr, PAGE_SIZE))
		return -EINVAL;

	return munmap(current, addr, length, flags, sync);
}

int os_munmap(void *addr, size_t length)
{
	return __os_munmap((unsigned long)addr, length, 0, 0);
}

int os_msync(void *addr, size_t length, int flags)
{
	return __os_munmap((unsigned long)addr, length, flags, 1);
}
