#ifndef _DIRENT_H_
#define _DIRENT_H_

struct os_dirent {
	long		d_ino;
	offset_t	d_off;
	unsigned short	d_reclen;
	char		d_name[1];
};

struct dirent {
	char d_type;
	char name[256];
};

#endif
