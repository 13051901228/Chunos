#include <os/printk.h>
#include <os/kernel.h>
#include <asm/asm_sched.h>
#include <os/sched.h>

void arch_dump_register(unsigned long sp)
{
	pt_regs *regs;

	if (!sp)
		return;

	regs = (pt_regs *)sp;
	if (current)
		printk("current process %s\n", current->name);

	printk("register value:\n");
	printk("	r0:0x%x  r1:0x%x  r2:0x%x r3:0x%x\n",
			regs->r0, regs->r1, regs->r2, regs->r3);
	printk("	r4:0x%x  r5:0x%x  r6:0x%x r7:0x%x\n",
			regs->r4, regs->r5, regs->r6, regs->r7);
	printk("	r8:0x%x  r9:0x%x  r110:0x%x r11:0x%x\n",
			regs->r8, regs->r9, regs->r10, regs->r11);
	printk("	r12:0x%x  sp:0x%x  lr:0x%x pc:0x%x\n",
			regs->r12, regs->sp, regs->lr, regs->pc);
	printk("	spsr:0x%x	cpsr:%x\n", regs->spsr, regs->cpsr);
}
