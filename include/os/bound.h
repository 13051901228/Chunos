#ifndef _BOUND_H
#define _BOUND_H

#define ARRAY_SIZE(array)	(sizeof(array) / sizeof(array[0]))

#define baligin(num,size) 	((num + size - 1) & ~(size - 1))
#define daligin(num,size) 	(((num + size - 1) / size) * size)
#define min_aligin(num,size)	((num) & ~(size - 1))

#define SIZE_1K			(1024)
#define SIZE_K(n)		(SIZE_1K * n)
#define SIZE_4K			(4 * 1024)

#define SIZE_1M			(1 * 1024 * 1024)
#define SIZE_NM(n)		(SIZE_1M * n)

#define SIZE_1G			(1 * 1024 * 1024 * 1024)
#define SIZE_NG(n)		(SIZE_1G * n)

#define ALIGIN_SIZE		4


static inline int is_aligin(unsigned long base, int bound)
{
	return ((base & (bound - 1)) == 0);
}

#endif
