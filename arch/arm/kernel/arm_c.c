#include <os/init.h>
#include <os/types.h>
#include <os/printk.h>
#include <asm/asm_mmu.h>
#include <os/mirros.h>
#include <os/mm.h>
#include <os/mmu.h>
#include "include/arm.h"
#include <os/irq.h>
#include <os/errno.h>
#include <os/string.h>
#include <os/io.h>
#include <asm/asm_sched.h>
#include <os/sched.h>
#include <os/panic.h>
#include <os/task.h>
#include "include/kconfig.h"

/* code for libgcc.a */
int raise(int signum)
{
	printk("raise: Signal %d catched \n",signum);

	return 0;
}

/* code for arm920t mmu */
unsigned long arch_get_tlb_base(void)
{
	return KERNEL_BASE_ADDRESS;	
}

u32 arch_build_tlb_des(unsigned long pa, u32 attr)
{
	u32 ret_val;

	/*
	 * note if you use if(attr & 0x03 == 0x01) there is syntax error
	 * you need add a (attr & 0x03), remember it.
	 * 0x01 means 1level page table
	 * 0x02 means 2level page table
	 */
	if ((attr & 0x03) == 0X01)
		ret_val = (pa & 0xfffffc00) | attr;
	else
		ret_val = (pa & 0xfff00000) | attr;

	return ret_val;
}

u32 arch_build_page_table_des(unsigned long pa, u32 attr)
{
	return ((pa & 0xfffff000) | attr);
}

u32 arch_get_tlb_attr(u32 flag)
{
	u32 attr = 0;

	switch (flag & TLB_ATTR_MEMORY_MASK) {
		case TLB_ATTR_KERNEL_MEMORY:
			attr |= MST_WB_AP_SRW_UN;
			break;

		case TLB_ATTR_DMA_MEMORY:
		case TLB_ATTR_IO_MEMORY:
			attr |= MST_NCNB_AP_SRW_UN;
			break;

		case TLB_ATTR_USER_MEMORY:
			attr |= CURDE_PAGE_TABLE;
			break;

		default:
			attr |= MST_WB_AP_SRW_UN;
			break;
	}

	return attr;
}

u32 arch_get_page_table_attr(u32 flag)
{
	return PGT_DES_DEFAULT;
}

static void arm920t_invalidate_all_tlb(void)
{
	asm(
		"push {r0}\n\t"
		"mov r0,#0\n\t"
		"mcr p15,0,r0,c8,c7,0\n\t"
		"nop\n\t"
		"nop\n\t"
		"pop {r0}\n\t"
	);
}

static void arm920t_clean_icache(void)
{
	asm(
		"push {r0}\n\t"
		"mov r0,#0\n\t"
		"mcr p15,0,r0,c7,c5,0\n\t"
		"nop\n\t"
		"nop\n\t"
		"pop {r0}\n\t"
	);

}

static void arm920t_drain_wbuffer(void)
{
	asm(
		"push {r0}\n\t"
		"mov r0,#0\n\t"
		"mcr p15,0,r0,c7,c10,4\n\t"
		"nop\n\t"
		"nop\n\t"
		"pop {r0}\n\t"
	);
}

static void arm920t_invalidata_all_cache(void)
{
	asm(
		"push {r0}\n\t"
		"mov r0,#0\n\t"
		"mcr p15,0,r0,c7,c7,0\n\t"
		"nop\n\t"
		"nop\n\t"
		"pop {r0}\n\t"
	);
}

#if 0
static void arm920t_invalidata_icache(void)
{
	asm(
		"push {r0}\n\t"
		"mov r0,#0\n\t"
		"mcr p15,0,r0,c7,c5,0\n\t"
		"nop\n\t"
		"nop\n\t"
		"pop {r0}\n\t"
	);
}

static void arm920t_invalidata_dcache(void)
{
	asm(
		"push {r0}\n\t"
		"mov r0,#0\n\t"
		"mcr p15,0,r0,c7,c6,0\n\t"
		"nop\n\t"
		"nop\n\t"
		"pop {r0}\n\t"
	);
}
#endif

void arch_flush_cache(void)
{
	arm920t_clean_icache();
	arm920t_drain_wbuffer();
	arm920t_invalidata_all_cache();
}

void inline arch_flush_mmu_tlb(void)
{
	arm920t_invalidate_all_tlb();
}

/* code for irq and interrupt for arm920t */
void arch_disable_irqs(void)
{
	asm (
		"push {r1}\n\t"
		"mrs r1,cpsr\n\t"
		"orr r1, r1, #0xc0\n\t"
		"msr cpsr_c, r1\n\t"
		"pop {r1}\n\t"
	);	
}

void arch_enable_irqs(void)
{
	asm (
		"push {r1}\n\t"
		"mrs r1,cpsr\n\t"
		"bic r1, r1, #0xc0\n\t"
		"msr cpsr_c, r1\n\t"
		"pop {r1}\n\t"
	);	
}

void arch_enter_critical(unsigned long *val)
{
	asm (
		"push {r1}\n\t"
		"mrs r1, cpsr\n\t"
		"str r1, [r0]\n\t"
		"orr r1, r1, #0xc0\n\t"
		"msr cpsr_c, r1\n\t"
		"pop {r1}\n\t"
	);
}

void arch_exit_critical(unsigned long *val)
{
	asm(
		"push {r1}\n\t"
		"ldr r1, [r0]\n\t"
		"msr cpsr_c, r1\n\t"
		"pop {r1}\n\t"
	);	
}

