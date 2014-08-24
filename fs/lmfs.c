#include <os/kernel.h>
#include <os/types.h>
#include <os/file.h>
#include <os/mount.h>
#include <os/fnode.h>
#include <os/filesystem.h>
#include <os/disk.h>
#include <os/mutex.h>
#include "lmfs.h"
#include <os/vfs.h>
#include <config/config.h>
#include <os/init.h>

#ifdef DEBUG_LMFS
#define lmfs_debug(fmt, ...) kernel_debug(fmt, ##__VA_ARGS__)
#else
#define lmfs_debug(fmt, ...)
#endif

static struct lmfs_super_block *lmfs_super;
static char *dbuffer;
DECLARE_MUTEX(dbuffer_mutex);

static int lmfs_read_block(struct super_block *sb,
		char *buffer, block_t block)
{
	struct bdev *bdev = sb->bdev;

	return read_sectors(bdev, buffer, block * 8, 8);
}

static int lmfs_write_block(struct super_block *sb,
		char *buffer, block_t block)
{
	struct bdev *bdev = sb->bdev;

	return write_sectors(bdev, buffer, block * 8, 8);
}

static struct lmfs_super_block *new_lmfs_super(void)
{
	struct lmfs_super_block *lsb = NULL;

	lsb = kzalloc(sizeof(struct lmfs_super_block), GFP_KERNEL);
	if (!lsb)
		return NULL;

	lsb->ib_pool = NULL;
	lsb->db_pool = NULL;

	return lsb;
}

static int init_lmfs_ibpool(struct super_block *sb)
{
	int err;
	struct lmfs_super_block *lsb = (struct lmfs_super_block *)sb->private_data;

	lsb->ib_pool = get_free_page(GFP_KERNEL);
	if (!lsb->ib_pool)
		return -ENOMEM;

	/* read the current fnode free block from the pool */
	err = lmfs_read_block(sb, (char *)lsb->ib_pool,
			lsb->ib_pool_current_block);
	if (err) {
		free_pages(lsb->ib_pool);
		return -EIO;
	}

	return 0;
}

static int init_lmfs_dbpool( struct super_block *sb)
{
	int err;
	struct lmfs_super_block *lsb = (struct lmfs_super_block *)sb->private_data;

	lsb->db_pool = get_free_page(GFP_KERNEL);
	if (!lsb->db_pool)
		return -ENOMEM;

	/* read the current data free block to from the pool */
	err = lmfs_read_block(sb, (char *)lsb->db_pool,
			lsb->db_pool_current_block);
	if (err) {
		free_pages(lsb->db_pool);
		return -EIO;
	}

	return 0;
}

static void fnode_location(struct super_block *sb,
		u32 index, int *i, int *j)
{
	struct lmfs_super_block *lsb = sb->private_data;

	*i = index / 8;
	*j = index % 8;
	*i += lsb->fnode_start_block;
	lmfs_debug("fnode location b:%d s:%d\n", *i, *j);
}

static int lmfs_pop_fnode(struct super_block *sb)
{
	struct lmfs_super_block *lsb = (struct lmfs_super_block *)sb->private_data;
	u32 cur_pos = lsb->ib_pool_pos;
	u32 cur_block = lsb->ib_pool_current_block;
	u32 *pool = lsb->ib_pool;
	u32 index;

	if (lsb->fnode_free == 0)
		return 0;

	index = pool[cur_pos];
	if (cur_pos == 0) {
		/* update the pool buffer and the superblock information */
		cur_pos = 1023;
		cur_block--;
		if (cur_block >= lsb->db_pool_start_block) {
			if (lmfs_read_block(sb, (char *)pool, cur_block))
				return 0;
		}
	}
	else
		cur_pos--;

	lsb->ib_pool_current_block = cur_block;
	lsb->ib_pool_pos = cur_pos;
	lsb->fnode_free--;

	return index;
}

