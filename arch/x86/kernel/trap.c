/*
 * arch/x86/kernel/idt.c
 *
 * Created By Le Min at 2015/01/03
 */
#include "include/idt.h"
#include <os/kernel.h>
#include <os/init.h>
#include "include/gdt.h"
#include "include/trap.h"

extern int x86_int(void);

static int x86_trap_handler_de(void)
{
	return 0;
}

static int x86_trap_handler_db(void)
{
	return 0;
}

static int x86_trap_handler_nmi(void)
{
	return 0;
}

static int x86_trap_handler_bp(void)
{
	return 0;
}

static int x86_trap_handler_of(void)
{
	return 0;
}

static int x86_trap_handler_br(void)
{
	return 0;
}

static int x86_trap_handler_ud(void)
{
	return 0;
}

static int x86_trap_handler_nm(void)
{
	return 0;
}

static int x86_trap_handler_df(void)
{
	return 0;
}

static int x86_trap_handler_old_mf(void)
{
	return 0;
}

static int x86_trap_handler_ts(void)
{
	return 0;
}

static int x86_trap_handler_np(void)
{
	return 0;
}

static int x86_trap_handler_ss(void)
{
	return 0;
}

static int x86_trap_handler_gp(void)
{
	return 0;
}

static int x86_trap_handler_pf(void)
{
	return 0;
}

static int x86_trap_handler_spurious(void)
{
	return 0;
}

static int x86_trap_handler_mf(void)
{
	return 0;
}

static int x86_trap_handler_ac(void)
{
	return 0;
}

static int x86_trap_handler_nc(void)
{
	return 0;
}

static int x86_trap_handler_xf(void)
{
	return 0;
}

static int x86_trap_handler_iret(void)
{
	return 0;
}

void __init_text trap_init(void)
{
	int i;

	install_trap_gate(X86_TRAP_DE, (unsigned long)x86_trap_de);
	install_trap_gate(X86_TRAP_DB, (unsigned long)x86_db);
	install_interrupt_gate(X86_TRAP_NMI,
		(unsigned long)x86_nmi);

	install_trap_gate(X86_TRAP_BP, (unsigned long)x86_bp);
	install_trap_gate(X86_TRAP_OF, (unsigned long)x86_of);
	install_trap_gate(X86_TRAP_BR, (unsigned long)x86_br);
	install_trap_gate(X86_TRAP_UD, (unsigned long)x86_ud);
	install_trap_gate(X86_TRAP_NM, (unsigned long)x86_nm);
	install_trap_gate(X86_TRAP_DF, (unsigned long)x86_df);
	install_trap_gate(X86_TRAP_OLD_MF,
		(unsigned long)x86_old_mf);

	install_trap_gate(X86_TRAP_TS, (unsigned long)x86_ts);
	install_trap_gate(X86_TRAP_NP, (unsigned long)x86_np);
	install_trap_gate(X86_TRAP_SS, (unsigned long)x86_ss);
	install_trap_gate(X86_TRAP_GP, (unsigned long)x86_gp);
	install_trap_gate(X86_TRAP_PF, (unsigned long)x86_pf);
	install_trap_gate(X86_TRAP_SPURIOUS,
		(unsigned long)x86_spurious);

	install_trap_gate(X86_TRAP_MF, (unsigned long)x86_mf);
	install_trap_gate(X86_TRAP_AC, (unsigned long)x86_ac);
	install_trap_gate(X86_TRAP_MC, (unsigned long)x86_nc);
	install_trap_gate(X86_TRAP_XF, (unsigned long)x86_xf);
	install_trap_gate(X86_TRAP_IRET, (unsigned long)x86_iret);
}
