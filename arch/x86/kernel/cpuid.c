/*
 * arch/x86/kernel/cpuid.c
 */

#include <os/kernel.h>

void cpuid(unsigned int op, unsigned int *eax, unsigned int *ebx,
		unsigned int *ecx, unsigned int *edx)
{
	*eax = op;
	asm volatile("cpuid"
		     : "=a"(*eax),
		       "=b"(*ebx),
		       "=c"(*ecx),
                       "=d"(*edx)
                     : "0"(*eax), "2"(*ecx));
}
