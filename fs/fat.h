#ifndef _FAT_H_
#define _FAT_H_

struct fs_info {
	u32 lead_sig;		/* 0 */
	u32 struct_sig;		/* 484 */
	u32 free_count;		/* 488 */
	u32 next_free;		/* 492 */
};

struct fat32_extra {
	u32 fat32_sec_cnt;
	u16 fat32_ext_flag;
	u16 fs_ver;
	u32 root_clus;
	u16 fs_info;
	u16 boot_sec;
	char res[12];
	u8 drv_num;
	u8 res1;
	u8 boot_sig;
	u32 vol_id;
	char vol_lab[11];
	char file_system[8];		/* fat12 or fat16 or fat32 */
};

struct fat16_extra {
	u8 drv_num;
	u8 res;
	u8 boot_sig;
	u32 vol_id;
	char vol_lab[11];
	char file_system[8];		/* fat12 or fat16 or fat32 */
};

struct fat_super_block {
	u8 jmp[3];
	char oem_name[8];
	u16 byts_per_sec;
	u8 sec_per_clus;
	u16 res_sec_cnt;
	u8 fat_num;
	u16 root_ent_cnt;
	u16 total_sec16;
	u8 media;
	u16 fat16_sec_size;
	u16 sec_per_trk;
	u16 num_heads;
	u32 hide_sec;
	u32 total_sec32;
	union _fat_extra {
		struct fat32_extra fat32;
		struct fat16_extra fat16;
	} fat_extra;

	/* to be done */
	struct fs_info info;

	u32 clus_size;		/* bytes_per_sec * sec_per_clus */
	u32 first_data_sector;
	u32 total_sec;
	u16 fat_size;
	u32 data_sec;
	u32 clus_count;
	u8 fat_type;
	u8 fat_offset;
	u32 root_dir_sectors;
	u16 dentry_per_block;
	u16 root_dir_start_sector;
	u16 fat12_16_root_dir_blk;

	/* for fat buffer only for read, if need write
	 * need request a buffer pool from system buffer
	 */
	struct mutex buf_mutex;
	u32 buf_sector_base;
	char *buf;
};

struct fat_fnode {
	u32 first_clus;
	u32 first_sector;
	u32 prev_clus;
	u32 prev_sector;
	u32 current_clus;
	u32 current_sector;
	u32 file_entry_clus;		/* information of the file entry */
	u16 file_entry_pos;
	u16 same_name;
};

struct fat_dir_entry {
	char name[8];		/* name[0] = 0xe5 the dir is empty */
	char externed[3];
	u8 dir_attr;
	u8 nt_res;
	u8 crt_time_teenth;
	u16 crt_time;
	u16 crt_date;
	u16 last_acc_date;
	u16 fst_cls_high;
	u16 write_time;
	u16 write_date;
	u16 fst_cls_low;
	u32 file_size;
} __attribute__((packed));

struct fat_long_dir_entry {
	u8 attr;
	u16 name1[5];
	u8 dir_attr;
	u8 res;
	u8 check_sum;
	u16 name2[6];
	u16 file_start_clus;
	u16 name3[2];
}__attribute__((packed));

struct fat_name {
	char short_name[11];
	char is_long;
	u16 *long_name;
};

#define FIRST_DATA_BLOCK	2

#define FAT_ATTR_READ_ONLY	0x01
#define FAT_ATTR_HIDDEN		0x02
#define FAT_ATTR_SYSTEM		0X04
#define FAT_ATTR_VOLUME_ID	0x08
#define FAT_ATTR_DIRECTORY	0x10
#define FAT_ATTR_ARCHIVE	0x20

#define FAT_ATTR_LONG_NAME	\
	(FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | \
	FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID)

#define FAT_ATTR_LONG_NAME_MASK	\
	(FAT_ATTR_LONG_NAME | FAT_ATTR_DIRECTORY | FAT_ATTR_ARCHIVE)

#endif
