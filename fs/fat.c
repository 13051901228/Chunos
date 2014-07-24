#include <os/kernel.h>
#include <os/disk.h>
#include <os/filesystem.h>
#include <os/fnode.h>
#include <os/vfs.h>
#include <os/file.h>
#include <os/init.h>
#include "fat.h"

#define FAT12				0
#define FAT16				1
#define FAT32				2

#define FAT_NAME_TYPE_SHORT		0
#define FAT_NAME_TYPE_LONG		1	
#define FAT_NAME_TYPE_UNKNOWN		0xff

#define FAT_LONG_NAME_MIN_MATCH_SIZE	6

static u32 get_first_sector(struct fat_super_block *fsb, u32 clus)
{
	u32 sector;

	if (clus > (0xfff7 - fsb->fat12_16_root_dir_blk))	/* root dir for fat12 or fat 16*/
		sector = fsb->root_dir_start_sector +
			((clus - 0xfff7) * fsb->sec_per_clus);
	else
		sector = ((clus - 2) * fsb->sec_per_clus) +
				fsb->first_data_sector;
	return sector;
}

static int fat_read_block(struct super_block *sb, char *buffer, block_t block)
{
	struct fat_super_block *fsb = (struct fat_super_block *)sb->private_data;
	u32 start_sector = get_first_sector(fsb, block);

	kernel_debug("fat_read_block: block:%d start_sector:%d\n", block, start_sector);

	return read_sectors(sb->bdev, buffer,
			start_sector, fsb->sec_per_clus);
}

static int fat_write_block(struct super_block *sb, char *buffer, block_t block)
{
	struct fat_super_block *fsb = (struct fat_super_block *)sb->private_data;
	u32 start_sector = get_first_sector(fsb, block);

	return write_sectors(sb->bdev, buffer,
			start_sector, fsb->sec_per_clus);
}

static u8 get_fat_type(struct fat_super_block *fsb, char *tmp)
{
	u32 fat_size, total_sec;
	u32 data_sec, root_dir_sectors, clus_count;

	/* following is the way to check the fat fs type */
	root_dir_sectors = ((fsb->root_ent_cnt * 32) +
			   (fsb->byts_per_sec - 1)) /
		           fsb->byts_per_sec;

	if (fsb->fat16_sec_size != 0)
		fat_size = fsb->fat16_sec_size;
	else
		fat_size = u8_to_u32(tmp[36], tmp[37], tmp[38], tmp[39]);

	if (fsb->total_sec16 != 0)
		total_sec = fsb->total_sec16;
	else
		total_sec = fsb->total_sec32;

	data_sec = total_sec -
		(fsb->res_sec_cnt + fsb->fat_num * fat_size + root_dir_sectors);
	clus_count = data_sec / fsb->sec_per_clus;

	if (clus_count < 4085)
		fsb->fat_type = FAT12;
	else if (clus_count < 65525)
		fsb->fat_type = FAT16;
	else
		fsb->fat_type = FAT32;

	fsb->total_sec = total_sec;
	fsb->fat_size = fat_size;
	fsb->data_sec = data_sec;
	fsb->clus_count = clus_count;
	fsb->root_dir_sectors = root_dir_sectors;
	fsb->root_dir_start_sector = fsb->res_sec_cnt + fsb->fat_num * fat_size;

	/* do not support fat12 */
	if (fsb->fat_type == FAT32)
		fsb->fat_offset = 4;
	else
		fsb->fat_offset = 2;

	return fsb->fat_type;
}

static u32 clus_in_fat(struct fat_super_block *fsb, u32 clus, int *i, int *j)
{
	u32 offset;

	if (fsb->fat_type == FAT12)
		offset = clus + (clus / 2);
	else
		offset = fsb->fat_offset * clus;

	*i = fsb->res_sec_cnt + (offset / fsb->byts_per_sec);
	*j = offset % fsb->byts_per_sec;

	return 0;
}

