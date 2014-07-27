/*
 * include/os/devfs.h
 *
 * Create by Le Min at 07/27/2014
 *
 */

#ifndef _DEVFS_H_
#define _DEVFS_H_

struct devfs_node {
	struct list_head child;
	struct list_head list;
	int nr_child;
	dev_t dev;
	u32 create_ymd;
	u32 create_hms;
	u8 type;
	char name[31];			/* no terminal */
} __attribute__((packed));

int init_devfs_node(struct devfs_node *dnode);

int devfs_create_file(struct devfs_node *p,
		struct devfs_node *dnode);

int devfs_create_dir(struct devfs_node *p,
		struct devfs_node *dnode);

struct devfs_node *create_devfs_dir_node(char *name);

#endif
