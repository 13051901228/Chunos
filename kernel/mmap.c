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
	struct file *file = NULL;
	int i;
	offset_t off = offset;
	size_t size;

	kernel_debug("%s %x %x %x %x %x %x\n", __func__, (u32)start,
			length, prot, flags, fd, offset);

	if (!length)
		return NULL;
	
	/* should be > 3? */
	if (fd >= 0) {
		file = task->file_desc->file_open[fd];
		if (!file) {
			kernel_error("file %d is not open\n", fd);
			return NULL;
		}
	}

	pages = page_nr(length);

	if (start) {
		if (!is_aligin((unsigned long)start, PAGE_SIZE)) {
			kernel_error("mmap: start size is not page aligin\n");
			return NULL;
		}

		/* check wether this addr is vaild now, TBD */
	} else {
		/* get the current vaild user space's address to map */
		__start = (unsigned long)get_task_user_mm_free_base(task, pages);
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

		if (mmap(task, tmp, (unsigned long)buffer, flags)) {
			kfree(buffer);
			goto out;
		}

		/* if the fd is required and opened , we read the data */
		if (file) {
			size = length > PAGE_SIZE ? PAGE_SIZE : length;
			if (!file->fops->mmap(file, buffer, size, off))
				goto out;

			length -= size;
			off += size;
		}

		tmp += PAGE_SIZE;
	}

	return ((void *)__start);

out:
	/* release the buffer which is arealdy maped */
	munmap(task, __start, pages);
	return NULL;
}

int os_munmap(void *addr, size_t length)
{
	kernel_debug("munmap addr is 0x%x, length is %d\n", (u32)addr, length);
	if (!is_aligin((unsigned long)addr, PAGE_SIZE))
		return -EINVAL;

	return munmap(current, (unsigned long)addr, page_nr(length));
}
