#ifndef __SYS_MOUNT_H
#define __SYS_MOUNT_H

#define MS_RDONLY	1
#define MS_NOSUID	2
#define MS_NODEV	4
#define MS_NOEXEC	8
#define MS_SYNCHRONOUS	16
#define MS_REMOUNT	32
#define MS_MANDLOCK	64
#define S_WRITE		128
#define S_APPEND	256
#define S_IMMUTABLE	512
#define MS_NOATIME	1024
#define MS_NODIRATIME	2048
#define MS_BIND		4096
#define MS_MOVE		8192
#define MS_REC		16384
#define MS_VERBOSE	32768
#define MS_SILENT	32768
#define MS_POSIXACL	(1 << 16)
#define MS_PRIVATE	(1 << 18)
#define MS_SLAVE	(1 << 19)
#define MS_SHARED	(1 << 20)
#define MS_RELATIME	(1 << 21)
#define MS_KERNMOUNT	(1 << 22)
#define MS_I_VERSION	(1 << 23)
#define MS_STRICTATIME	(1 << 24)
#define MS_NOSEC	(1 << 28)
#define MS_BORN		(1 << 29)
#define MS_ACTIVE	(1 << 30)
#define MS_NOUSER	(1 << 31)

#endif
