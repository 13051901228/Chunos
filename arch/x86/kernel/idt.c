/*
 * arch/x86/kernel/idt.c
 *
 * Created By Le Min at 2015/01/03
 */

#include "include/idt.h"
#include "include/x86_config.h"
#include <os/kernel.h>
#include <os/init.h>

struct idt_entry *base = (struct idt_entry *)SYSTEM_IDT_BASE;

static int install_interrupt_gate(int index, unsigned long func)
{
	return 0;
}

static int install_trap_gate(int index, unsigned long func)
{
	return 0;
}

static int install_task_gate(int index, unsigned long func)
{
	return 0;
}

void x86_idt_init(void)
{
	
}