static u32 lmfs_push_fnode(struct super_block *sb, u32 index)
{
	struct lmfs_super_block *lsb = (struct lmfs_super_block *)sb->private_data;
	u32 cur_pos = lsb->ib_pool_pos;
	u32 cur_block = lsb->ib_pool_current_block;
	u32 *pool = lsb->ib_pool;

	if (!index)
		return 0;

	cur_pos++;
	if (cur_pos == 1024) {
		/* write the full block to the device
		 * and prepare another block's data */
		cur_pos = 0;
		if (lmfs_write_block(sb, (char *)pool, cur_block))
			return -EIO;
		cur_block++;
	}
	pool[cur_pos] = index;

	lsb->ib_pool_current_block = cur_block;
	lsb->ib_pool_pos = cur_pos;
	lsb->fnode_free++;

	return 0;

}

static block_t lmfs_pop_block(struct super_block *sb)
{
	block_t block;
	struct lmfs_super_block *lsb = (struct lmfs_super_block *)sb->private_data;
	u32 cur_pos = lsb->db_pool_pos;
	block_t cur_block = lsb->db_pool_current_block;
	u32 *pool = lsb->db_pool;

	if (lsb->data_free_blocks == 0)
		return 0;

	block = pool[cur_pos];
	if (cur_pos == 0) {
		/* update the pool buffer and the superblock information */
		cur_pos = 1023;
		cur_block--;
		if (cur_block >= lsb->db_pool_start_block) {
			if (lmfs_read_block(sb, (char *)pool, cur_block))
				return 0;
		}
	}
	else
		cur_pos--;

	lsb->db_pool_current_block = cur_block;
	lsb->db_pool_pos = cur_pos;

	return block;
}

static int lmfs_push_block(struct super_block *sb, u32 block)
{
	struct lmfs_super_block *lsb = (struct lmfs_super_block *)sb->private_data;
	u32 cur_pos = lsb->db_pool_pos;
	u32 cur_block = lsb->db_pool_current_block;
	u32 *pool = lsb->db_pool;

	if (!lsb || !block)
		return -EINVAL;

	cur_pos++;
	if (cur_pos == 1024) {
		/* write the full block to the device
		 * and prepare another block's data */
		cur_pos = 0;
		if (lmfs_write_block(sb, (char *)pool, cur_block))
			return -EIO;
		cur_block++;
	}
	pool[cur_pos] = block;

	lsb->db_pool_current_block = cur_block;
	lsb->db_pool_pos = cur_pos;

	return 0;
}

static block_t lmfs_get_new_block(struct fnode *fnode)
{
	struct lmfs_fnode *lfnode = (struct lmfs_fnode *)fnode->private_data;
	struct super_block *sb = fnode->sb;
	block_t new_block;
	block_t new_level1 = 0;
	int err = 0;
	u32 pos1, pos2;

	pos1 = lfnode->pos_in_lvl0;
	pos2 = lfnode->pos_in_lvl1;
	do {
		new_block = lmfs_pop_block(sb);
		if (!new_block)
			return -ENOSPC;

		/* need to allocate a new level1 block */
		if (pos2 == 1024) {
			if (pos1 == 64) {
				new_level1 = lmfs_pop_block(sb);
				if (!new_level1) {
					goto release_new_block;
				}
				err = lmfs_write_block(sb, (char *)lfnode->lvl1, pos1);
				if (err) {
					goto release_new_level1;
				}

				pos1++;
				pos2 = 0;
				lfnode->lvl0[pos1] = new_level1;
				memset((char *)lfnode->lvl1, 0, 4096);
			}
		}

		lfnode->lvl1[pos2] = new_block;
		pos2++;
	} while (0);

	lfnode->pos_in_lvl0 = pos1;
	lfnode->pos_in_lvl1 = pos2;
	lmfs_write_block(sb, (char *)lfnode->lvl1, pos1);

	return new_block;

release_new_level1:
	lmfs_push_block(sb, new_level1);
release_new_block:
	lmfs_push_block(sb, new_block);

	return 0;
}

static int init_data_block_info(struct super_block *sb,
			    struct lmfs_fnode *lfnode,
			    struct lmfs_fnode_entry *entry)
{
	int ret;

