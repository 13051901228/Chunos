#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "types.h"
#include "lmfs.h"

#define MAX_SIZE	(1 * 1024 * 1024)  /* 1M */
#define MAX_FILE	(32)

#define BLOCK_SIZE	(4096)

#define baligin(num,size) 	((num + size - 1) & ~(size - 1))
#define daligin(num,size) 	(((num + size - 1) / size) * size)
#define min_aligin(num,size)	((num) & ~(size - 1))

#ifdef DEBUG
#define debug(fmt, ...)	printf(fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

struct lmfs_address {
	struct lmfs_super_block *p_sb;
	u32 *p_ib;
	u32 *p_db;
	char *p_inode;
	char *p_data;
	u32 sb_size;
	u32 ib_size;
	u32 db_size;
	u32 inode_size;
};

static struct lmfs_address address;

struct file_buffer {
	char inode_entry[512];		/* base size is 512 bytes */
	int inode;

	int lvl1_pos;
	int lvl0_pos;
	int data_pos;

	u32 data;
	u32 lvl1;

	char block_buffer[4096];
	char data_buffer[4096];

	u32 *bmp;			/* bitmap */
	u32 *lvl0_buf;
};

#define TYPE_FILE	0
#define TYPE_DIR	1
void dump_super_block(struct lmfs_super_block *sb)
{
	u32 *tmp = (u32 *)sb;
	int size, i;

	size = (sizeof(struct lmfs_super_block) / sizeof(u32));

	for (i = 0; i < size; i ++)
		printf("0x%x ", tmp[i]);
	printf("\n");
}

static void init_buffer_pool(u32 *addr, u32 base, int count)
{
	int i = 0;

	while (count) {
		addr[i] = base + count - 1;
		i++;
		count--;
	}
}

static init_super_block(struct lmfs_address *addr, u32 data_start,
			u32 free_block, u32 block_cnt)
{
	struct lmfs_super_block *sb = addr->p_sb;
	char *disk = (char *)sb;
	int i = 0;
	
	memcpy((char *)&sb->magic, "LMFS", 4);
	sb->n_blocks = block_cnt;
	sb->start_block = 0;
	sb->super_block_size = BLOCK_SIZE;

	/* init inode buffer pool base is 0*/
	sb->ib_pool_start_block = ((char *)addr->p_ib - disk) / BLOCK_SIZE;
	sb->ib_pool_current_block = baligin(MAX_FILE * sizeof(u32), BLOCK_SIZE) / BLOCK_SIZE - 1;
	sb->ib_pool_current_block += sb->ib_pool_start_block;
	sb->ib_pool_pos = MAX_FILE - 2;			// 14 15 inodes 
	sb->ib_pool_block_nr = addr->ib_size / BLOCK_SIZE;
	init_buffer_pool(addr->p_ib, 1, MAX_FILE  -1);	// base is 1 since 0 for root

	sb->db_pool_start_block = ((char *)addr->p_db - disk) /BLOCK_SIZE;
	sb->db_pool_current_block = baligin(free_block * sizeof(u32), BLOCK_SIZE) / BLOCK_SIZE -1;
	sb->db_pool_current_block += sb->db_pool_start_block;
	sb->db_pool_pos = free_block - 1;
	sb->db_pool_block_nr = addr->db_size / BLOCK_SIZE;
	init_buffer_pool(addr->p_db, data_start, free_block);

	sb->inode_start_block = ((char *)addr->p_inode - disk) / BLOCK_SIZE;
	sb->inode_free = MAX_FILE;
	sb->inode_nr = MAX_FILE;
	sb->inode_n_blocks = addr->inode_size / BLOCK_SIZE;
	sb->data_start_block = data_start;
	sb->data_free_blocks = free_block;

	sb->root_inode_index = 0;
	sb->ib_pool = 0;
	sb->db_pool = 0;
	
	dump_super_block(sb);

	return 0;
}

