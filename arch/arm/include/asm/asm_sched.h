#ifndef _ASM_SCHED_H
#define _ASM_SCHED_H

#include <os/types.h>

struct task_struct;

typedef struct _pt_regs {
	unsigned long r0;
	unsigned long r1;
	unsigned long r2;
	unsigned long r3;
	unsigned long r4;
	unsigned long r5;
	unsigned long r6;
	unsigned long r7;
	unsigned long r8;
	unsigned long r9;
	unsigned long r10;
	unsigned long r11;
	unsigned long r12;
	unsigned long spsr;
	unsigned long lr;
	unsigned long sp;
	unsigned long cpsr;
	unsigned long pc;
} pt_regs;

void arch_switch_task_sw(void);
void arch_switch_task_hw(void);
void arch_init_pt_regs(pt_regs *regs, void *fn, void *arg);
int arch_set_up_task_stack(struct task_struct *task, pt_regs *regs);

int arch_set_task_return_value(pt_regs *reg,
		struct task_struct *task);
pt_regs *arch_get_pt_regs(void);

#endif