	memcpy(&lfnode->data_block, (unsigned char *)(entry) + 256, 256);
	lfnode->pos_in_lvl0 = 0;
	lfnode->pos_in_lvl1 = 0;
	lfnode->block_count = entry->block_nr;
	lmfs_debug("### LMFS type is %d\n", entry->type);
	if (entry->type == DT_BLK ) {
		lmfs_debug("file is a file\n");
		/* this is not a dir */
		lfnode->bitmap = NULL;
		lfnode->lvl0 = lfnode->data_block;
	} else {
		lmfs_debug("file is a dir\n");
		/* this is a directory blokc_data[0-31] is for bitmap */
		lfnode->bitmap = lfnode->data_block;
		lfnode->lvl0 = &lfnode->data_block[32];
	}

	lmfs_debug("lfnode->data_block[0] 0x%x\n", lfnode->lvl0[0]);
	ret = lmfs_read_block(sb,
			(char *)lfnode->lvl1,
			lfnode->lvl0[0]);
	if (ret)
		return ret;

	lfnode->current_block = (u32)lfnode->lvl1[0];
	return 0;
}

static int lmfs_fill_fnode(struct fnode *fnode, u32 fnode_index)
{
	struct lmfs_fnode *lfnode = (struct lmfs_fnode *)fnode->private_data;
	struct super_block *sb = fnode->sb;
	int i, j, ret;
	struct lmfs_fnode_entry *fnode_entry =
		(struct lmfs_fnode_entry *)(fnode->data_buffer);

	fnode_location(sb, fnode_index, &i, &j);
	if (i == 0 && j == 0)
		return -EINVAL;

	lmfs_debug("LMFS_fill fnode fnode:%d i:%d j:%d\n", fnode_index, i, j);
	ret = lmfs_read_block(sb, (void *)fnode_entry, i);
	if (ret)
		return ret;
	fnode_entry += j;

	fnode->mode = fnode_entry->mode;
	fnode->flags = fnode_entry->flag;
	fnode->type = fnode_entry->type;
	fnode->atime = 0;
	fnode->mtime = 0;
	fnode->ctime = 0;
	fnode->blk_cnt = fnode_entry->block_nr;
	fnode->fnode_size = 512;
	fnode->sb = sb;
	fnode->state = 0;
	lmfs_debug("file size is %d\n", fnode_entry->size);
	fnode->data_size = fnode_entry->size;
	fnode->buffer_size = 4096;
	fnode->psize = sizeof(struct lmfs_fnode);

	lfnode->fnode_index = fnode_index;
	lfnode->file_nr = fnode_entry->file_nr;

	return init_data_block_info(sb, lfnode, fnode_entry);
}

static struct fnode *lmfs_get_root_fnode(struct super_block *sb)
{
	struct fnode *fnode;
	struct lmfs_fnode *lfnode;

	fnode = allocate_fnode(sizeof(struct lmfs_fnode));
	if (!fnode)
		return NULL;

	/* we do not fill many information there,
	 * since copy_fnode will do it again
	 */
	lfnode = fnode->private_data;
	fnode->fnode_size = 512;
	fnode->sb = sb;
	fnode->psize = sizeof(struct lmfs_fnode);
	fnode->buffer_size = 4096;

	lfnode->fnode_index = 0;
	
	return fnode;
}

static int lmfs_write_sb(struct super_block *sb)
{
	struct lmfs_super_block *lsb = (struct lmfs_super_block *)sb;
	block_t block = lsb->start_block;
	int ret;

	mutex_lock(&dbuffer_mutex);
	ret = lmfs_read_block(sb, (char *)dbuffer, block);
	if (ret)
		goto out;
	memcpy(dbuffer, (void *)lsb, sizeof(struct lmfs_super_block) - 8);

	ret = lmfs_write_block(sb, dbuffer, block);

out:
	mutex_unlock(&dbuffer_mutex);
	return ret;
}


static int lmfs_fill_super(struct bdev *bdev,
			   struct filesystem *fs,
			   struct super_block *sb)
{
	int err = 0;
	struct lmfs_super_block *tmp;

	dbuffer = get_free_page(GFP_KERNEL);
	if (!dbuffer)
		return -ENOMEM;

	err = read_sectors(bdev, dbuffer, 0, 8);		/*super block need to be 0*/
	if (err)
		goto out;

	tmp = (struct lmfs_super_block *)dbuffer;
	if (strncmp((char *)tmp, "LMFS", 4)) {
		kernel_error("File system is not LMFS\n");
		err = -EINVAL;
		goto out;
	}

