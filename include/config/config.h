#ifndef _CONFIG_H
#define _CONFIG_H

#define LOG_LEVEL			3

#define HZ				100

#define IRQ_NR				512

#define SYSTEM_TIMER_IRQ_NUMBER		14

/* update diet-libc to 0.33, using EABI */
//#define SYSCALL_OLD_EABI		1

#define SLAB_MAX_SIZE			10

#define DEBUG_MM			1
#define DEBUG_SLAB			1
#define DEBUG_PROCESS			1
#define DEBUG_SCHED			1
#define DEBUG_MMU			1
#define DEBUG_FS			1

#endif
