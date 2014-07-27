/*
 * driver/device.c
 * 
 * Created by Le Min at 07/24/2014
 *
 */

#include <os/kernel.h>
#include <os/types.h>
#include <os/device.h>
#include <os/devfs.h>
#include <os/file.h>
#include <os/fnode.h>

static struct device_class *ctable[MAX_MAJOR];

struct file_operations *get_device_fops(struct file *file)
{
	int major = MAJOR(file->fnode->dev);

	return ctable[major]->fops;
}

struct device_class *allocate_device_class(int major)
{
	struct device_class *class;

	if (major > (MAX_MAJOR - 1) || major < 0)
		return NULL;

	class = kzalloc(sizeof(struct device_class), GFP_KERNEL);
	if (!class)
		return NULL;

	class->major = major;
	class->fops = NULL;
	init_list(&class->device);
	class->device_nr = 0;
	init_mutex(&class->class_mutex);

	return class;
}

static int register_device_class(struct device_class *class)
{
	int err = 0;
	unsigned long flags;

	lock_kernel_irqsave(&flags);
	if (ctable[class->major]) {
		kernel_error("device class arealdy registereda	\
			major:%d\n", class->major);
		err = -EINVAL;
		goto out;
	}

	ctable[class->major] = class;
out:
	unlock_kernel_irqstore(&flags);
	return err;
}

int unregister_device_class(struct device_class *class)
{
	int err = 0;

	if (!class)
		return -EINVAL;

	lock_kernel();
	if (!ctable[class->major]) {
		err = -ENODEV;
		goto out;
	}
	
	ctable[class->major] = NULL;
	kfree(class);
out:
	unlock_kernel();
	return err;
}

int register_chardev_class(struct device_class *class)
{
	class->is_char = 1;

	return register_device_class(class);
}

int register_blockdev_class(struct device_class *class)
{
	class->is_char = 0;

	return register_device_class(class);
}

struct device *alloc_device(int major, int minor)
{
	struct device *device;

	device = kzalloc(sizeof(struct device), GFP_KERNEL);
	if (!device)
		return NULL;

	init_list(&device->list);
	device->parent = NULL;
	device->dev = MKDEV(major, minor);
	device->pnode = NULL;

	return device;
}

int register_device(struct device *device)
{
	int major, minor;
	struct device_class *class;
	int len, len1;
	char name[4];

	major = MAJOR(device->dev);
	minor = MINOR(device->dev);

	class = ctable[major];

	mutex_lock(&class->class_mutex);
	class->device_nr++;
	list_add(&class->device, &device->list);
	mutex_unlock(&class->class_mutex);

	device->class = class;
	len = strlen(class->name);
	len1 = num_to_str(name, minor, 10);
	memcpy(device->name, class->name, len);
	memcpy(&device->name[len], name, len1);
	
	init_devfs_node(&device->dnode);
	devfs_create_file(device->pnode, &device->dnode);

	return 0;
}

int unregister_device(struct device *device)
{
	struct device_class *class;

	if (!device)
		return -EINVAL;

	class = device->class;

	mutex_lock(&class->class_mutex);
	class->device_nr--;
	list_del(&device->list);
	mutex_unlock(&class->class_mutex);

	kfree(device);

	return 0;
}
