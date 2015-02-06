#include <os/printk.h>
#include <os/types.h>

extern void arch_dump_register(unsigned long sp);

unsigned long get_sp(void)
{
	return 0;
}

static void dump_register(unsigned long sp)
{
	arch_dump_register(sp);
}

void _panic(char *str, unsigned long sp)
{
	kernel_fatal("PANIC: caused by %s loop forever\n", str);
	dump_register(sp);
	while(1);
}

void panic(char *str)
{
	/* get_sp() */
	_panic(str, get_sp());
}