static void format_disk(char *disk, struct lmfs_address *addr, int files)
{
	u32 ib_size = 0;
	u32 db_size = 0;
	u32 inode_size = 0;
	u32 sb_size = 0;
	u32 block_cnt = (MAX_SIZE / BLOCK_SIZE);
	u32 data_start_block;
	u32 free_block;

	sb_size = BLOCK_SIZE;
	ib_size = baligin((sizeof(u32) * files), BLOCK_SIZE);
	inode_size = baligin(512 *files, BLOCK_SIZE);
	db_size = block_cnt - ((ib_size + inode_size + sb_size) / BLOCK_SIZE);
	db_size = baligin(sizeof(u32) *files, BLOCK_SIZE);

	addr->p_sb = (struct lmfs_super_block *)disk;
	addr->p_ib = (u32 *)(disk + sb_size);
	addr->p_db = (u32 *)(disk + sb_size + ib_size);
	addr->p_inode = disk + sb_size + ib_size + db_size;
	addr->p_data = disk + sb_size + ib_size + db_size + inode_size;
	addr->sb_size = sb_size;
	addr->ib_size = ib_size;
	addr->db_size = db_size;
	addr->inode_size = inode_size;

	data_start_block = (addr->p_data - disk) / BLOCK_SIZE;
	free_block = block_cnt - data_start_block;

	printf("p_sb:0x%x p_ib:0x%x p_db:0x%x p_inode:0x%x p_data:0x%x\n",
	       addr->p_sb, addr->p_ib, addr->p_db, addr->p_inode, addr->p_data);
	printf("block_cnt:%d, free_block:%d data_start_block:%d\n",
			block_cnt, free_block, data_start_block);

	(void)init_super_block(addr, data_start_block, free_block, block_cnt);
}

static void init_file_buffer(struct file_buffer *ret, int type)
{
	if (!ret)
		return;

	if (type == TYPE_FILE) { /* type file */
		ret->lvl0_buf = (u32 *)&ret->inode_entry[256];
		ret->bmp = NULL;
	}
	else {
		ret->bmp = (u32 *)&ret->inode_entry[256];
		ret->lvl0_buf = (u32 *)&ret->inode_entry[384];
	}

	ret->data = 0;
	ret->lvl1 = 0;
	ret->lvl0_pos = 0;
	ret->lvl1_pos = 0;
	ret->data_pos = 0;
}

static struct file_buffer *allocate_file_buffer(void)
{
	struct file_buffer *ret;

	ret = (struct file_buffer *)malloc(4 * 4096);
	if (!ret)
		return NULL;

	memset(ret, 0, 4 * 4096);
	return ret;
}

static void free_file_buffer(struct file_buffer *fb)
{
	free(fb);
}

static int init_inode_entry(struct file_buffer *p, int inode, int type)
{
	struct lmfs_inode_entry *linode =
		(struct lmfs_inode_entry *)p->inode_entry;

	/* will add more attribute later */
	linode->flag = type;
	linode->mode = type;
	linode->type = type;
	linode->create_ymd = 0x19700101;
	linode->create_hms = 0;
	linode->modify_ymd = 0;
	linode->modify_hms = 0x1234;

	if (type == TYPE_DIR)
		linode->size = 4096;
	else
		linode->size = 0;

	linode->file_nr = 0;
	linode->block_nr = 0;
	linode->pos_in_lvl1 = 0;
	linode->pos_in_lvl0 = 0;
}

static int pop_inode()
{
	struct lmfs_super_block *lsb = address.p_sb;
	u32 *pib = address.p_ib;
	int inode;

	/* if there are no more inode */
	if ((lsb->ib_pool_pos == 1023) &&
	    (lsb->ib_pool_current_block == (lsb->ib_pool_start_block - 1)))
		return -ENOSPC;

	pib = pib + (lsb->ib_pool_current_block - lsb->ib_pool_start_block) * 1024;
	inode = pib[lsb->ib_pool_pos];
	pib[lsb->ib_pool_pos] = 0;
	debug("pop inode %d\n", inode);

	if (lsb->ib_pool_pos == 0) {
		lsb->ib_pool_pos = 1023;
		lsb->ib_pool_current_block--;
	}
	else {
		lsb->ib_pool_pos--;
	}

	lsb->inode_free--;

	return inode;
}

static u32 pop_block()
{
	struct lmfs_super_block *lsb = address.p_sb;
	u32 *pdb = address.p_db;
	u32 block;

	/* if there are no more inode */
	if ((lsb->db_pool_pos == 1023) &&
	    (lsb->db_pool_current_block == (lsb->db_pool_start_block - 1)))
		return -ENOSPC;

	pdb = pdb + (lsb->db_pool_current_block - lsb->db_pool_start_block) * 1024;
	block = pdb[lsb->db_pool_pos];
	pdb[lsb->db_pool_pos] = 0;
	debug("pop new block %d\n", block);

	if (lsb->db_pool_pos == 0) {
		lsb->db_pool_pos = 1023;
		lsb->db_pool_current_block--;
	}
	else {
		lsb->db_pool_pos--;
	}

	lsb->data_free_blocks--;

	return block;
}

static void flush_data(char *buffer, u32 block)
{
	char *tmp = (char *)address.p_sb;

	debug("flush data, block is %d\n", block);
	tmp += block * 4096;
	memcpy(tmp, buffer, 4096);
}

static void write_sector(char *buffer, int sector)
{
	char *tmp = (char *)address.p_sb;

	debug("write sector %d \n", sector);
	tmp += sector * 512;
	memcpy(tmp, buffer, 512);
}

