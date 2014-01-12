#ifndef _ASM_MMU_H
#define _ASM_MMU_H

#include <os/types.h>

unsigned long arch_get_tlb_base(void);
u32 arch_build_tlb_des(unsigned long pa, u32 attr);
u32 arch_build_page_table_des(unsigned long pa, u32 attr);
u32 arch_get_tlb_attr(u32 flag);
u32 arch_get_page_table_attr(u32 flag);
void arch_flush_mmu_tlb(void);
void arch_flush_cache();

#endif

