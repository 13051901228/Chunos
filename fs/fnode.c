#include <os/kernel.h>
#include <os/file.h>
#include <os/mount.h>
#include <os/fnode.h>
#include <os/spin_lock.h>
#include <os/init.h>
#include <os/vfs.h>

#define FS_UPDATE_INODE		0
#define FS_UPDATE_SB		1

#define INAME_SIZE		(512)
#define INAME_BUFFER_SIZE	(1 * PAGE_SIZE)
#define INAME_NR		(INAME_BUFFER_SIZE / INAME_SIZE)
static char *iname_buffer_base;
spin_lock_t iname_buffer_lock;

DECLARE_BITMAP(iname_buffer, INAME_NR);

int init_iname_buffer(void)
{
	kernel_info("init fnode name buffer\n");

	iname_buffer_base =
		get_free_pages(page_nr(INAME_BUFFER_SIZE), GFP_KERNEL);
	if (!iname_buffer_base) {
		kernel_error("no memory size for iname buffer\b");
		return -ENOMEM;
	}

	spin_lock_init(&iname_buffer_lock);
	init_bitmap(iname_buffer, INAME_NR);

	return 0;
}
fs_initcall(init_iname_buffer);

char *get_iname_buffer(void)
{
	char *ret = NULL;
	int i;

	spin_lock(&iname_buffer_lock);
	for (i = 0; i < INAME_NR; i++) {
		if (!read_bit(iname_buffer, i)) {
			set_bit(iname_buffer, i);
			ret = iname_buffer_base + INAME_SIZE * i;
			goto out;
		}
	}

out:
	spin_unlock(&iname_buffer_lock);
	return ret;
}

void put_iname_buffer(char *buf)
{
	int i;

	if (!is_aligin(buf - iname_buffer_base, INAME_SIZE))
		kernel_warning("may not a iname buffer\n");

	i = (buf - iname_buffer_base) / INAME_SIZE;

	printk("put i is %d\n", i);
	spin_lock(&iname_buffer_lock);
	if (!read_bit(iname_buffer, i)) {
		kernel_error("iname buffer already released\n");
		goto out;
	}
	clear_bit(iname_buffer, i);
out:
	spin_unlock(&iname_buffer_lock);
}

static struct fnode *__sys_creat(char *name, struct fnode *root, int flag)
{
	/* To be implement */
	return NULL;
}

int update_fnode(struct fnode *fnode)
{
	return 0;
}

char *find_file_name(char *org_name, char *ret_name)
{
	char *pos;

	if ((!org_name) || (!ret_name) || (*org_name == 0))
		return NULL;

	pos = strchr(org_name, '/');
	if (pos == NULL) {
		/* the last one, so this is a file name */
		strcpy(ret_name, org_name);
		pos = org_name + strlen(org_name) - 1;
	}
	else {
		/* these are dir name */
		strncpy(ret_name, org_name, pos - org_name);
		ret_name[pos - org_name ] = 0;
	}
	
	return (pos + 1);
}

static int check_file_name(char *name, char type)
{
	int len = 0;

	if (!name)
		return -EINVAL;

	len = strlen(name);
	if (len == 0)
		return -EINVAL;

	if (*name == '.') {
		if (name[1] != '/')
			return -EINVAL;
	}

	if (type == DT_BLK) {
		if (*(name + len) == '/')
			return -EINVAL;
	}

	return 0;
}

struct fnode *__allocate_fnode(size_t psize)
{
	struct fnode *fnode = NULL;

	fnode = kzalloc(sizeof(struct fnode) + psize, GFP_KERNEL);
	if (!fnode)
		return NULL;

	if (psize)
		fnode->private_data = (void *)fnode->p;

	init_mutex(&fnode->mutex);

	return fnode;
}

static inline void __release_fnode(struct fnode *fnode)
{
	kfree(fnode);
}

void release_fnode(struct fnode *fnode)
{
	if (fnode->data_buffer)
		kfree(fnode->data_buffer);

	__release_fnode(fnode);
}

struct fnode *allocate_fnode(size_t psize)
{
	return __allocate_fnode(psize);	
}