static int inode_to_sector(int inode)
{
	struct lmfs_super_block *lsb = address.p_sb;

	return (lsb->inode_start_block * 8) + inode;
}

static int write_inode(struct file_buffer *fb)
{
	struct lmfs_inode_entry *lp = (struct lmfs_inode_entry *)fb->inode_entry;
	char *data_buffer = fb->data_buffer;
	u32 *block_buffer = (u32 *)fb->block_buffer;
	int i;

	debug("write inode %d\n", fb->inode);
	
	lp->pos_in_lvl1 = fb->lvl1_pos;
	lp->pos_in_lvl0 = fb->lvl0_pos;

	/* fix me some conner case will block
	 * data write to the file
	 * should consider the fb->data_pos and the
	 * fb->lvl1_pos. TBD later
	 */
	if (fb->data)
		flush_data(data_buffer, fb->data);

	printf("lvl1 %d buf[0] %d\n", fb->lvl1, block_buffer[0]);
	if (fb->lvl1)
		flush_data((char *)block_buffer, fb->lvl1);
	
	i = inode_to_sector(fb->inode);
	
	/* first copy the inode meta data, then copy the block info */
	//memcpy(data_buffer, lp, sizeof(struct lmfs_inode_entry) - 64 * sizeof(u32));
	//memcpy(data_buffer + 256, lp->lvl1_block, 256);
	write_sector((char *)lp, i);

	return 0;
}

static int write_file(int file, struct file_buffer *bfile)
{
	struct lmfs_inode_entry *lp =
		(struct lmfs_inode_entry *)bfile->inode_entry;
	char *data_buffer = bfile->data_buffer + bfile->data_pos;
	u32 *block_buffer = (u32 *)bfile->block_buffer;
	int i;
	int read_size = 0;

	do {
		if (bfile->data_pos == 0) {
			/* flush data to the data block */
			if (bfile->data)
				flush_data(data_buffer, bfile->data);
			bfile->data = pop_block();
			if (!bfile->data)
				return -ENOSPC;
			memset(data_buffer, 0, 4096);

			if (bfile->lvl1_pos == 0) {
				if (bfile->lvl1)
					flush_data((char *)block_buffer, bfile->lvl1);
				bfile->lvl1 = pop_block();
				debug("allocate new lvl1 block in write file\n");
				if (!bfile->lvl1)
					return -ENOSPC;
				memset(block_buffer, 0, 4096);

				if (bfile->lvl0_pos == 32) {
					return -ENOSPC;
				}
				else
					bfile->lvl0_buf[bfile->lvl0_pos] = bfile->lvl1;
			}
			lp->block_nr ++;
			block_buffer[bfile->lvl1_pos] = bfile->data;
		}

		read_size = read(file, data_buffer, 4096);
		debug("read size is %d\n", read_size);
		if (read_size > 0) {
			bfile->data_pos += read_size;
			if (bfile->data_pos == 4096) {
				bfile->data_pos = 0;
				bfile->lvl1_pos++;
				if (bfile->lvl1_pos == 1024) {
					bfile->lvl1_pos = 0;
					bfile->lvl0_pos++;
				}
			}
			lp->size += read_size;
			debug("lp->size is %d\n", lp->size);
			if (read_size < 4096)
				break;
		}
		else
			break;

	} while (read_size == 4096);

	write_inode(bfile);

	return 0;
}

static void update_bitmap(struct file_buffer *dir)
{
	struct lmfs_inode_entry *le =
		(struct lmfs_inode_entry *)dir->inode_entry;
	u32 *bitmap = dir->bmp;
	int n = le->file_nr - 1;
	int x, y;

	x = n / (sizeof(u32) * 8);
	y = n % (sizeof(u32) * 8);

	bitmap[x] = bitmap[x] | (1 << y);
}

