/*
 * fs/devfs.c
 *
 * Create by Le Min at 07/26/2014
 *
 */

#include <os/kernel.h>
#include <os/filesystem.h>
#include <os/vfs.h>
#include <os/fnode.h>
#include <os/disk.h>
#include <os/devfs.h>
#include <os/device.h>

static struct devfs_node root;

int init_devfs_node(struct devfs_node *dnode)
{
	struct device *device;

	device = container_of(dnode, struct device, dnode);

	init_list(&dnode->list);
	init_list(&dnode->child);
	dnode->nr_child = 0;
	dnode->dev = device->dev;
	dnode->type = DT_CHR;
	memcpy(dnode->name, device->name, 31);

	return 0;
}

struct devfs_node *create_devfs_dir_node(char *name)
{
	struct devfs_node *node;

	node = kzalloc(sizeof(struct devfs_node), GFP_KERNEL);
	if (!node)
		return NULL;

	init_list(&node->list);
	init_list(&node->child);
	node->nr_child = 0;
	node->dev = 0;
	node->type = DT_DIR;
	memcpy(node->name, name, 31);
	
	return node;
}

static int devfs_create_entry(struct devfs_node *p,
			struct devfs_node *dnode, int dir)
{
	unsigned long flags;

	if (p == NULL)
		p = &root;

	if (dir) {
		if (p->type != DT_DIR)
			return -EINVAL;
	}

	lock_kernel_irqsave(&flags);
	list_add_tail(&p->child, &dnode->list);
	p->nr_child++;
	unlock_kernel_irqstore(&flags);

	return 0;
}

int devfs_create_file(struct devfs_node *p,
		struct devfs_node *dnode)
{
	return devfs_create_entry(p, dnode, 0);
}

int devfs_create_dir(struct devfs_node *p, struct devfs_node *dnode)
{
	return devfs_create_entry(p, dnode, 1);
}

static int devfs_fill_fnode(struct fnode *fnode, struct devfs_node *dnode)
{
	fnode->dev = dnode->dev;
	fnode->atime = 0;
	fnode->mtime = 0;
	fnode->ctime = 0;
	fnode->blk_cnt = 1;
	fnode->blk_size = sizeof(struct devfs_node);
	
	memcpy(fnode->private_data, dnode, sizeof(struct devfs_node));
	
	return 0;
}

static int devfs_find_file(char *name, struct fnode *fnode, char *buffer)
{
	struct list_head *list = (struct list_head *)buffer;
	struct devfs_node *tmp;

	tmp = list_entry(list, struct devfs_node, list);
	if (!strncmp(name, tmp->name, strlen(name))) {
		return devfs_fill_fnode(fnode, tmp);
	}

	return -ENOENT;
}

static struct fnode *devfs_get_root_fnode(struct super_block *sb)
{
	struct fnode *fnode;

	fnode = allocate_fnode(sizeof(struct devfs_node));
	if (!fnode)
		return NULL;

	fnode->fnode_size = sizeof(struct devfs_node);
	fnode->sb = sb;
	fnode->psize = sizeof(struct devfs_node);
	fnode->buffer_size = sizeof(struct list_head);

	/* copy root devfs_node */
	memcpy(fnode->private_data, &root, sizeof(struct devfs_node));
}

static u32 devfs_get_data_block(struct fnode *fnode, int whence)
{
	struct devfs_node *dnode = fnode->private_data;
	struct list_head *list = (struct list_head *)fnode->data_buffer;

	switch (whence) {
	case VFS_START_BLOCK:
		list = list_next(&dnode->child);
		break;

	case VFS_NEXT_BLOCK:
		list = list_next(list);
		break;
	}

	if (list == list_next(list))
		return 0;

	return (u32)list;
}

static int devfs_copy_fnode(struct fnode *fnode, struct fnode *parent)
{
	memcpy(fnode->private_data, parent->private_data,
			sizeof(struct devfs_node));

	return 0;
}

int devfs_fill_super(struct bdev *bdev,
		     struct filesystem *fs,
		     struct super_block *sb)
{
	sb->private_data = &root;
	sb->filesystem = fs;
	sb->bdev = bdev;

	return 0;
}

static size_t devfs_getdents(struct fnode *fnode,
		struct dirent *dent, char *data)
{
	struct list_head *list = (struct list_head *)data;
	struct devfs_node *dnode;

	if (list != (&dnode->child)) {
		dnode = list_entry(list, struct devfs_node, list);
		dent->d_type = dnode->type;
		strcpy(dent->name, dnode->name);
		return sizeof(struct devfs_node);
	}

	return 0;
}

static int devfs_write_sb(struct super_block *sb)
{
	return 0;
}

static int devfs_read_block(struct super_block *sb,
		char *buffer, block_t block)
{
	return 0;
}

static int devfs_write_block(struct super_block *sb,
		char *buffer, block_t block)
{
	return 0;
}

struct filesystem_ops devfs_ops = {
	.find_file		= devfs_find_file,
	.get_data_block		= devfs_get_data_block,
	.copy_fnode		= devfs_copy_fnode,
	.get_root_fnode		= devfs_get_root_fnode,
	.fill_super		= devfs_fill_super,
	.write_superblock	= devfs_write_sb,
	.getdents		= devfs_getdents,
	.read_block		= devfs_read_block,
	.write_block		= devfs_write_block,
};

#define DEVFS_FS_FLAG	FS_FOPS_NEXT_BLOCK

struct filesystem devfs = {
	.name = "devfs",
	.fops = &devfs_ops,
	.flag = DEVFS_FS_FLAG,
};

static int devfs_init(void)
{
	init_list(&root.list);
	init_list(&root.child);
	root.nr_child = 0;
	root.dev = 0;
	root.type = DT_DIR;
	memset(root.name, 0, 31);

	return register_filesystem(&devfs);
}

fs_initcall(devfs_init);