static void arm_remap_vector(void)
{
	asm(
		"push {r0}\n\t"
		"mrc p15, 0, r0, c1, c0, 0\n\t"
		"orr r0, #1<<13\n\t"
		"mcr p15, 0, r0, c1, c0, 0\n\t"
		"pop {r0}\n\t"
	);
}

int exception_trap_init(void)
{
	extern char _vector_start[];
	extern char _vector_end[];
	void *vector_page;

	/*
	 * first we get a page load the vector code,notice:
	 * since the vector must remaped at 0xffff0000, we also 
	 * need map 0xfff00000 to the page section address which
	 * we get.
	 */
	vector_page = get_free_page_aligin(0xffff0000, GFP_KERNEL);
	kernel_debug("Get vector address is 0x%x\n", (u32)vector_page);
	if(vector_page == NULL){
		kernel_error("faild get page for trap\n");
		return -ENOMEM;
	}

	/*
	 * copy the vector code to the page we get. then map 0xfff00000
	 * to va_to_pa(vector_page) & 0xfff00000. at the last chage the
	 * value of bit13 of cp register c1 to indicate that we need remap
	 * the vector.
	 */
	memcpy(vector_page, (void *)_vector_start, _vector_end - _vector_start);
	build_tlb_table_entry(0xfff00000,
			      va_to_pa((unsigned long)vector_page) & 0xfff00000,
			      SIZE_1M,
			      TLB_ATTR_KERNEL_MEMORY);

	arm_remap_vector();

	return 0;
}

/*
 * when run a kernel thread, we need this function to 
 * init the values of each registers.
 */
void arch_init_pt_regs(pt_regs *regs, void *fn, void *arg)
{
	/*
	 * if fn != NULL, this means a kernel thread, other
	 * wise it is called by exec(), fill correct value
	 * to each register. kernel_stack will store at
	 * the first member of tash_struct, so we fill it to
	 * 0
	 */
	if (fn) {
		regs->r0 = (u32)arg;
		regs->r1 = 1;
		regs->sp = 13;
		regs->pc = (u32)fn;
		regs->cpsr = SVC_MODE;
	} else {
		/*
		 * calculate how many arguments passed to this
		 * program.
		 */
		regs->r0 = 0;
		regs->r1 = PROCESS_USER_BASE;
		regs->sp = PROCESS_USER_STACK_BASE - (int)arg;
		regs->pc = PROCESS_USER_EXEC_BASE;
		regs->cpsr = USER_MODE;
	}

	regs->r2 = 2;
	regs->r3 = 3;
	regs->r4 = 4;
	regs->r5 = 5;
	regs->r6 = 6;
	regs->r7 = 7;
	regs->r8 = 8;
	regs->r9 = 9;
	regs->r10 = 10;
	regs->r11 = 11;
	regs->r12 = 12;
	regs->lr = 14;
	regs->spsr = 15;
}

int arch_set_up_task_stack(struct task_struct *task, pt_regs *regs)
{
	u32 *stack_base;

	/*
	 * in arm, stack is grow from up to down,so we 
	 * adjust it;
	 */
	if (task->stack_base == NULL) {
		kernel_error("task kernel stack invailed\n");
		return -EFAULT;
	}
	task->stack_base = task->stack_origin;
	task->stack_base += KERNEL_STACK_SIZE;
	stack_base = task->stack_base;
	
	/*
	 * when switch task, the context will follow
	 * below stucture.
	 */
	kernel_debug("regs->pc 0x%x regs->sp 0x%x\n", regs->pc, regs->sp);
	*(--stack_base) = regs->pc;
	*(--stack_base) = regs->cpsr;
	*(--stack_base) = regs->sp;
	*(--stack_base) = regs->lr;
	*(--stack_base) = regs->spsr;
	*(--stack_base) = regs->r12;
	*(--stack_base) = regs->r11;
	*(--stack_base) = regs->r10;
	*(--stack_base) = regs->r9;
	*(--stack_base) = regs->r8;
	*(--stack_base) = regs->r7;
	*(--stack_base) = regs->r6;
	*(--stack_base) = regs->r5;
	*(--stack_base) = regs->r4;
	*(--stack_base) = regs->r3;
	*(--stack_base) = regs->r2;
	*(--stack_base) = regs->r1;
	*(--stack_base) = regs->r0;

	task->stack_base = (void *)stack_base;

	return 0;
}

int arch_set_task_return_value(pt_regs *reg,
		struct task_struct *task)
{
	reg->r0 = task->pid;

	return 0;
}

void data_abort_handler(unsigned long sp)
{
	panic("Data aboart exception\n", sp);
}

void prefetch_abort_handler(unsigned long sp)
{
	panic("Prefetch abort exception\n", sp);
}

void arch_set_mode_stack(u32 base, u32 mode)
{
	/* r0 = base, r1=mode */
	asm(
		"push {r2}\n\t"
		"mrs r2,cpsr\n\t"
		"msr cpsr_c, r1\n\t"
		"mov sp, r0\n\t"
		"msr cpsr, r2\n\t"
		"pop {r2}"
	);
}

/* do some arch irq releate init work */
int arch_irq_init(void)
{
	return 0;
}

int arch_init(void)
{
	int ret;

	ret = exception_trap_init();
	if (ret) {
		kernel_error("Cant remap exception vector to \
			      0xfffff000 using default setting\n");
	}

	arch_irq_init();

	return 0;
}
