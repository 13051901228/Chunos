/*
 * arch/x86/kernel/idt.c
 *
 * Created By Le Min at 2015/01/03
 */

#include "include/idt.h"
#include "include/x86_config.h"
#include <os/kernel.h>
#include <os/init.h>
#include "include/gdt.h"

#define IDT_TYPE_INTERRUPT_GATE		(0)
#define IDT_TYPE_TRAP_GATE		(1)
#define IDT_TYPE_TASK_GATE		(2)

#define MAX_IDT_ENTRY			(1024)

struct idt_entry *idt_base = (struct idt_entry *)SYSTEM_IDT_BASE;

static inline struct idt_entry *get_idt_entry(int index)
{
	if (index > MAX_IDT_ENTRY)
		return NULL;

	return (idt_base + index);
}

static inline void
setup_interrupt_gate(struct idt_entry *entry, unsigned long func)
{
	struct interrupt_gate *gate = &entry->interrupt_gate;

	gate->offset_l = func & 0xffff;
	gate->seg_sel = KERNEL_CS;
	gate->attr = 0xe;
	gate->dpl = 0x0;
	gate->p = 1;
	gate->offset_h = (func & 0xffff000) >> 16;
}

static inline void
setup_trap_gate(struct idt_entry *entry, unsigned long func)
{
	struct trap_gate *gate = &entry->trap_gate;

	gate->offset_l = func & 0xffff;
	gate->seg_sel = KERNEL_CS;
	gate->attr = 0xf;
	gate->dpl = 0x0;
	gate->p = 1;
	gate->offset_h = (func & 0xffff000) >> 16;
}

static inline void
setup_task_gate(struct idt_entry *entry, unsigned long func)
{
	/* TBC */
}

static int setup_idt_entry(int index,
		unsigned long func, int type)
{
	struct idt_entry *entry;

	entry = get_idt_entry(index);
	if (!entry)
		return -EINVAL;

	memset((char *)entry, 0, sizeof(struct idt_entry));

	switch (type) {
	case IDT_TYPE_INTERRUPT_GATE:
		setup_interrupt_gate(entry, func);
		break;

	case IDT_TYPE_TRAP_GATE:
		setup_trap_gate(entry, func);
		break;

	case IDT_TYPE_TASK_GATE:
		setup_task_gate(entry, func);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

int install_interrupt_gate(int index, unsigned long func)
{
	return setup_idt_entry(index, func, IDT_TYPE_INTERRUPT_GATE);
}

int install_trap_gate(int index, unsigned long func)
{
	return setup_idt_entry(index, func, IDT_TYPE_TRAP_GATE);
}

int install_task_gate(int index, unsigned long func)
{
	return setup_idt_entry(index, func, IDT_TYPE_TASK_GATE);
}

void x86_idt_init(void)
{
	memset((char *)idt_base, 0, SYSTEM_IDT_SIZE);
	trap_init();
}
