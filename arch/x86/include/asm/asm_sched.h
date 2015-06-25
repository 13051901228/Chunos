/*
 * arch/x86/include/asm/asm_sched.h
 *
 * Create by Le Min at 12/16/2014
 *
 */

#ifndef __ASM_SCHED__H
#define __ASM_SCHED__H

typedef struct _pt_regs {
	unsigned short gs, res_gs;
	unsigned short fs, res_fs;
	unsigned short ds, res_ds;
	unsigned short es, res_es;
	unsigned short ss, res_ss;
	unsigned long esp_cr3;
	unsigned long edi;
	unsigned long esi;
	unsigned long ebp;
	unsigned long esp_cr0;
	unsigned long ebx;
	unsigned long edx;
	unsigned long ecx;
	unsigned long eax;
	unsigned long error_code;
	unsigned long eip;
	unsigned short cs, res_cs;
	unsigned long eflags;
} pt_regs;

#endif
