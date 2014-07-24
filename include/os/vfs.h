#ifndef _VFS_H_
#define _VFS_H_

#define VFS_NEXT_BLOCK		0
#define VFS_PREV_BLOCK		1
#define VFS_START_BLOCK		2
#define VFS_END_BLOCK		3
#define VFS_CURRENT_BLOCK	4

#include <sys/stat.h>
#include <os/file.h>

int vfs_getdents(struct file *file, char *buffer, size_t count);
int vfs_stat(struct file *file, struct stat *stat);

#endif
