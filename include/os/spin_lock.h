#ifndef _SPIN_LOCK_H
#define	_SPIN_LOCK_H

#include <os/types.h>

typedef struct spin_lock {
#ifdef CONFIG_SMP
	u8 value;
	u8 cpu;
#endif
	unsigned long flags;
} spin_lock_t;

#ifdef CONFIG_SMP
#define SPIN_LOCK_INIT(name)	\
	spin_lock_t name = {	\
		.value	= 0,	\
		.cpu 	= -1,	\
		.flags	= 0,	\
	}
#else
#define SPIN_LOCK_INIT(name)	\
	spin_lock_t name = {	\
		.flags = 0,	\
	}
#endif

void spin_lock_init(spin_lock_t *lock);
void spin_lock(spin_lock_t *lock);
void spin_unlock(spin_lock_t *lock);
void spin_lock_irqsave(spin_lock_t *lock);
void spin_unlock_irqstore(spin_lock_t *lock);

#endif
