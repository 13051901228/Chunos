#include <os/kernel.h>
#include <os/disk.h>

#define MEMDISK_SECTOR_SIZE	512

unsigned long __ramdisk_start;

#define MEMDISK_ADDRESS(sector)	\
	(char *)(__ramdisk_start + sector * MEMDISK_SECTOR_SIZE)

int memdisk_read_sectors(char *buffer, u32 s_sector, int count)
{
	memcpy(buffer, MEMDISK_ADDRESS(s_sector), count * MEMDISK_SECTOR_SIZE);

	return 0;
}

int memdisk_write_sectors(char *buffer, u32 s_sector, int count)
{
	memcpy(MEMDISK_ADDRESS(s_sector), buffer, count * MEMDISK_SECTOR_SIZE);

	return 0;
}

struct disk_operations memdisk_ops = {
	.read_sectors = memdisk_read_sectors,
	.write_sectors = memdisk_write_sectors,
};

int memdisk_init(void)
{
	kernel_debug("Init ramdisk, Ramdisk base address is 0x%x\n",
			__ramdisk_start);
	return register_disk("memdisk", &memdisk_ops);
}

device_initcall(memdisk_init);
