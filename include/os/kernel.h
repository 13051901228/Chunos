#ifndef _KERNEL_H
#define _KERNEL_H

#include <os/types.h>
#include <config/config.h>
#include <os/printk.h>
#include <os/bitops.h>
#include <os/list.h>
#include <os/bound.h>
#include <os/slab.h>
#include <os/string.h>
#include <os/task.h>
#include <os/mm.h>
#include <os/errno.h>
#include <os/io.h>

static inline void lock_kernel(void)
{
	disable_irqs();
}

static inline void unlock_kernel(void)
{
	enable_irqs();
}

static inline void lock_kernel_irqsave(unsigned long *flags)
{
	enter_critical(flags);
}

static inline void unlock_kernel_irqstore(unsigned long *flags)
{
	exit_critical(flags);
}

#endif
