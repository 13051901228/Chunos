#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include <os/types.h>
#include <asm/asm_irq.h>

struct irq_des {
	int (*fn)(void *arg);
	void *arg;
};

struct irq_chip	{
	int irq_nr;
	int (*enable_irq)(int nr, u32 flags);
	int (*get_irq_nr)(void);
	int (*disable_irq)(int nr);
	int (*clean_irq_pending)(int nr);
	int (*irq_init)(struct irq_chip *chip);
};

extern int in_interrupt;

void disable_irqs(void);
void enable_irqs(void);
void enter_critical(unsigned long *val);
void exit_critical(unsigned long *val);
int register_irq(int nr, int (*fn)(void *arg), void *arg);

#endif