static int fill_fat_super(struct fat_super_block *fsb, char *buf)
{
	u8 *tmp = (u8 *)buf;
	struct fat32_extra *extra32;
	struct fat16_extra *extra16;
	u8 fat_type;
	int i;

	if ((tmp[510] != 0x55) || (tmp[511] != 0xaa))
		return -EINVAL;

	memcpy(fsb->jmp, tmp, 3);
	memcpy(fsb->oem_name, &tmp[3], 8);
	fsb->byts_per_sec = u8_to_u16(tmp[11], tmp[12]);
	fsb->sec_per_clus = tmp[13];
	fsb->res_sec_cnt = u8_to_u16(tmp[14], tmp[15]);
	fsb->fat_num = tmp[16];
	fsb->root_ent_cnt = u8_to_u16(tmp[17], tmp[18]);
	fsb->total_sec16 = u8_to_u16(tmp[19], tmp[20]);
	fsb->media = tmp[21];
	fsb->fat16_sec_size = u8_to_u16(tmp[22], tmp[23]);
	fsb->sec_per_trk = u8_to_u16(tmp[24], tmp[25]);
	fsb->num_heads = u8_to_u16(tmp[26], tmp[27]);
	fsb->hide_sec = u8_to_u32(tmp[28], tmp[29], tmp[30], tmp[31]);
	fsb->total_sec32 = u8_to_u32(tmp[32], tmp[33], tmp[34], tmp[35]);

	fat_type = get_fat_type(fsb, buf);
	if ((fat_type == FAT16 || (fat_type == FAT12))) {
		extra16 = &fsb->fat_extra.fat16;
		extra16->drv_num = tmp[36];
		extra16->boot_sig = tmp[38];
		extra16->vol_id = u8_to_u32(tmp[39], tmp[40], tmp[41], tmp[42]);
		memcpy(extra16->vol_lab, &tmp[43], 11);
		memcpy(extra16->file_system, &tmp[54], 8);
	}
	else {
		extra32 = &fsb->fat_extra.fat32;
		extra32->fat32_sec_cnt = u8_to_u32(tmp[36], tmp[37], tmp[38], tmp[39]);
		extra32->fs_ver = u8_to_u16(tmp[42], tmp[43]);
		extra32->root_clus = u8_to_u32(tmp[44], tmp[45], tmp[46], tmp[47]);
		extra32->fs_info = u8_to_u16(tmp[48], tmp[49]);
		extra32->boot_sec = u8_to_u16(tmp[50], tmp[51]);
		extra32->drv_num = tmp[64];
		extra32->boot_sig = tmp[66];
		extra32->vol_id = u8_to_u32(tmp[67], tmp[68], tmp[69], tmp[70]);
		memcpy(extra32->vol_lab, &tmp[71], 11);
		memcpy(extra32->file_system, &tmp[82], 8);
	}

	fsb->clus_size = fsb->byts_per_sec * fsb->sec_per_clus;
	fsb->first_data_sector = fsb->res_sec_cnt +
		(fsb->fat_num * fsb->fat_size) + fsb->root_dir_sectors;
	fsb->dentry_per_block = fsb->clus_size / 32;
	fsb->fat12_16_root_dir_blk =
		(fsb->root_dir_sectors + (fsb->sec_per_clus - 1)) / fsb->sec_per_clus;

	return 0;
}

static int fat_fill_super(struct bdev *bdev,
		struct filesystem *fs,
		struct super_block *sb)
{
	int ret = 0;
	struct fat_super_block *fat_super;
	char *buf;

