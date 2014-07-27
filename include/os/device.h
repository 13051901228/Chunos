/*
 * include/os/device.h
 *
 * Create by Le Min at 07/26/2014
 *
 */

#ifndef _DEVICE_CLASS_H_
#define _DEVICE_CLASS_H_

#include <os/file.h>
#include <os/devfs.h>

#define MAJOR(dev)		(dev >> 16)
#define MINOR(dev)		(dev & 0x0000ffff)
#define MKDEV(major, minor)	((major << 16) | minor)

#define MAX_MAJOR	256

struct device_class {
	char name[32];
	int major;
	bool is_char;
	struct file_operations *fops;
	struct list_head device;
	int device_nr;
	struct mutex class_mutex;
};

struct device {
	dev_t dev;
	char name[32];
	struct device* parent;
	struct devfs_node *pnode;
	struct devfs_node dnode;
	struct device_class *class;
	struct list_head list;
	void *pdata;
};

struct device_class *allocate_device_class(int major);
int register_chardev_class(struct device_class *class);
int register_blockdev_class(struct device_class *class);
struct device *alloc_device(int major, int minor);
int register_device(struct device *device);
int unregister_device(struct device *device);
int unregister_device_class(struct device_class *class);
struct file_operations *get_device_fops(struct file *file);

#endif
