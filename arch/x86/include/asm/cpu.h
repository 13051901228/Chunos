/*
 * include/asm/cpu.h
 *
 * Created by LeMin at 2014/01/26
 *
 */

#ifndef _CPU_H_
#define _CPU_H_

#include <os/bound.h>

#define ARCH_PDE_ALIGN_SIZE		(4 * 1024 * 1024)
#define ARCH_PDE_ALIGN(addr)		align(addr, ARCH_PDE_ALIGN_SIZE)
#define ARCH_PDE_MIN_ALIGN(addr)	min_align(addr, ARCH_PDE_ALIGN_SIZE)

void arch_flush_mmu_tlb(void);
void arch_flush_cache();

#endif

