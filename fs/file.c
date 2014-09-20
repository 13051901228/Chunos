#include <os/kernel.h>
#include <os/file.h>
#include <os/mount.h>
#include <os/mm.h>
#include <os/slab.h>
#include <os/errno.h>
#include <os/string.h>
#include <os/task.h>
#include <os/sched.h>
#include <os/fnode.h>
#include <os/vfs.h>
#include <sys/stat.h>
#include <os/device.h>

extern struct file_operations vfs_ops;

int check_permission(struct fnode *fnode, int flag)
{
	return 0;
}

int check_flag(struct fnode *fnode, int flag)
{
	return 0;
}

static void release_file(struct file *file)
{
	if (file && file->fops && file->fops->close)
		file->fops->close(file);
}

struct file *__sys_open(char *name, int flag)
{
	struct file *file;
	struct fnode *fnode;

	fnode = get_file_fnode(name, flag);
	if (!fnode)
		return NULL;

	if (check_permission(fnode, flag)) {
		release_fnode(fnode);
		return NULL;
	}

	file = kzalloc(sizeof(struct file), GFP_KERNEL);
	if (!file) {
		release_fnode(fnode);
		return NULL;
	}

	file->fnode = fnode;
	file->offset = 0;
	file->f_type = fnode->type;
	file->f_flag = flag;
	file->private_data = NULL;

	if (file->f_type == DT_CHR)
		file->fops = get_device_fops(file);
	else
		file->fops = &vfs_ops;
	
	if (file->fops && file->fops->open) {
		if (file->fops->open(file, flag)) {
			release_file(file);
			return NULL;
		}
	}

	return file;
}

/*
 * flag : O_RDONLY O_WRONLY O_RDWR O_NOINHERIT O_DENYALL
 *	  O_DENYWRITE O_DENYREAD O_DENYNONE O_APPEND O_CREAT
 *	  O_TRUNC O_EXCL O_BINARY O_TEXT
 * mode : S_IWRITE S_IREAD
 * more detail please refer:
 * http://www.cnblogs.com/leaven/archive/2010/05/26/1744274.html
 */
int _sys_open(char *name, int flag)
{
	struct file *file;
	int fd;
	struct file_desc *fdes = current->file_desc;

	/*
	 * stdin 0 stdout 1 stderr 2
	 */
	for (fd = 0; fd < FILE_OPENS; fd++) {
		if (!fdes->file_open[fd])
			break;
	}
	if (fd == FILE_OPENS)
		return -EMFILE;

	file = __sys_open(name, flag);
	if (!file)
		return -ENOENT;

	fdes->file_open[fd] = file;
	fdes->open_nr++;

	return fd;
}

static inline struct file *fd_to_file(int fd)
{
	struct file_desc *fdes = current->file_desc;
	struct file *file;

	if (fd > FILE_OPENS || !fdes->file_open[fd])
		return NULL;

	file = fdes->file_open[fd];

	return file;
}

int _sys_write(int fd, char *buffer, size_t size)
{
	struct file *file = fd_to_file(fd);

	if (size <= 0 || !file)
		return -EINVAL;

	return file->fops->write(file, buffer, size);
}

int _sys_read(int fd, char *buffer, size_t size)
{
	struct file *file = fd_to_file(fd);

	if (size <= 0 || !file)
		return -EINVAL;

	return file->fops->read(file, buffer, size);
}

int _sys_seek(int fd, offset_t offset, int whence)
{
	struct file *file = fd_to_file(fd);

	if (offset < 0 || !file)
		return -EINVAL;

	return file->fops->seek(file, offset, whence);
}

int _sys_ioctl(int fd, u32 cmd, void *arg)
{
	struct file *file = fd_to_file(fd);

	if (!file)
		return -ENOENT;

	return file->fops->ioctl(file, cmd, arg);
}

int _sys_close(int fd)
{
	struct file_desc *fdes = current->file_desc;
	struct file *file;

	if (fd > FILE_OPENS || !fdes->file_open[fd])
		return -ENOENT;

	file = fdes->file_open[fd];
	release_file(file);
	fdes->file_open[fd] = NULL;
	fdes->open_nr--;

	return 0;
}

