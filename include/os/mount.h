#ifndef _MOUNT_H
#define _MOUNT_H

#include <os/kernel.h>
#include <os/filesystem.h>

struct mount_point {
	char *path;
	struct super_block *sb;
	struct filesystem *fs;
	struct mount_point *next;
	unsigned long flag;
};

struct mount_point *get_mount_point(char *file_name);
char *get_file_fs_name(struct mount_point *mnt, char *file_name);

#endif


