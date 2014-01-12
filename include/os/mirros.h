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

#endif
