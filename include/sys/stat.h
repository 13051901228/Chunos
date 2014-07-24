#ifndef _STAT_H_
#define _STAT_H_

struct stat {
	u32		st_dev;
	unsigned long	st_ino;
	u16	st_mode;
	u16	st_nlink;
	u16	st_uid;
	u16	st_gid;
	u32	st_rdev;
	unsigned long	st_size;
	unsigned long	st_blksize;
	unsigned long	st_blocks;
	time_t		st_atime;
	unsigned long	st_atime_nsec;
	time_t		st_mtime;
	unsigned long	st_mtime_nsec;
	time_t		st_ctime;
	unsigned long	st_ctime_nsec;
	unsigned long	__unused4;
	unsigned long	__unused5;
};

#endif
