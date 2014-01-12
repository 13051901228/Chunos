#ifndef _ASM_INTERRUPT_H
#define _ASM_INTERRUPT_H

void arch_disable_irqs(void);
void arch_enable_irqs(void);
struct irq_des *arch_get_irq_description(int nr);
int arch_register_irq(int nr, int (*fn)(void *arg), void *arg);
void arch_enter_critical(unsigned long *val);
void arch_exit_critical(unsigned long *val);

#endif
