#include <os/irq.h>
#include <os/types.h>
#include <os/printk.h>
#include <os/errno.h>
#include <config/config.h>
#include <os/soc.h>
#include <os/mm.h>
#include <os/string.h>
#include <os/panic.h>

static void *irq_table_base = NULL;
static struct irq_chip *irq_chip;

extern void arch_enter_critical(unsigned long *val);
extern void arch_exit_critical(unsigned long *val);

int in_interrupt = 0;

void enable_irqs(void)
{
	arch_enable_irqs();
}

void disable_irqs(void)
{
	arch_disable_irqs();
}

void enable_irq(int nr)
{
	irq_chip->unmask(nr);
}

void disable_irq(int nr)
{
	irq_chip->mask(nr);
}

void inline enter_critical(unsigned long *val)
{
	arch_enter_critical(val);
}

void inline exit_critical(unsigned long *val)
{
	arch_exit_critical(val);
}

static struct irq_des *get_irq_description(int nr)
{
	return ((struct irq_des*)irq_table_base) + nr;
}

void free_irq(int nr)
{
	unsigned long flags;
	struct irq_des *des;

	if (nr > (IRQ_NR - 1))
		return;

	enter_critical(&flags);

	des = get_irq_description(nr);
	if ((des) && (des->fn)) {
		des->fn = NULL;
		des->arg = NULL;
		irq_chip->mask(nr);
	}

	exit_critical(&flags);
}

int register_irq(int nr, int (*fn)(void *arg), void *arg)
{
	int err = -EINVAL;
	unsigned long flags;
	struct irq_des *des;

	if ((nr > (IRQ_NR - 1)) || (fn == NULL))
		return -EINVAL;

	enter_critical(&flags);

	des = get_irq_description(nr);
	if ((des) && (des->fn == NULL)) {
		des->fn = fn;
		des->arg = arg;
		irq_chip->unmask(nr);
		err = 0;
	}

	exit_critical(&flags);
	return err;
}

int do_irq_handler(int nr)
{
	struct irq_des *des;
	int ret;

	des = get_irq_description(nr);
	if (!des) {
		return -EINVAL;
	}
	
	ret = des->fn(des->arg);

	return ret;
}

void irq_handler(void)
{
	int nr;

	in_interrupt = 1;

	/* first get irq number, then clean irq pending */
	nr = irq_chip->get_irq_nr();
	do_irq_handler(nr);
	irq_chip->eoi(nr);

	in_interrupt = 0;
}

int irq_init(void)
{
	int nr;
	int irq_table_size = IRQ_NR * sizeof(struct irq_des);
	struct soc_platform *platform = get_platform();

	kernel_info("System IRQ init...\n");

	nr = page_nr(irq_table_size);
	irq_table_base = get_free_pages(nr, GFP_KERNEL);
	if(irq_table_base == NULL){
		kernel_error("Can not allocate size for irq_table\n");
		return -ENOMEM;
	}

	memset(irq_table_base, 0, nr << PAGE_SHIFT);

	if (platform->irq_chip) {
		irq_chip = platform->irq_chip;
		if (irq_chip->init)
			irq_chip->init(irq_chip);
	}
	else {
		panic("No irq chip found\n");
	}

	return 0;
}
