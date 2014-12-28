#include <os/irq.h>
#include <os/types.h>
#include <os/printk.h>
#include <os/errno.h>
#include <config/config.h>
#include <os/soc.h>
#include <os/mm.h>
#include <os/string.h>
#include <os/panic.h>
#include <asm/asm_sched.h>
#include <os/task.h>

void arch_disable_irqs(void)
{
	asm ("cli\n\t");
}

unsigned long arch_get_tlb_base(void)
{
	return 0;
}

u32 arch_build_page_table_des(unsigned long pa, u32 attr)
{
	return 0;
}

u32 arch_get_tlb_attr(u32 flag)
{
	return 0;
}

u32 arch_get_page_table_attr(u32 flag)
{
	return 0;
}

void arch_flush_cache(void)
{

}

void inline arch_flush_mmu_tlb(void)
{

}

void arch_enable_irqs(void)
{

}

void arch_enter_critical(unsigned long *val)
{
	
}

void arch_exit_critical(unsigned long *val)
{

}

void arch_init_pt_regs(pt_regs *regs, void *fn, void *arg)
{

}

int arch_set_up_task_stack(struct task_struct *task, pt_regs *regs)
{
	return 0;
}

void arch_set_mode_stack(u32 base, u32 mode)
{

}

int arch_irq_init(void)
{
	return 0;
}

int arch_init(void)
{
	/* setup idt and other */
	return 0;
}

void arch_dump_register(unsigned long sp)
{

}

pt_regs *arch_get_pt_regs(void)
{
	return 0;
}

void arch_switch_task_sw(void)
{

}

u32 arch_build_tlb_des(unsigned long pa, u32 attr)
{
	return 0;
}

int uart_puts(char *str)
{
	return 0;
}
