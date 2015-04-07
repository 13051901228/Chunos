/*
 * arch/x86/include/asm/asm_sched.h
 *
 * Create by Le Min at 12/16/2014
 *
 */

#ifndef __ASM_SCHED__H
#define __ASM_SCHED__H

typedef struct _pt_regs {
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
} pt_regs;

#endif
