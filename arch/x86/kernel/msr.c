/*
 * arch/x86/kernel/msr.c
 */


#include <os/kernel.h>

void msr_write(unsigned int msr, uint64_t value)
{
	__asm__ volatile (
		"movl %[msr], %%ecx\n\t"
		"movl %[data_lo], %%eax\n\t"
		"movl %[data_hi], %%edx\n\t"
		"wrmsr"
		:
		: [msr] "m" (msr),
		  [data_lo] "rm" ((uint32_t)(value & 0xFFFFFFFF)),
		  [data_hi] "rm" ((uint32_t)(value >> 32))
	        : "eax", "ecx", "edx");
}

uint64_t msr_read(unsigned int msr)
{
	uint64_t ret;

	__asm__ volatile (
	    "movl %[msr], %%ecx\n\t"
	    "rdmsr"
	    : "=A" (ret)
	    : [msr] "rm" (msr)
	    : "ecx");

	return ret;
}