static int write_entry_to_dir(struct file_buffer *dir,
		char *name, int inode, int type)
{
	struct lmfs_inode_entry *lp =
		(struct lmfs_inode_entry *)dir->inode_entry;
	char *buffer = dir->data_buffer + dir->data_pos;
	u32 *block_buffer = (u32 *)dir->block_buffer;
	u32 *inode_addr;
	struct lmfs_dir_entry *dent;

	if (dir->data_pos == 0) {
		/* flush data to the data block */
		if (dir->data)
			flush_data(buffer, dir->data);
		dir->data = pop_block();
		if (!dir->data)
			return -ENOSPC;
		memset(buffer, 0, 4096);

		if (dir->lvl1_pos == 0) {
			debug("allocate new lvl1 block\n");
			if (dir->lvl1)
				flush_data((char *)block_buffer, dir->lvl1);
			dir->lvl1 = pop_block();
			if (!dir->lvl1)
				return -ENOSPC;
			memset(block_buffer, 0, 4096);

			if (dir->lvl0_pos == 32) {
				return -ENOSPC;
			}
			else
				dir->lvl0_buf[dir->lvl0_pos] = dir->lvl1;
		}
		lp->block_nr++;
		block_buffer[dir->lvl1_pos] = dir->data;
	}
	
	dent = (struct lmfs_dir_entry *)buffer;
	strcpy(dent, name);
	dent->type = type;
	dent->inode_index = inode;

	dir->data_pos += 256;
	if (dir->data_pos == 4096) {
		dir->data_pos = 0;
		debug("write file entry, data pos is 4096\n");
		dir->lvl1 ++;
		if (dir->lvl1 == 1024) {
			debug("write file entry, lvl1 pos is 1024\n");
			dir->lvl1 = 0;
			
		}
	}

	lp->file_nr++;
	update_bitmap(dir);

	return 0;
}

static int create_files(char *dir_name, int inode)
{
	DIR *dir;
	struct dirent *pdir;
	struct file_buffer *bdir;
	struct file_buffer *bfile;
	char *name;
	char *tmp;
	int len = 0;
	int file;
	int ret = 0;
	int finode = 0;

	dir = opendir(dir_name);
	if (dir == NULL) {
		printf("Can not open dir %s\n", dir_name);
		return -EACCES;
	}

	bdir = allocate_file_buffer();
	if (!bdir) {
		ret = -ENOMEM;
		goto close_dir;
	}
	init_file_buffer(bdir, DT_DIR);
	bdir->inode = inode;

	bfile = allocate_file_buffer();
	if (!bdir) {
		ret = -ENOMEM;
		goto release_bdir;
	}

	name = malloc(1024);
	if (!name) {
		ret = -ENOMEM;
		goto release_bfile;
	}
	memset(name, 0, 1024);

	init_inode_entry(bdir, inode, DT_DIR);

	while (pdir = readdir(dir)) {
		if (!strcmp(pdir->d_name, "."))
			continue;
		if(!strcmp(pdir->d_name, ".."))
			continue;

		finode = pop_inode();
		if (!finode)
			goto release_bfile;
		
		strcpy(name, dir_name);
		len = strlen(name);
		if (name[len - 1] != '/')
			strcat(name, "/");
		strcat(name, pdir->d_name);
		printf("Add file [ %s ] to disk\n", name);
		
		if (pdir->d_type == DT_DIR) {
			write_entry_to_dir(bdir, pdir->d_name, finode, DT_DIR);
			create_files(name, finode);
		} else {
			write_entry_to_dir(bdir, pdir->d_name, finode, DT_BLK);
			init_file_buffer(bfile, TYPE_FILE);
			bfile->inode = finode;
			init_inode_entry(bfile, finode, DT_BLK);
			file = open(name, O_RDONLY);
			if(file < 0) {
				printf("can not open file %s\n", name);
				continue;
			}
			write_file(file, bfile);
		}
	}

	write_inode(bdir);

release_file_name_buffer:
	free(name);
release_bdir:
	free_file_buffer(bfile);
release_bfile:
	free_file_buffer(bdir);
close_dir:
	closedir(dir);

	return ret;
}

static int _mklmfs(char *disk, char *dir)
{
	struct lmfs_address *paddress = &address;
	int inode;
	int ret;

	format_disk(disk, paddress, MAX_FILE);
	
	ret = create_files(dir, 0);
	dump_super_block(address.p_sb);

	return ret;
}

int mklmfs(char *dir, char *target)
{
	char *disk = NULL;
	int ftarget = 0;
	int ret;

	disk = (char *)malloc(MAX_SIZE);
	if (disk == NULL) {
		printf("can not allocate temp memory\n");
		return 1;
	}
	memset(disk, 0, MAX_SIZE);

	ftarget = open(target, O_RDWR | O_CREAT, S_IRWXU);
	if (ftarget < 0) {
		printf("can not creat disk %s\n", target);
		free(disk);
		return 1;
	}

	if (_mklmfs(disk, dir)) {
		goto out;
	}

	ret = write(ftarget, disk, MAX_SIZE);
	debug("write file ret is %d\n", ret);
	if (ret < MAX_SIZE)
		printf("there are some errors when write buffer to file\n");

out:
	close(ftarget);
	free(disk);

	return 0;
}

int main(int argc, char **argv)
{
	if (!argv[1] || !argv[2]) {
		printf("invaid arguments\n");
		return 1;
	}

	/* argv[1] is the dir, argv[2] is the target file */
	return mklmfs(argv[1], argv[2]);
}
