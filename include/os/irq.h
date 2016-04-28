#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include <os/types.h>
#include <asm/asm_irq.h>

struct irq_des {
	int (*fn)(void *arg);
	void *arg;	
};

struct irq_chip	{
	unsigned int irq_nr;
	void (*unmask)(unsigned int nr);
	int (*get_irq_nr)(void);
	void (*mask)(unsigned int nr);
	void (*eoi)(unsigned int nr);
	void (*init)(struct irq_chip *chip);
	void (*set_affinity)(unsigned int irq, unsigned int affinity);
	void (*setup)(unsigned int irq,
			unsigned int vector, unsigned int affinity);
};

extern int in_interrupt;

void disable_irqs(void);
void enable_irqs(void);
void enter_critical(unsigned long *val);
void exit_critical(unsigned long *val);
int register_irq(int nr, int (*fn)(void *arg), void *arg);

#endif