int _sys_getdents(int fd, char *buf, int count)
{
	struct file *file = fd_to_file(fd);

	if (!file)
		return -ENOENT;

	return vfs_getdents(file, buf, count);
}

struct file_desc *alloc_and_init_file_desc(void)
{
	struct file_desc *fd;

	fd = kzalloc(sizeof(struct file_desc), GFP_KERNEL);
	if (!fd)
		return NULL;

	fd->file_open[0] = __sys_open("/dev/ttyS0", O_RDWR);
	fd->file_open[1] = __sys_open("/dev/ttyS0", O_RDWR);
	fd->file_open[2] = __sys_open("/dev/ttyS0", O_RDWR);
	fd->open_nr = 3;

	return fd;
}

void release_task_file_desc(struct task_struct *task)
{
	struct file_desc *fd = task->file_desc;
	int i;
	struct file *file;

	if (!fd)
		return;

	for (i = 0; i < FILE_OPENS; i++) {
		file = fd->file_open[i];
		if (file)
			release_file(file);
	}

	kfree(fd);
}

struct file inline *kernel_open(char *name, int flag)
{
	return __sys_open(name, flag);
}

size_t inline kernel_read(struct file *file, char *buffer, size_t size)
{
	return file->fops->read(file, buffer, size);
}

size_t inline kernel_write(struct file *file, char *buffer, size_t size)
{
	return file->fops->write(file, buffer, size);
}

size_t inline kernel_ioctl(struct file *file, int cmd, void *arg)
{
	return file->fops->ioctl(file, cmd, arg);
}

int inline kernel_seek(struct file *file, offset_t offset, int whence)
{
	return file->fops->seek(file, offset, whence);
}

int inline kernel_close(struct file *file)
{
	return file->fops->close(file);
}

int _sys_fstat(int fd, struct stat *stat)
{
	struct file *file = fd_to_file(fd);

	if (!file)
		return -ENOENT;

	return vfs_stat(file, stat);
}

int _sys_stat(char *path, struct stat *stat)
{
	struct file *file = NULL;
	int ret = 0;
	struct fnode *fnode;

	file = kernel_open(path, O_RDONLY);
	if (!file)
		return -ENOENT;
	fnode = file->fnode;

	stat->st_dev = fnode->dev;
	stat->st_ino = (unsigned long)fnode;
	stat->st_mode = fnode->mode;
	stat->st_nlink = fnode->nlinks;
	stat->st_uid = fnode->uid;
	stat->st_gid = fnode->gid;
	stat->st_rdev = 0;
	stat->st_size = fnode->data_size;
	stat->st_blksize = fnode->blk_size;
	stat->st_blocks = fnode->blk_cnt;
	stat->st_atime = fnode->atime;
	stat->st_mtime = fnode->mtime;
	stat->st_atime_nsec = 0;
	stat->st_mtime_nsec = 0;
	stat->st_ctime = fnode->ctime;
	stat->st_ctime_nsec = 0;

	if (file->fops->stat)
		file->fops->stat(file, stat);

	kernel_close(file);

	return ret;
}

int _sys_access(char *name, int flag)
{
	struct fnode *fnode;

	/* check wether this file is exist */
	fnode = get_file_fnode(name, flag);
	if (!fnode)
		return -ENODEV;

	/* check wether the usr has the access, TBC */

	release_fnode(fnode);

	return 0;
}

size_t fmmap(int fd, char *buffer,
	     size_t size, offset_t off)
{
	struct file *file = fd_to_file(fd);

	if (!file)
		return -ENOENT;

	return file->fops->mmap(file, buffer, size, off);
}

size_t fmsync(int fd, char *buffer, size_t size, offset_t off)
{
	struct file *file = fd_to_file(fd);

	if (!file)
		return -ENOENT;

	return file->fops->msync(file, buffer, size, off);
}
