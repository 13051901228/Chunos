/*
 * arch/x86/kernel/ioapic.c
 *
 * Created by Le Min at 2014/12/19
 */

#include <os/kernel.h>
#include <os/slab.h>
#include <os/irq.h>

int ioapic_enable_irq(int nr, int flag)
{
	return 0;
}

int ioapic_get_irq_nr(void)
{
	return 0;
}

int ioapic_disable_irq(int nr)
{
	return 0;
}

int ioapic_clean_irq_pending(int nr)
{
	return 0;
}

int ioapic_irqchip_init(struct irq_chip *chip)
{
	return 0;
}

struct irq_chip ioapic = {
	.irq_nr			= 256,
	.enable_irq 		= ioapic_enable_irq,
	.get_irq_nr		= ioapic_get_irq_nr,
	.disable_irq		= ioapic_disable_irq,
	.clean_irq_pending 	= ioapic_clean_irq_pending,
	.irq_init		= ioapic_irqchip_init,
};