	buf = get_free_page(GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	ret = read_sectors(bdev, buf, 0, 8);
	if (ret)
		goto err_read_block;

	fat_super = kzalloc(sizeof(struct fat_super_block), GFP_KERNEL);
	if (!fat_super)
		goto err_read_block;

	ret = fill_fat_super(fat_super, buf);
	if (ret)
		goto err_get_super_block;

	fat_super->buf = get_free_pages(page_nr(fat_super->clus_size), GFP_KERNEL);
	if (!fat_super->buf) {
		ret = -ENOMEM;
		goto err_get_super_block;
	}
	init_mutex(&fat_super->buf_mutex);
	fat_super->buf_sector_base = 0;

	kernel_debug("------Fat information-----\n");
	kernel_debug("clus size :%d\n", fat_super->clus_size);
	kernel_debug("first_data_sector: %d\n", fat_super->first_data_sector);
	kernel_debug("total_sec: %d\n", fat_super->total_sec);
	kernel_debug("fat_size: %d\n", fat_super->fat_size);
	kernel_debug("data_sec: %d\n", fat_super->data_sec);
	kernel_debug("clus_count: %d\n", fat_super->clus_count);
	kernel_debug("root_dir_sectors: %d\n", fat_super->root_dir_sectors);
	kernel_debug("dentry_per_block: %d\n", fat_super->dentry_per_block);
	kernel_debug("fat_type :%d\n", fat_super->fat_type);
	kernel_debug("fat_offset: %d\n", fat_super->fat_offset);
	kernel_debug("byts_per_sec: %d\n", fat_super->byts_per_sec);
	kernel_debug("sec_per_clus: %d\n", fat_super->sec_per_clus);
	kernel_debug("res_sec_cnt: %d\n", fat_super->res_sec_cnt);
	kernel_debug("root_ent_cnt: %d\n", fat_super->root_ent_cnt);
	kernel_debug("root_dir_start_secrot: %d\n", fat_super->root_dir_start_sector);
	kernel_debug("fat12_16_root_dir_blk: %d\n", fat_super->fat12_16_root_dir_blk);
	kernel_debug("------------------------\n");

	sb->dev = 0;
	sb->dir = 0;
	sb->block_size_bits = 12;
	sb->block_size = 4096;
	sb->max_file = 0;
	sb->filesystem = fs;
	sb->flags = 0;
	sb->magic = 0;
	sb->bdev = bdev;
	sb->private_data = (void *)fat_super;

	return 0;

err_get_super_block:
	kfree(fat_super);
err_read_block:
	kfree(buf);

	return ret;
}

static int fat_file_type(struct fat_dir_entry *entry)
{
	int type = DT_UNKNOWN;
	int attr = entry->dir_attr;
	
	/* if it is not a long name file */
	if ((attr & FAT_ATTR_LONG_NAME_MASK) != FAT_ATTR_LONG_NAME) {
		if (attr & FAT_ATTR_DIRECTORY)
			type = DT_DIR;
		else if (attr & FAT_ATTR_VOLUME_ID)
			type = FAT_ATTR_VOLUME_ID;
		else if ((attr & (FAT_ATTR_VOLUME_ID | FAT_ATTR_DIRECTORY)) == 0)
			type = DT_BLK;
		else
			kernel_info("unknow fat file type");
	}

	return type;
}

static int entry_name_type(struct fat_dir_entry *entry)
{
	if ((entry->dir_attr & FAT_ATTR_LONG_NAME_MASK) == FAT_ATTR_LONG_NAME)
		return FAT_NAME_TYPE_LONG;
	else
		return FAT_NAME_TYPE_SHORT;
}

static int cmp_long_name(struct fnode *root, char *buf,
		int i, char *long_name, int size)
{
	struct fat_long_dir_entry *lentry =
		((struct fat_long_dir_entry *)buf) + i;
	struct fat_fnode *ffnode = root->private_data;
	struct fat_super_block *fsb = root->sb->private_data;
	int _cmp_size = 0;
	int j;
	u8 cmp_size[3] = {10, 12, 4};
	
	if (size <= 0)
		return -EINVAL;

	do {
		for (j = 0; j < 3; j++) {
			_cmp_size = MIN(size, cmp_size[j]);
			if (memcmp((char *)long_name, (char *)lentry->name1, _cmp_size))
				goto out;

			long_name += _cmp_size;
			size -= _cmp_size;
			if (size == 0)
				return 0;
		}
		
		lentry--;
		i--;
		if (i < 0) {
			/* 
			 * the info is in different clus load
			 * the previous clus data
			 */
			if (fat_read_block(root->sb, buf,
						ffnode->prev_clus))
				return -EIO;
			lentry = ((struct fat_long_dir_entry *)buf) +
				 (fsb->dentry_per_block - 1);
			i = fsb->dentry_per_block - 1;
		}
	} while (size > 0);
out:
	return size;
}

static int fat_fill_fnode(struct fnode *fnode,
		struct fat_super_block *fsb,
		struct fat_dir_entry *entry)
{
	struct fat_fnode *ffnode = (struct fat_fnode *)fnode->private_data;

	if (fsb->fat_type == FAT32)
		ffnode->first_clus = u16_to_u32(entry->fst_cls_low,
					entry->fst_cls_high);
	else
		ffnode->first_clus = entry->fst_cls_low;
	
