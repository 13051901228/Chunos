#ifndef _RAM_DISK_H
#define _RAM_DISK_H

#define FILE_NAME_SIZE 32

#define u32 unsigned int 
#define s32 int

struct ramdisk_header {
	char name[16];
	s32 total_size;
	s32 file_count;	
	u32 unused;
	u32 unused2;
};

struct file_header {
	char name[FILE_NAME_SIZE];
	u32 base;
	u32 size;
};

struct file {
	u32 base;
	u32 size;
	u32 curr;
};

#endif
