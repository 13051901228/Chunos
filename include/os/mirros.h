#ifndef _MIRROS_H
#define _MIRROS_H

#include <config/config.h>

/********************************
 * ---------------------- 0x8000000
 *  user process stack	  256M
 * ---------------------- 0x7000000
 *  reseverd              256M
 * ---------------------- 0x6000000
 *  reseverd		  256M
 * ---------------------- 0x5000000
 *  mmap & heap           256M
 * ---------------------- 0x4000000
 *  text and data & bss.. 1G - 4K -1K
 * ---------------------- 0x0040100
 *  argv and env etc	  1K
 * ---------------------- 0x0040000
 *  not map for NULL      4K
 * ---------------------- 0x0000000
 */

/* 
 * user app will loaded to 0x00400000, resever 4k
 * for other useage, such as task argv, and user
 * space stack base is 0x80000000.
 */
#define PROCESS_USER_BASE		0X00400000
#define PROCESS_USER_STACK_BASE		0x80000000
#define PROCESS_STACK_MAX_SIZE		0x10000000
#define PROCESS_USER_EXEC_BASE		0x00401000

/* 1M memory for mmap zone */
#define PROCESS_USER_MMAP_SIZE		0x00400000
#define PROCESS_USER_MMAP_MAX_SIZE	0x10000000
#define PROCESS_USER_MMAP_BASE		(0x40000000)

#endif
