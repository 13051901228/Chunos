/*
 * arch/x86/include/asm/asm_sched.h
 *
 * Create by Le Min at 12/16/2014
 *
 */

#ifndef __ASM_SCHED__H
#define __ASM_SCHED__H

typedef struct _pt_regs {
	unsigned long eax;
	unsigned long ebx;
	unsigned long ecx;
	unsigned long edx;
	unsigned long esi;
	unsigned long edi;
	unsigned long esp;
	unsigned long ebp;
	unsigned long es;
	unsigned long cs;
	unsigned long ss;
	unsigned long ds;
	unsigned long fs;
	unsigned long gx;
	unsigned long eip;
	unsigned long eflags;
} pt_regs;

#endif