	lmfs_super = new_lmfs_super();
	if (!lmfs_super) {
		err = -ENOMEM;
		goto out;
	}

	/* copy the information to the memory */
	memcpy(lmfs_super, tmp, sizeof(struct lmfs_super_block));

	/* fill the member of the os super block */
	sb->dev = 0;
	sb->dir = 0;
	sb->block_size_bits = 12;
	sb->block_size = 4096;
	sb->max_file = lmfs_super->fnode_nr;
	sb->filesystem = fs;
	sb->flags = 0;
	sb->magic = lmfs_super->magic;
	sb->bdev = bdev;

	sb->private_data = lmfs_super;

	err = init_lmfs_ibpool(sb);
	if (err)
		goto free_lmfs_super;

	err = init_lmfs_dbpool(sb);
	if (err)
		goto free_lmfs_ibpool;

	return 0;

free_lmfs_ibpool:
	free_pages(lmfs_super->ib_pool);

free_lmfs_super:
	kfree(lmfs_super);

out:
	free_pages(dbuffer);

	return err;
}

static int fnode_to_fnode_entry(struct fnode *fnode,
		struct lmfs_fnode_entry *entry)
{
	struct lmfs_fnode *lfnode = (struct lmfs_fnode *)fnode;

	entry->flag = fnode->flags;
	entry->mode = fnode->mode;
	entry->modify_ymd = 0;
	entry->modify_hms = 0;
	entry->size = fnode->data_size;
	entry->block_nr = lfnode->block_count;
	memcpy(entry->data_block, lfnode->data_block, 256);

	return 0;
}

static u32 lmfs_get_data_block(struct fnode *fnode, int whence)
{
	int ret = 0;
	struct lmfs_fnode *lfnode = fnode->private_data;
	int pos = lfnode->pos_in_lvl0;
	int pos_db = lfnode->pos_in_lvl1;
	u32 *buffer = lfnode->lvl1;
	block_t *db_buffer = lfnode->lvl1;
	struct super_block *sb = fnode->sb;

	switch (whence) {
	case VFS_START_BLOCK:
		if (pos == 0 && pos_db == 0) {
			lmfs_debug("already in the start block\n");
			break;
		}

		ret = lmfs_read_block(sb,(char *) buffer, db_buffer[0]);
		if (ret)
			return 0;

		lfnode->pos_in_lvl0 = 0;
		lfnode->pos_in_lvl1 = 0;
		lfnode->current_block = (buffer[0]);
		break;

	case VFS_CURRENT_BLOCK:
		break;

	case VFS_NEXT_BLOCK:
		if (pos_db == 1023) {
			if (db_buffer[pos + 1] != 0) {
				ret = lmfs_read_block(sb, (char *)buffer, 
						db_buffer[pos + 1]);
				if (ret)
					return 0;

				lfnode->pos_in_lvl0 = pos + 1;
				lfnode->pos_in_lvl1 = 0;
				lfnode->current_block = buffer[0];
			} else {
				return 0;
			}
		} else {
			lfnode->pos_in_lvl1 += 1;
			lfnode->current_block = buffer[lfnode->pos_in_lvl1];
		}
		break;

	case VFS_PREV_BLOCK:
		if (pos_db == 0) {
			if (pos == 0)
				return 0;
			ret = lmfs_read_block(sb, (char *)buffer,
					 db_buffer[pos - 1]);
			if (ret)
				return 0;
			lfnode->pos_in_lvl0 = pos - 1;
			lfnode->pos_in_lvl1 = 1023;
			lfnode->current_block = buffer[1023];
		} else {
			lfnode->pos_in_lvl1 -= 1;
			lfnode->current_block = buffer[lfnode->pos_in_lvl1];
		}
		break;

	default:
		return 0;
	}

	lmfs_debug("get data block %d %d\n", whence, lfnode->current_block);
	return lfnode->current_block;
}

static int lmfs_copy_fnode(struct fnode *fnode,
				struct fnode *parent)
{
	struct lmfs_fnode *plfnode = parent->private_data;
	struct lmfs_fnode *lfnode = fnode->private_data;

