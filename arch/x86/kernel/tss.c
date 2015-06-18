/*
 * arch/x86/kernel/idt.c
 *
 * Created By Le Min at 2015/01/03
 */
#include "include/idt.h"
#include <os/kernel.h>
#include <os/init.h>
#include "include/x86_config.h"
#include "include/gdt.h"
#include <os/mm.h>

struct x86_tss {
	unsigned short p_task_link, res_l;
	unsigned long esp0;
	unsigned short ss0, res_ss0;
	unsigned long esp1;
	unsigned long ss1, res_ss1;
	unsigned long esp2;
	unsigned long ss2, res_ss2;
	unsigned long cr3;
	unsigned long eip;
	unsigned long eflags;
	unsigned long eax;
	unsigned long ecx;
	unsigned long edx;
	unsigned long ebx;
	unsigned long esp;
	unsigned long ebp;
	unsigned long esi;
	unsigned long edi;
	unsigned short es, res_es;
	unsigned short cs, res_cs;
	unsigned short ss, res_ss;
	unsigned short ds, res_ds;
	unsigned short fs, res_fs;
	unsigned short gs, res_gs;
	unsigned short ldt_s, res_lldtss;
	unsigned short res_iobitmap, iobitmap;
};

static struct x86_tss *x86_tss = (struct x86_tss *)SYSTEM_TSS_BASE;

void inline x86_setup_tss(struct task_struct *task)
{
	x86_tss->esp0 = task->stack_base;
}

void __init_text tss_init(void)
{
	if (!x86_tss)
		return;

	memset((char *)x86_tss, 0, sizeof(struct x86_tss));

	x86_tss->ss0 = KERNEL_SS;
	x86_tss->es = KERNEL_ES | 0x3;
	x86_tss->cs = KERNEL_CS | 0x3;
	x86_tss->ss = KERNEL_SS | 0x3;
	x86_tss->ds = KERNEL_DS | 0x3;
	x86_tss->fs = KERNEL_FS | 0x3;
	x86_tss->gs = KERNEL_GS | 0x3;

	setup_kernel_tss_des((unsigned long)x86_tss, sizeof(struct x86_tss));
	__asm("movw $0x68, %ax");
	__asm("ltr %ax");
}