	ffnode->first_sector = get_first_sector(fsb, ffnode->first_clus);
	ffnode->prev_clus = ffnode->first_clus;
	ffnode->prev_sector = ffnode->first_sector;
	ffnode->current_clus = ffnode->first_clus;
	ffnode->current_sector = ffnode->first_sector;

	fnode->mode = entry->dir_attr;
	fnode->flags = entry->dir_attr;
	fnode->atime = u16_to_u32(entry->crt_date, entry->crt_time);
	fnode->mtime = u16_to_u32(entry->write_date, entry->write_time);
	fnode->ctime = entry->crt_time_teenth;
	fnode->fnode_size = 32;
	fnode->state = 0;
	fnode->data_size = entry->file_size;
	fnode->blk_cnt = entry->file_size / fsb->clus_size;
	fnode->buffer_size = fsb->clus_size;
	fnode->psize = sizeof(struct fat_fnode);
	fnode->type = fat_file_type(entry);

	return 0;
}

static struct fnode *fat_get_root_fnode(struct super_block *sb)
{
	struct fnode *fnode;
	struct fat_fnode *ffnode;
	struct fat_super_block *fsb = (struct fat_super_block *)sb->private_data;

	fnode = allocate_fnode(sizeof(struct fat_fnode));
	if (!fnode)
		return NULL;

	ffnode = (struct fat_fnode *)fnode_get_private(fnode);
	if (fsb->fat_type == FAT32)
		ffnode->first_clus = 2;
	else
		ffnode->first_clus = 0xfff7;

	ffnode->prev_clus = ffnode->first_clus;
	ffnode->current_clus = ffnode->first_clus;

	fnode->fnode_size = 32;
	fnode->sb = sb;
	fnode->psize = sizeof(struct fat_fnode);
	fnode->buffer_size = fsb->clus_size;

