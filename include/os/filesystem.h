#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_

#include <os/kernel.h>
#include <os/mutex.h>
#include <sys/dirent.h>

struct fnode;
struct fnode_ops;

struct super_block {
	struct list_head list;				/* link to all super block in the system */
	dev_t dev;					/* device type */
	unsigned char dir;				/* if the superblock is dirty */
	unsigned char block_size_bits;			/* block size contains how many bits */
	unsigned short block_size;			/* block size */
	size_t max_file;				/* max file count */
	struct filesystem *filesystem;			/* filesystem type */	
	unsigned long flags;				/* flags */
	unsigned long magic;				/* maigic number of the super block if has */
	struct fnode *root_fnode;			/* fnode of root directory */
	struct mutex mutex;				/* mutex for the super block */
	struct list_head fnodes;			/* all opened fnode is link to this list */
	struct bdev *bdev;				/* block device driver, contain the read_sector and write_sector */
	u8 state;
	void *private_data;				/* this field filled by file system */
};

struct filesystem_ops {
	struct fnode *(*get_root_fnode)(struct super_block *sb);
	u32 (*get_data_block)(struct fnode *fnode, int whence);
	int (*find_file)(char *name, struct fnode *fnode, char *buffer);
	int (*copy_fnode)(struct fnode *fnode, struct fnode *parent);
	int (*get_block)(struct fnode *fnode);
	int (*free_block)(struct fnode *fnode, block_t block);
	int (*read_block)(struct super_block *sb, char *buffer, block_t block);
	int (*write_block)(struct super_block *sb, char *buffer, block_t block);
	int (*parse_name)(char *name);
	size_t (*getdents)(struct fnode *fnode, struct dirent *dent, char *data);
	int (*fill_super)(struct bdev *bdev, struct filesystem *fs,
			  struct super_block *sb);
	int (*write_superblock)(struct super_block *sb);
	int (*write_fnode)(struct fnode *fnode);
	int (*release_fnode)(struct fnode *fnode);
};

#define FILE_SYSTEM_NAME_SIZE		32
struct filesystem {
	char name[FILE_SYSTEM_NAME_SIZE + 1];
	struct list_head list;
	struct filesystem_ops		*fops;
	int flag;
};

#define FS_FOPS_NEXT_BLOCK	0x00000001
#define FS_FOPS_PREV_BLOCK	0x00000002

struct filesystem *lookup_filesystem(char *name);
int register_filesystem(struct filesystem *filesystem);
int update_super_block(struct super_block *sb);

#endif
