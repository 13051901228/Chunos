#include <os/types.h>
#include <os/string.h>
#include <os/mutex.h>
#include <os/printk.h>
#include <os/ramdisk.h>
#include <os/errno.h>
#include <os/slab.h>
#include <os/mm.h>

unsigned long __ramdisk_start;
static struct ramdisk_header *ramdisk_header = NULL;
static struct file_header *file_header = NULL;

unsigned long mount_ramdisk(void)
{
	if (!__ramdisk_start) {
		kernel_error("Can not mount ramdisk for system\n");
		goto exit;
	}

	/* get ramdisk header to check wether it is a correct ramdisk image */
	ramdisk_header = (struct ramdisk_header *)__ramdisk_start;
	if (strcmp(ramdisk_header->name, "ramdisk")) {
		ramdisk_header = NULL;
		kernel_error("Not a correct ramdisk\n");
		goto exit;
	}

	if (ramdisk_header->file_count == 0) {
		kernel_debug("No file in ramdisk\n");
	}

	kernel_info("find ramdisk : size 0x%x file_count %d\n",
			ramdisk_header->total_size,
			ramdisk_header->file_count);
	
	/* get the address of file table. */
	file_header = (struct file_header *)(__ramdisk_start + sizeof(struct ramdisk_header));
exit:
	return __ramdisk_start;
}

int ramdisk_read(struct file *file, char *buf, int size)
{
	int copy_size = size;
	unsigned char *start = (unsigned char *)file->curr;

	if (!file)
		return 0;

	if ((u32)(start + size) > (file->base + file->size)) {
		copy_size = (unsigned char *)(file->base + file->size) - start;
	}

	if (copy_size < 0)
		copy_size = 0;

	memcpy(buf, start, copy_size);
	/* adjust file->curr to right location */
	file->curr += copy_size;
	return copy_size;
}

int ramdisk_seek(struct file *file, u32 offset)
{
	if (!file)
		return -EINVAL;

	if (file->size < offset)
		file->curr = file->base + file->size;
	else
		file->curr = file->base + offset;

	return 0;
}

struct file *ramdisk_open(char *name)
{
	int i;
	struct file *file = NULL;
	struct file_header *temp = file_header;

	/* search file though compare his name */
	for (i = 0; i < ramdisk_header->file_count; i++) {
		if(strcmp(name, temp->name) == 0) {
			file = kmalloc(sizeof(struct file), GFP_KERNEL);
			if (!file) {
				return NULL;
			}

			/* the base address of the file is offset for ramdisk_base */
			file->base = __ramdisk_start + temp->base;
			file->size = temp->size;
			file->curr = file->base;
			break;
		}

		temp++;
	}

	return file;
}

void ramdisk_close(struct file *file)
{
	if (!file)
		kfree((void *)file);
}
