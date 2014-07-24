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
	kfree(file);
}

struct file_desc *alloc_and_init_file_desc(void)
{
	struct file_desc *fd;

	fd = kzalloc(sizeof(struct file_desc), GFP_KERNEL);
	if (!fd)
		return NULL;

	return fd;
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
	file->f_type = DT_BLK;
	file->f_flag = flag;
	file->private_data = NULL;
	file->fops = &vfs_ops;
	
	if (file->fops->open) {
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

int _sys_write(int fd, char *buffer, size_t size)
{
	struct file_desc *fdes = current->file_desc;
	struct file *file;

	if (fd > FILE_OPENS || !fdes->file_open[fd])
		return -ENOENT;

	if (size <= 0)
		return -EINVAL;

	file = fdes->file_open[fd];

	return file->fops->write(file, buffer, size);
}

int _sys_read(int fd, char *buffer, size_t size)
{
	struct file_desc *fdes = current->file_desc;
	struct file *file;

	if (fd > FILE_OPENS || !fdes->file_open[fd])
		return -ENOENT;

	if (size <= 0)
		return -EINVAL;

	file = fdes->file_open[fd];

	return file->fops->read(file, buffer, size);
}

int _sys_seek(int fd, offset_t offset, int whence)
{
	struct file_desc *fdes = current->file_desc;
	struct file *file;

	if (fd > FILE_OPENS || !fdes->file_open[fd])
		return -ENOENT;

	if (offset <= 0)
		return -EINVAL;

	file = fdes->file_open[fd];
	
	return file->fops->seek(file, offset, whence);
}

int _sys_ioctl(int fd, u32 cmd, void *arg)
{
	struct file_desc *fdes = current->file_desc;
	struct file *file;

	if (fd > FILE_OPENS || !fdes->file_open[fd])
		return -ENOENT;

	file = fdes->file_open[fd];

	return file->fops->ioctl(file, cmd, arg);
}

int _sys_close(int fd)
{
	struct file_desc *fdes = current->file_desc;
	struct file *file;

	if (fd > FILE_OPENS || !fdes->file_open[fd])
		return -ENOENT;

	file = fdes->file_open[fd];

	return file->fops->close(file);
}

int _sys_getdents(int fd, char *buf, int count)
{
	struct file_desc *fdes = current->file_desc;
	struct file *file;

	if (fd > FILE_OPENS || !fdes->file_open[fd])
		return -ENOENT;

	file = fdes->file_open[fd];

	return vfs_getdents(file, buf, count);
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
	struct file_desc *fdes = current->file_desc;
	struct file *file;

	if (fd > FILE_OPENS || !fdes->file_open[fd])
		return -ENOENT;

	file = fdes->file_open[fd];

	return vfs_stat(file, stat);
}

int _sys_stat(char *path, struct stat *stat)
{
	struct file *file = NULL;
	int ret;

	file = kernel_open(path, O_RDONLY);
	if (!file)
		return -ENOENT;

	ret = vfs_stat(file, stat);
	kernel_close(file);

	return ret;
}
