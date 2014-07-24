#ifndef _LMFS_H
#define _LMFS_H

#include <os/types.h>

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

	u32 fnode_start_block;		/* start block of the fnode erae */
	u32 fnode_free;			/* how many free fnodes */
	u32 fnode_n_blocks;		/* how many blocks for fnode */
	u32 fnode_nr;			/* how many fnode this partition has */
	u32 data_start_block;
	u32 data_free_blocks;

	u32 root_fnode_index;		/* 0 */

	u32 *ib_pool;
	u32 *db_pool;
};

struct lmfs_fnode {
	u32 fnode_index;		/* the index of the file in this partition */
	u32 *lvl1;			/* 4096 bytes */
	u32 data_block[64];		/* 256 bytes, max size = 64 * 4096 *4096 */
	s32 pos_in_lvl1;
	s32 pos_in_lvl0;
	u32 current_block;
	u32 block_count;
	int file_nr;
	u32 *bitmap;
	u32 *lvl0;
};

struct lmfs_dir_entry {
	char name[248];			/* name of the file */
	int type;
	u32 fnode_index;		/* the fnode number of the file */
};

/* stored on the disk will convert to indoe takes up 512byte*/
struct lmfs_fnode_entry {
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
	s32 pos_in_lvl1;
	s32 pos_in_lvl0;
	u32 reserve[52];
	u32 data_block[64];
};

#endif
