#ifndef _TYPES_H
#define _TYPES_H

typedef unsigned int		_u32;
typedef unsigned short		_u16;
typedef unsigned char		_u8;
typedef unsigned long		_u32l;
typedef unsigned long long	_u64;
typedef int			_s32;
typedef short			_s16;
typedef char			_s8;
typedef long			_s32l;
typedef long long		_s64;

typedef _u32			u32;
typedef _u16			u16;
typedef _u8			u8;
typedef	_u32l			u32l;
typedef _u64			u64;
typedef _s32			s32;
typedef _s16			s16;
typedef _s8			s8;
typedef	_s32l			s32l;
typedef _s64			s64;

typedef int			pid_t;

typedef int			irq_t;

typedef unsigned long		address_t;
typedef unsigned long 		stack_t;
typedef u32			offset_t;

typedef u32			block_t;

#define MAX(a, b)	a > b ? a : b
#define MIN(a, b)	a < b ? a : b

#define NULL ((void *)0)

#define container_of(ptr,name,member) \
	(name *)((unsigned char *)ptr - ((unsigned char *)&(((name *)0)->member)))

#define bit(nr) (1 << nr)

#define __user

#endif
