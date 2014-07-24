#ifndef _INODE_H
#define _INODE_H

#include <os/filesystem.h>
#include <os/types.h>

struct fnode {
	u8 mode;				/* attaribute of fnode */
	u8 state;
	u16 nlinks;
	dev_t dev;
	gid_t gid;
	uid_t uid;
	time_t atime;
	time_t mtime;
	time_t ctime;
	size_t blk_cnt;
	size_t blk_size;
	unsigned long flags;
	u8 type;
	u16 fnode_size;
	u16 psize;
	size_t data_size;
	size_t buffer_size;
	s32 rw_pos;
	struct super_block *sb;
	char *data_buffer;
	u32 current_block;
	struct mutex mutex;
	void *private_data;				/* private data of different file sytem */
	u32 p[0];
};

static inline struct filesystem *fnode_get_filesystem(struct fnode *fnode)
{
	return fnode->sb->filesystem;
}

static inline void *fnode_get_private(struct fnode *fnode)
{
	return fnode->private_data;
}

static inline void fnode_set_private(struct fnode *fnode, void *arg)
{
	if (fnode && arg)
		fnode->private_data = arg;
}

struct fnode *get_file_fnode(char *name, int flag);
void release_fnode(struct fnode *fnode);
int update_fnode(struct fnode *fnode);
char *get_iname_buffer(void);
void put_iname_buffer(char *buf);
struct fnode *allocate_fnode(size_t psize);

#endif
