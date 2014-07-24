#include <os/kernel.h>
#include <os/disk.h>
#include <os/spin_lock.h>

#define MAX_DISK	4

static struct disk *disk_table[MAX_DISK];
int major_current = 0;
SPIN_LOCK_INIT(disk_lock);

int read_sectors(struct bdev *bdev, char *buf,
		u32 start_sector, size_t count)
{
	struct disk *disk = bdev->disk;
	int ret = 0;

	if (start_sector > (bdev->max_start))
		return -EINVAL;

	mutex_lock(&disk->disk_mutex);
	ret = disk->ops->read_sectors(buf, start_sector, count);
	mutex_unlock(&disk->disk_mutex);

	return ret;
}

int write_sectors(struct bdev *bdev, char *buf,
		u32 start_sector, size_t count)
{
	struct disk *disk = bdev->disk;
	int ret = 0;

	if (start_sector > (bdev->max_start))
		return -EINVAL;

	mutex_lock(&disk->disk_mutex);
	ret = disk->ops->write_sectors(buf, start_sector, count);
	mutex_unlock(&disk->disk_mutex);

	return ret;
}

struct bdev *disk_get_bdev(u8 major, u8 minor)
{
	struct bdev *bdev;
	struct disk *disk;

	if (major >= MAX_DISK || minor >= DISK_MAX_PARTITIONS) {
		kernel_error("Disk major and minor to big\n");
		return NULL;
	}
	
	disk = disk_table[major];
	if (!disk)
		return NULL;

	kernel_debug("disk name is %s minor %d\n", disk->name, minor);
	bdev = disk->bdev[minor];

	return bdev;
}

struct bdev *alloc_bdev(void)
{
	struct bdev *bdev;

	bdev = kzalloc(sizeof(struct bdev), GFP_KERNEL);
	if (!bdev)
		return NULL;

	return bdev;
}

static int init_disk(struct disk *disk, struct disk_operations *ops, char *buf)
{
	struct bdev *bdev;
	int i;
	unsigned long flags;

	/* this function can be called in booting time 
	 * so we should use spinlock_irqsave
	 */
	spin_lock_irqsave(&disk_lock, &flags);
	if (major_current == MAX_DISK) {
		spin_unlock_irqstore(&disk_lock, &flags);
		return -ENOMEM;
	}
	disk_table[major_current] = disk;
	major_current++;
	spin_unlock_irqstore(&disk_lock, &flags);

	init_mutex(&disk->disk_mutex);
	disk->ops = ops;
	disk->partitions = 1;
	disk->sector_size = 512;
	disk->sector_nr = 2048;
	disk->blk_n_sectors = (BLOCK_SIZE / disk->sector_size);

	/* parse the MBR data, TBD later, currently only support memdisk */
	for (i = 0; i < 1; i++) {
		bdev = alloc_bdev();
		if (!bdev)
			return -ENOMEM;

		bdev->start_sector = 4;		/* 4096 aligin */
		bdev->sector_nr = 2048;		/* 1M size */
		bdev->minor = i;
		bdev->disk = disk;
		bdev->max_start = (bdev->start_sector + bdev->sector_nr);
		bdev->max_start -= disk->blk_n_sectors;
		
		disk->bdev[i] = bdev;
	}

	return 0;
}

static struct disk *alloc_and_init_disk(struct disk_operations *ops)
{
	struct disk *disk = NULL;
	char *buf;

	disk = kzalloc(sizeof(struct disk), GFP_KERNEL);
	if (!disk)
		return NULL;
	
	buf = get_free_page(GFP_KERNEL);
	if (!buf)
		goto out;

	/* read the first sector MBR */
	if (ops->read_sectors(buf, 0, 1)) {
		kfree(buf);
		goto out;
	}

	if (init_disk(disk, ops, buf)) {
		kfree(buf);
		goto out;
	}

	kfree(buf);
	return disk;

out:
	kfree(disk);
	return NULL;
}

int register_disk(char *name, struct disk_operations *ops)
{
	struct disk *disk;

	if (!ops)
		return -EINVAL;

	disk = alloc_and_init_disk(ops);
	if (!disk) {
		kernel_info("alloc disk failed\n");
		return -ENOMEM;
	}

	if (name)
		memcpy(disk->name, name, strlen(name));

	kernel_debug("exit register disk\n");

	return 0;
}