	fnode->sb = parent->sb;
	lfnode->lvl1 = get_free_page(GFP_KERNEL);
	if (!lfnode->lvl1)
		return -ENOMEM;

	return lmfs_fill_fnode(fnode, plfnode->fnode_index);
}

static int lmfs_fnode_write(struct fnode *fnode)
{
	struct super_block *sb = fnode->sb;
	struct lmfs_fnode_entry *tmp;
	int i, j;
	struct lmfs_fnode *lfnode = (struct lmfs_fnode *)fnode;

	if (!fnode)
		return -EINVAL;

	fnode_location(sb, lfnode->fnode_index, &i, &j);

	mutex_lock(&dbuffer_mutex);
	if (lmfs_read_block(sb, dbuffer, i)) {
		mutex_unlock(&dbuffer_mutex);
		return -EIO;
	}
	tmp = (struct lmfs_fnode_entry *)dbuffer;
	tmp += j;
	fnode_to_fnode_entry(fnode, tmp);

	if (lmfs_write_block(sb, dbuffer, i)) {
		mutex_unlock(&dbuffer_mutex);
		return -EIO;
	}
	mutex_unlock(&dbuffer_mutex);

	return 0;
}

int lmfs_find_file(char *name, struct fnode *fnode, char *buffer)
{
	struct lmfs_dir_entry *dir_entry;
	int j;
	int file_find = 0;
	int err = 0;

	dir_entry = (struct lmfs_dir_entry *)buffer;
	for (j = 0; j < 16; j++) {
		lmfs_debug("%s %s\n", name, dir_entry->name);
		if (!strcmp(name, dir_entry->name)) {
			file_find = 1;
			break;
		}

		dir_entry++;
	}

	/* got the indoe for the file */
	if (file_find) {
		err = lmfs_fill_fnode(fnode, dir_entry->fnode_index);
		return err;
	}

	return -ENOENT;
}

static int lmfs_free_block(struct fnode *fnode, block_t blk)
{
	block_t block = 0;
	struct super_block *sb = fnode->sb;

	/* free all block or the unused blocks */
	if (blk == 0) {

		block = lmfs_get_data_block(fnode, VFS_START_BLOCK);

		while (block != 0) {
			lmfs_push_block(sb, block);
			block = lmfs_get_data_block(fnode, VFS_NEXT_BLOCK);
		}
	} else {
		lmfs_push_block(sb, block);
	}

	return 0;
}

static int fnode_to_lmfs_entry(struct fnode *fnode,
		struct lmfs_fnode_entry *entry)
{
	/* TO be implemented */
	return 0;
}

size_t lmfs_getdents(struct fnode *fnode, struct dirent *dent, char *data)
{
	struct lmfs_dir_entry *entry = (struct lmfs_dir_entry *)data;
	int npos = fnode->rw_pos;

	while (npos < fnode->buffer_size) {
		npos += sizeof(struct lmfs_dir_entry);
		if (entry->name[0] == 0) {
			entry++;
			continue;
		}

		dent->d_type = entry->type;
		strcpy(dent->name, entry->name);
		
		lmfs_debug("npos:%d %s\n", npos, entry->name);
		break;
	}

	return (npos - fnode->rw_pos);
}

struct filesystem_ops lmfs_ops = {
	.find_file		= lmfs_find_file,
	.get_data_block		= lmfs_get_data_block,
	.copy_fnode		= lmfs_copy_fnode,
	.get_root_fnode		= lmfs_get_root_fnode,
	.get_block		= lmfs_get_new_block,
	.free_block		= lmfs_free_block,
	.read_block		= lmfs_read_block,
	.write_block		= lmfs_write_block,
	.write_fnode		= lmfs_fnode_write,
	.fill_super		= lmfs_fill_super,
	.write_superblock	= lmfs_write_sb,
	.getdents		= lmfs_getdents,
};

#define LMFS_FS_FLAG	\
	FS_FOPS_NEXT_BLOCK | FS_FOPS_PREV_BLOCK

struct filesystem lmfs = {
	.name 		= "lmfs",
	.fops		= &lmfs_ops,
	.flag		= LMFS_FS_FLAG,
};

int lmfs_init(void)
{
	return register_filesystem(&lmfs);
}

fs_initcall(lmfs_init);