	return fnode;
}

static int sector_in_fat_buffer(struct fat_super_block *fsb, int i, int j)
{
	if (fsb->buf_sector_base == 0)
		return 0;

	if ((i >= fsb->buf_sector_base) &&
	    (i < fsb->buf_sector_base + fsb->sec_per_clus)) {
		return ((i - fsb->buf_sector_base) * fsb->byts_per_sec) / fsb->fat_offset + j;
	}

	return 0;
}

static u32 __fat_get_data_block(struct fnode *fnode)
{
	struct fat_fnode *ffnode = fnode->private_data;
	struct fat_super_block *fsb =
		(struct fat_super_block *)fnode->sb->private_data;
	char *buf = fsb->buf;
	int i, j, k;
	u32 temp;

	mutex_lock(&fsb->buf_mutex);
	clus_in_fat(fsb, ffnode->current_clus, &i, &j);
	kernel_debug("%s i:%d j:%d\n", __func__, i, j);
	if (!(k = sector_in_fat_buffer(fsb, i, j))) {
		/* if the fat entry already in buffer */
		if (read_sectors(fnode->sb->bdev, buf, i,
				4096 / fsb->byts_per_sec))
			return 0;
		fsb->buf_sector_base = i;
		k = j;
		kernel_debug("not in buffer k:%d\n", k);
	} else {
		kernel_debug("in buffer k:%d\n", k);
		if (fsb->fat_type == FAT12) {
			if (k == 4095) {
				if (read_sectors(fnode->sb->bdev, buf, i,
						4096 / fsb->byts_per_sec));
					return 0;
				fsb->buf_sector_base = i;
				k = j;
			}
		}
	}

	ffnode->prev_clus = ffnode->current_clus;
	if (fsb->fat_type == FAT32)
		ffnode->current_clus = *((u32 *)&buf[k]) & 0x0fffffff;
	else if (fsb->fat_type == FAT16)
		ffnode->current_clus = *((u16 *)&buf[k]);
	else {
		temp = ffnode->current_clus;
		ffnode->current_clus = u8_to_u16(buf[k], buf[k + 1]);
		if (temp & 0x0001) {
			ffnode->current_clus >>= 4;
		} else {
			ffnode->current_clus &= 0x0fff;
		}
	}
	mutex_unlock(&fsb->buf_mutex);

	return ffnode->current_clus;
}

static u32 fat12_16_get_data_block(struct fnode *fnode)
{
	struct fat_fnode *ffnode = fnode->private_data;
	struct fat_super_block *fsb =
		(struct fat_super_block *)fnode->sb->private_data;
	u16 count = fsb->fat12_16_root_dir_blk;

	kernel_debug("%s 0x%x\n", __func__, ffnode->current_clus);
	if (ffnode->current_clus > (0xfff7 - count)
	    && ffnode->current_clus != 0) {
		ffnode->prev_clus = ffnode->current_clus;
		ffnode->current_clus--;

		if (ffnode->current_clus == (0xfff7 - count)) {
			ffnode->current_clus = 0;
		}
		kernel_debug("%s 0x%x\n", __func__, ffnode->current_clus);
		return ffnode->current_clus;
	}

	if (ffnode->current_clus == 0)
		return 0;

	return __fat_get_data_block(fnode);
}

static u32 fat_get_data_block(struct fnode *fnode, int whence)
{
	struct fat_super_block *fsb =
		(struct fat_super_block *)fnode->sb->private_data;
	struct fat_fnode *ffnode = fnode->private_data;

	switch (whence) {
	case VFS_NEXT_BLOCK:
		if (fsb->fat_type == FAT32)
			__fat_get_data_block(fnode);
		else
			fat12_16_get_data_block(fnode);
		break;

	case VFS_START_BLOCK:
		/* reset the block data information */
		ffnode->current_clus = ffnode->first_clus;
		ffnode->prev_clus = ffnode->current_clus;
		break;

	case VFS_CURRENT_BLOCK:
		break;

	case VFS_END_BLOCK:
	case VFS_PREV_BLOCK:
	default:
		return 0;
	}
	
	/* whether at the end of the file */
	if (fsb->fat_type == FAT32) {
		if (ffnode->current_clus >= 0xfffffff8)
			return 0;
	} else {
		if (ffnode->current_clus >= 0xfff8)
			return 0;
	}

	kernel_debug("fat_get_block: whence%d block:0x%x\n",
			whence, ffnode->current_clus);
	return ffnode->current_clus;
}

static int fat_find_file(char *name, struct fnode *fnode, char *buffer)
{
	struct super_block *sb = fnode->sb;
	struct fat_super_block *fsb = sb->private_data;
	struct fat_dir_entry *entry = (struct fat_dir_entry *)buffer;
	struct fat_fnode *ffnode = fnode->private_data;
	int i;
	u8 is_long;
	int len = strlen(name);
	char *long_name = &name[12];

	is_long = name[11];

	for (i = 0; i < fsb->dentry_per_block; i++) {	
		if ((entry_name_type(entry) == FAT_NAME_TYPE_LONG) ||
					entry->name[0] == 0) {
			entry++;
			continue;
		}

		kernel_debug("entry name is %s\n", entry->name);
		if (is_long) {
			/* 
			 * we compare the min size of the short name
			 * when the file's name is long
			 */
			if (strncmp(name, entry->name, 
				    FAT_LONG_NAME_MIN_MATCH_SIZE))
				continue;

			/* 
			 * do not consider the case when the entry is
			 * in different clus, to be done later
			 */
			ffnode->same_name++;
			if (!cmp_long_name(fnode, buffer, i, long_name, len - 12))
				goto find_file;

		} else {
			if (!strncmp(name, entry->name, 11))
				goto find_file;
		}

		entry++;
	}

	return -ENOENT;

find_file:
	/* fill the fnode struct */
	ffnode->file_entry_clus = fat_get_data_block(fnode, VFS_CURRENT_BLOCK);
	ffnode->file_entry_pos = i;
	fat_fill_fnode(fnode, fsb, entry);

	return 0;
}


static int get_file_name_type(char *name, int f, int b)
{
	/*
	 * support for unix hidden file, buf windows
	 * do not support support it
	 */
	if (*name == '.')
		return FAT_NAME_TYPE_LONG;

	if (f > 8)
		return FAT_NAME_TYPE_LONG;

	if (f == 0) {
		if (b > 3)
			return FAT_NAME_TYPE_LONG;
		else
			return FAT_NAME_TYPE_SHORT;
	}

	if (f < 8) {
		if (b <= 3)
			return FAT_NAME_TYPE_SHORT;
		else
			return FAT_NAME_TYPE_LONG;
	}

	return FAT_NAME_TYPE_UNKNOWN;
}

static int is_lower(char ch)
{
	return ((ch <= 'z') && (ch >= 'a'));
}

char lower_to_upper(char ch)
{
	if (is_lower(ch))
		return (ch -'a' + 'A');

	return ch;
}

static int fill_fat_name(char *target, char *source, int len, int type)
{
	int i = 0;
	int dot_pos = 0;
	char ch;

	/* skip the . at the begining of the string */
	while (source[dot_pos] == '.')
		dot_pos++;
	do {
		ch = lower_to_upper(source[dot_pos++]);
		if (ch == 0)
			break;

		if ((ch != ' ') && (ch != '.'))
			target[i++] = ch;

		if (dot_pos == len)
			break;
	} while (i < 6);

	/* 
	 * if the name is a long name file we add ~ 
	 * the end of the name and 0 will indicate
	 * we need to match the long name
	 */
	for (; i < 8; i++) {
		if (type) {
			target[i++] = '~';
			target[i] = '1';
			type = 0;
		} else {
			target[i] = ' ';
		}
	}

	return 0;
}

/* to ensure the following string: " er .txt" ".123.123" "egg .txt" "egg~.txt" */
static int fill_fat_appendix(char *target, char *source, int len, int type)
{
	int dot_pos = 0;
	int i = 0;
	char ch;

	if (!len)
		goto out;

	/*
	 * skip the . at the begining of the string and
	 * also need to skip the blank when
	 */
	while (source[dot_pos] == '.')
		dot_pos++;

	do {
		ch = lower_to_upper(source[dot_pos++]);
		if (ch == 0)
			break;

		if (ch != ' ') {
			target[8 + i] = ch;
			i++;
		}
				
		/* reach the end of the string */
		if (dot_pos == len)
			break;
	} while (i < 3);
	
out:	
	for (; i < 3; i++ )
		target[8 + i] = ' ';

	return 0;
}

static int fat_copy_fnode(struct fnode *fnode, struct fnode *parent)
{
	memcpy(fnode->private_data, parent->private_data, parent->psize);

	return 0;
}

static int fat_parse_name(char *name)
{
	char *buf;
	char *tmp;
	short *tmp1;
	int i, dot_pos, type = 0;
	int len = strlen(name);
	int f, b;

	if (!name || !len)
		return -EINVAL;

	buf = get_iname_buffer();
	if (!buf)
		return -EAGAIN;

	tmp = name +len;
	for (dot_pos = len; dot_pos > 0; dot_pos--) {
		if (name[dot_pos] == '.')
			break;
	}

	f = dot_pos;
	if (f == 0)
		f = len;

	if (dot_pos == 0)
		b = 0;
	else
		b = len - dot_pos - 1;

	type = get_file_name_type(name, f, b);
	kernel_debug("dot_pos:%d f:%d b:%d type:%d\n",
			dot_pos, f, b, type);

	/* copy the name to the buffer dot_pos will */
	memset(buf, 0, 512);

	/* add 1 to skip the dot '.' */
	fill_fat_appendix(buf, name + f + 1, b, type);
	fill_fat_name(buf, name, f, type);

	/* covert the string to unicode */
	tmp1 = (short *)(&buf[12]);
	for (i = 0; i < len; i++)
		tmp1[i] = name[i];
		
	buf[11] = type;
	memcpy(name, buf, 512);
	put_iname_buffer(buf);

	return 0;
}

#define FAT_FS_FLAG	\
	FS_FOPS_NEXT_BLOCK

struct filesystem_ops fat_ops = {
	.find_file 	= fat_find_file,
	.get_data_block = fat_get_data_block,
	.parse_name	= fat_parse_name,
	.read_block	= fat_read_block,
	.write_block	= fat_write_block,
	.copy_fnode	= fat_copy_fnode,
	.get_root_fnode = fat_get_root_fnode,
	.fill_super	= fat_fill_super,
};

struct filesystem fat = {
	.name 		= "msdos",
	.fops 		= &fat_ops,
};

int fat_init(void)
{
	return register_filesystem(&fat);
}

fs_initcall(fat_init);
