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
#include "include/gdt.h"

extern void x86_idt_init(void);
extern void x86_switch_page_table(unsigned long base);
extern void x86_setup_tss(struct task_struct *task);
extern void tss_init(void);

void inline arch_disable_irqs(void)
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

void inline arch_flush_mmu_tlb(void)
{
	__asm ("wbinvd");
}

void inline arch_flush_cache(void)
{
	__asm ("wbinvd");
}

void arch_enable_irqs(void)
{
	__asm ("sti");
}

void arch_init_pt_regs(pt_regs *regs, void *fn, void *arg)
{
	if (fn) {
		/* this is for kernel thread interrupt is enabled*/
		regs->eflags = 0x200;
		regs->cs = KERNEL_CS;
		regs->eip = (unsigned long)fn;
		regs->error_code = 0;
		regs->eax = (unsigned long)arg;
		regs->ecx = 0x0;
		regs->edx = 0x0;
		regs->ebx = 0x0;
		regs->esp_cr0 = 0x0;
		regs->ebp = 0;
		regs->esi = 0;
		regs->edi = 0;
		regs->esp_cr3 = 0;
		regs->ss = KERNEL_SS;
		regs->es = KERNEL_ES;
		regs->ds = KERNEL_DS;
		regs->fs = KERNEL_FS;
		regs->gs = KERNEL_GS;
	} else {
		/* this is for user space */
		regs->eflags = 0x200;
		regs->cs = USER_CS;
		regs->eip = PROCESS_USER_EXEC_BASE;
		regs->error_code = 0;
		regs->eax = (unsigned long)arg;
		regs->ecx = 0x0;
		regs->edx = 0x0;
		regs->ebx = PROCESS_USER_META_BASE;
		regs->esp_cr0 = 0x0;
		regs->ebp = 0;
		regs->esi = 0;
		regs->edi = 0;
		regs->esp_cr3 = PROCESS_USER_STACK_BASE;
		regs->ss = USER_SS;
		regs->es = USER_ES;
		regs->ds = USER_DS;
		regs->fs = USER_FS;
		regs->gs = USER_GS;
	}
}

unsigned long arch_set_up_task_stack(unsigned long *stack_base, pt_regs *regs)
{
	/* resever 8 bytes for user esp and ss */
	*(--stack_base) = regs->ss;
	*(--stack_base) = regs->esp_cr3;

	*(--stack_base) = regs->eflags;
	*(--stack_base) = regs->cs;
	*(--stack_base) = regs->eip;
	*(--stack_base) = regs->error_code;
	*(--stack_base) = regs->eax;
	*(--stack_base) = regs->ecx;
	*(--stack_base) = regs->edx;
	*(--stack_base) = regs->ebx;
	*(--stack_base) = regs->esp_cr0;
	*(--stack_base) = regs->ebp;
	*(--stack_base) = regs->esi;
	*(--stack_base) = regs->esi;
	*(--stack_base) = regs->esp_cr3;
	*(--stack_base) = regs->ss;
	*(--stack_base) = regs->es;
	*(--stack_base) = regs->ds;
	*(--stack_base) = regs->fs;
	*(--stack_base) = regs->gs;

	return (unsigned long)stack_base;
}

void arch_set_mode_stack(u32 base, u32 mode)
{

}

int arch_irq_init(void)
{
	return 0;
}

void arch_set_up_task_return_value(int value, pt_regs *reg)
{
	reg->eax = value;
}

int arch_init(void)
{
	tss_init();
	return 0;
}

void arch_dump_register(unsigned long sp)
{

}

pt_regs *arch_get_pt_regs(void)
{
	return 0;
}

u32 arch_build_tlb_des(unsigned long pa, u32 attr)
{
	return 0;
}

int arch_early_init(void)
{
	x86_idt_init();
	return 0;
}

void x86_switch_task(struct task_struct *cur, struct task_struct *next)
{
	if (cur == next)
		return;

	if (task_is_user(next)) {
		x86_setup_tss(next);
		arch_flush_cache();
		x86_switch_page_table(next->mm_struct.page_table.pde_base_pa);
	}
}
