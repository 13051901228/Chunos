#ifndef _FILE_H_
#define _FILE_H_

#include <os/fnode.h>
#include <os/filesystem.h>
#include <sys/stat.h>

struct task_struct;

enum {
	DT_UNKNOWN = 0,
#define DT_UNKNOWN	DT_UNKNOWN
	DT_FIFO = 1,
#define DT_FIFO		DT_FIFO
	DT_CHR = 2,
#define DT_CHR		DT_CHR
	DT_DIR = 4,
#define DT_DIR		DT_DIR
	DT_BLK = 6,
#define DT_BLK		DT_BLK
	DT_REG = 8,
#define DT_REG		DT_REG
	DT_LNK = 10,
#define DT_LNK		DT_LNK
	DT_SOCK = 12,
#define DT_SOCK		DT_SOCK
	DT_WHT = 14
#define DT_WHT		DT_WHT
#define DT_ANY		0xff
};


#define O_ACCMODE	   00000003
#define O_RDONLY	   00000000
#define O_WRONLY	   00000001
#define O_RDWR		   00000002
#define O_CREAT		   00000100
#define O_EXCL		   00000200	/* not fcntl */
#define O_NOCTTY	   00000400	/* not fcntl */
#define O_TRUNC		   00001000	/* not fcntl */
#define O_APPEND	   00002000
#define O_NONBLOCK	   00004000
#define O_NDELAY	   O_NONBLOCK
#define O_SYNC		   00010000
#define FASYNC		   00020000	/* fcntl, for BSD compatibility */
#define O_DIRECTORY	   00040000	/* must be a directory */
#define O_NOFOLLOW	   00100000	/* don't follow links */
#define O_DIRECT	   00200000	/* direct disk access hint - currently ignored */
#define O_LARGEFILE	   00400000
#define O_NOATIME	   01000000

#define F_DUPFD		0	/* dup */
#define F_GETFD		1	/* get close_on_exec */
#define F_SETFD		2	/* set/clear close_on_exec */
#define F_GETFL		3	/* get file->f_flags */
#define F_SETFL		4	/* set file->f_flags */
#define F_GETLK		5
#define F_SETLK		6
#define F_SETLKW	7

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

#define FILE_OPENS	64

#ifdef DEBUG_FS
#define fs_debug(fmt, ...) kernel_debug(fmt, ##__VA_ARGS__)
#else
#define fs_debug(fmt, ...)
#endif


struct file;

struct file_operations {
	int (*read)(struct file *file, char *buf, size_t size);
	int (*write)(struct file *file, char *buf, size_t size);
	int (*seek)(struct file *file, offset_t offset, int whence);
	int (*ioctl)(struct file *file, int cmd, void *arg);
	int (*open)(struct file *file, int flag);
	int (*close)(struct file *file);
	int (*mmap)(struct file *file, char *buffer, size_t size, offset_t offset);
	size_t (*msync)(struct file *file, char *buffer, size_t size, offset_t offset);
	int (*stat)(struct file *file, struct stat *stat);
};

typedef struct file {
	u8 f_type;			/* file type */
	u8 f_mode;			/* mode of the file FMREAD or FMWRITE */
	unsigned long f_flag;		/* flags O_* */
	struct file_operations *fops;
	struct fnode *fnode;
	offset_t offset;
	void *private_data;
} dir;

struct file_desc {
	int open_nr;
	int fd;
	struct file *file_open[FILE_OPENS];
	struct fnode *cdir;
	char cdir_name[256];
};

struct file_desc *alloc_and_init_file_desc(struct file_desc *p);
struct file *kernel_open(char *name, int flag);
size_t kernel_read(struct file *file, char *buffer, size_t size);
size_t kernel_write(struct file *file, char *buffer, size_t size);
size_t kernel_ioctl(struct file *file, int cmd, void *arg);
int kernel_seek(struct file *file, offset_t offset, int whence);
int kernel_close(struct file *file);
int _sys_open(char *name, int flag);
int _sys_write(int fd, char *buffer, size_t size);
int _sys_read(int fd, char *buffer, size_t size);
int _sys_seek(int fd, offset_t offset, int whence);
int _sys_ioctl(int fd, u32 cmd, void *arg);
int _sys_access(char *name, int flag);
size_t fmsync(int fd, char *buffer, size_t size, offset_t off);
size_t fmmap(int fd, char *buffer, size_t size, offset_t offset);
void release_task_file_desc(struct task_struct *task);

#endif
