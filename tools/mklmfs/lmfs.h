#ifndef _LMFS_H
#define _LMFS_H

#include "types.h"

struct lmfs_inode_entry {
	u32 flag;			/* flag of this file */
	u32 mode;			/* mode of the file */
	u32 type;			/* type of this file */
	u32 create_ymd;			/* create time of this file ymd */
	u32 create_hms;			/* create time this file hms */
	u32 modify_ymd;			/* modify time of this file */
	u32 modify_hms;			/* modify time of this file */
	u32 size;			/* size of this file */
	int file_nr;
	block_t block_nr;		/* how many blocks have allocate */
	int pos_in_lvl1;
	int pos_in_lvl0;
	u32 lvl1_block[64];
};

struct lmfs_dir_entry {
	char name[248];			/* name of the file */
	int type;
	u32 inode_index;		/* the inode number of the file */
};

struct lmfs_super_block {
	u32 magic;			/* magic of the lmfs partition always LMFS */
	u32 n_blocks;			/* how may blocks of this partitions */
	u32 start_block;		/* start blocks of this partition in the disk */
	u32 super_block_size;		/* size of super block always 4k for LMFS */

	u32 ib_pool_start_block;
	u32 ib_pool_current_block;
	u32 ib_pool_pos;
	u32 ib_pool_block_nr;

	u32 db_pool_start_block;
	u32 db_pool_current_block;
	u32 db_pool_pos;
	u32 db_pool_block_nr;

	u32 inode_start_block;		/* start block of the inode erae */
	u32 inode_free;			/* how many free inodes */
	u32 inode_n_blocks;		/* how many blocks for inode */
	u32 inode_nr;			/* how many inode this partition has */
	u32 data_start_block;
	u32 data_free_blocks;

	u32 root_inode_index;		/* 0 */

	u32 *ib_pool;
	u32 *db_pool;
};

#endif
