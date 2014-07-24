#ifndef _BDEV_H_
#define _BDEV_H_

#include <os/types.h>
#include <os/list.h>
#include <os/mutex.h>

#define DISK_MAX_PARTITIONS	4
#define BLOCK_SIZE		4096

struct disk_operations {
	int (*read_sectors)(char *buffer, size_t s_sector, int count);
	int (*write_sectors)(char *buffer, size_t s_sector, int count);
};

struct bdev {
	u32 start_sector;
	u32 sector_nr;
	u32 max_start;
	int minor;
	struct disk *disk;
};

struct disk {
	char name[32];
	struct disk_operations *ops;
	int blk_n_sectors;
	int partitions;
	int major;
	struct bdev *bdev[DISK_MAX_PARTITIONS];
	struct list_head bdev_list;
	u32 sector_size;
	u32 sector_nr;
	struct mutex disk_mutex;
};

int register_disk(char *name, struct disk_operations *ops);
struct bdev *disk_get_bdev(u8 major, u8 minor);

int write_sectors(struct bdev *bdev, char *buf,
		u32 start_sectors, size_t count);
int read_sectors(struct bdev *bdev, char *buf,
		u32 start_sectors, size_t count);
#endif