static int copy_fnode(struct fnode *fnode, struct fnode *father)
{
	int page_nr = 0;
	struct filesystem *fs = fnode_get_filesystem(father);
	int ret = 0;

	/* TBC */
	fnode->sb = father->sb;
	fnode->psize = father->psize;
	fnode->data_size = father->data_size;
	fnode->buffer_size = father->buffer_size;
	fnode->blk_cnt = father->blk_cnt;
	fnode->mode = father->mode;
	fnode->state = 0;
	fnode->nlinks = 0;
	fnode->type = father->type;

	if (father->buffer_size != 0) {
		if (father->buffer_size < 3 * 512 ) {
			fnode->data_buffer = kzalloc(father->buffer_size, GFP_KERNEL);
		} else {
			page_nr = page_nr(father->buffer_size);
			fnode->data_buffer = get_free_pages(page_nr, GFP_KERNEL);
		}

		if (!fnode->data_buffer)
			return -ENOMEM;
	}

	if (fs->fops->copy_fnode)
		ret = fs->fops->copy_fnode(fnode, father);

	return ret;
}

static int __get_file_fnode(char *name, struct fnode *fnode, char **r_name, int flag)
{
	char *tname;
	char *fname = name;
	int type, file_find = 0;
	struct filesystem *fs = fnode_get_filesystem(fnode);
	struct super_block *sb = fnode->sb;
	char *buffer = fnode->data_buffer;
	int err = 0;
	u32 block;

	tname = get_iname_buffer();
	if (!tname)
		return -EAGAIN;

	/*
	 * fnode contains the father fnode's information, and
	 * fs also need to update it when find a file, then it
	 * will used as a father fnode passed to next search
	 */
	while (fname = find_file_name(fname, tname)) {
		fs_debug("fname is %s tname is %s\n", fname, tname);
		block = fs->fops->get_data_block(fnode, VFS_START_BLOCK);
		if (!block)
			return -ENOSPC;

		if (fs->fops->parse_name) {
			if (fs->fops->parse_name(tname))
				kernel_error("parse name failed\n");
		}

		if (*fname == 0) {
			/* if reach the last file, need to know wether it
			 * needed to open a dir */
			if ((flag & O_DIRECTORY) == O_DIRECTORY)
				type = DT_DIR;
			else
				type = DT_ANY;
		} else {
			type = DT_DIR;
		}

		fs_debug("open file type is %d\n", type);

		do {
			err = fs->fops->read_block(sb, buffer, block);
			if (err)
				return -EIO;

			if (!fs->fops->find_file(tname, fnode, buffer)) {
				fs_debug("fnode->type:%x type:%x\n", fnode->type, type);
				if ((fnode->type & type) != fnode->type)
					err = -ENOTDIR;

				file_find = 1;
				break;
			}

			block = fs->fops->get_data_block(fnode, VFS_NEXT_BLOCK);
		} while (block);

		fs_debug("err:0x%x file_find:%d\n", err, file_find);

		if (err)
			break;

		if (!file_find) {
			err = -ENOENT;
			break;
		}
	}

	*r_name = fname;
	put_iname_buffer(tname);

	return err;;
}

static struct fnode *_get_file_fnode(char *name, struct fnode *father, int flag)
{
	struct fnode *fnode;
	int ret = 0;
	char *rname;

	/* create a new fnode and copy the father's data to new */
	fnode = allocate_fnode(father->psize);
	if (!fnode)
		return NULL;

	if (copy_fnode(fnode, father)) {
		release_fnode(fnode);
		return NULL;
	}

	/* just return the root fnode */
	if (*name == 0)
		goto out;

	ret = __get_file_fnode(name, fnode, &rname, flag);
	if (ret == -ENOENT) {
		if (O_CREAT & flag)
			fnode = __sys_creat(rname, fnode, flag);
	}

out:
	return fnode;
}

struct fnode *get_file_fnode(char *file_name, int flag)
{
	struct mount_point *mnt;
	char *real_name;
	struct fnode *root_fnode = NULL;
	char type;

	if (flag & O_DIRECTORY)
		type = DT_DIR;
	else
		type = DT_BLK;

	if (check_file_name(file_name, type))
		return NULL;

	/*
	 * if the file path is a abs path, find his mount point first
	 * then get the fnode of the file, if open file is in current
	 * directory, get the sb and fs directory.
	 */
	if (*file_name == '/') {
		mnt = get_mount_point(file_name);
		if (!mnt) {
			kernel_error("can not find mount ponint\n");
			return NULL;
		}
		real_name = get_file_fs_name(mnt, file_name);
		if (!real_name)
			return NULL;
		kernel_debug("find mount point %s real_name %s\n",
				mnt->path, real_name);
		root_fnode = mnt->sb->root_fnode;
	}
	else {
		real_name = file_name;
		//root_fnode = current->cdir->file.fnode;
	}

	/* skip '/' at the head of the file path */
	if (*real_name == '.')
		real_name++;
	while(*real_name == '/')
		real_name++;

	return _get_file_fnode(real_name, root_fnode, flag);
}
