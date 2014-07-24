#ifndef _MIRROS_H
#define _MIRROS_H

#include <config/config.h>

/* 
 * user app will loaded to 0x00400000, resever 4k
 * for other useage, such as task argv, and user
 * space stack base is 0x80000000.
 */
#define PROCESS_USER_BASE		0X00400000
#define PROCESS_USER_STACK_BASE		0x80000000
#define PROCESS_USER_EXEC_BASE		0x00401000

/* 1M memory for mmap zone */
#define PROCESS_USER_MMAP_SIZE		0x400000
#define PROCESS_USER_MMAP_BASE		(0x40000000 - 0x100000)

#endif
