/*
 * arch/x86/kernel/include/idt.h
 *
 * Created by Le Min at 2015/01/03
 */

#ifndef _IDT_H_
#define _IDT_H_

#include <os/types.h>

struct task_gate {
	u64 res_1 : 16;
	u64 tss_seg_sel : 16;
	u64 res_2 : 8;
	u64 attr : 5;
	u64 dpl : 2;
	u64 p : 1;
	u64 res_3 : 16;
} __attribute__((packed));

struct interrupt_gate {
	u64 offset_l : 16;
	u64 seg_sel : 16;
	u64 res_1 : 5;
	u64 it : 3;
	u64 attr : 5;
	u64 dpl : 2;
	u64 p : 1;
	u64 offset_h : 16;
} __attribute__((packed));

struct trap_gate {
	u64 offset_l : 16;
	u64 seg_sel : 16;
	u64 res_1 : 5;
	u64 it : 3;
	u64 attr : 5;
	u64 dpl : 2;
	u64 p : 1;
	u64 offset_h : 16;
} __attribute__((packed));

struct idt_entry {
	union {
		struct task_gate task_gate;
		struct interrupt_gate interrupt_gate;
		struct trap_gate trap_gate;
		u64 data;
	};
};

int install_interrupt_gate(int index, unsigned long func);

int install_trap_gate(int index, unsigned long func);

int install_task_gate(int index, unsigned long func);

int install_syscall_gate(int index, unsigned long func);

#endif
