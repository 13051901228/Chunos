#include <os/printk.h>
#include <os/types.h>

extern void arch_dump_register(unsigned long sp);

static void dump_register(unsigned long sp)
{
	arch_dump_register(sp);
}

void panic(char *str, unsigned long sp)
{
	kernel_fatal("PANIC: caused by %s loop forever.\n", str);

	dump_register(sp);

	while(1);
}
