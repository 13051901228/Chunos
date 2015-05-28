#ifndef _CONFIG_H
#define _CONFIG_H

#define CONFIG_LOG_LEVEL		3

#define HZ				100

#define IRQ_NR				512

#define SYSTEM_TIMER_IRQ_NUMBER		14

/* update diet-libc to 0.33, using EABI */
//#define CONFIG_SYSCALL_OLD_EABI

#define SLAB_MAX_SIZE			10

#define CONFIG_DEBUG_MM
#define CONFIG_DEBUG_SLAB
#define CONFIG_DEBUG_PROCESS
#define CONFIG_DEBUG_SCHED
#define CONFIG_DEBUG_FS

#define CONFIG_HAS_ARCH_MEMSET
#define CONFIG_HAS_ARCH_MEMCPY

#define CONFIG_EARLY_CONSOLE_BASE	(0x40000000)

#endif
