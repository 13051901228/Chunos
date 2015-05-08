/*
 * arch/x86/kernel/include/gdt.c
 *
 * Created by Le Min at 2015
 */

#include "include/gdt.h"
#include "include/x86_config.h"
#include <os/kernel.h>
#include <os/init.h>

struct gdt_des {
	u64 s_limit_l : 16;
	u64 base_l : 16;
	u64 base_h1 : 8;
	u64 type : 4;
	u64 s : 1;
	u64 dpl : 2;
	u64 p : 1;
	u64 s_limit_h : 4;
	u64 avl : 1;
	u64 l : 1;
	u64 db : 1;
	u64 g : 1;
	u64 base_h2 : 8;
} __attribute__((packed));

struct gdt_entry {
	union {
		struct gdt_des gdt_des;
		u64 data;
	};
};

struct gdt_entry *gdt_base = (struct gdt_entry *)SYSTEM_GDT_BASE;

void setup_tss_des(int index, unsigned long base, int page)
{
	struct gdt_entry *entry;
	struct gdt_des *des;

	entry = gdt_base + index;
	des = &entry->gdt_des;

	des->s_limit_l = page & 0xffff;
	des->base_l = base & 0xffff;
	des->base_h1 = (base >> 16) & 0xff;
	des->type = 9;
	des->s = 0;
	des->dpl = 0;
	des->p = 1;
	des->s_limit_h = (page >> 16) & 0xf;
	des->avl = 1;
	des->l = 0;
	des->db = 1;
	des->g = 1;
	des->base_h2 = (base >> 24) & 0xff;
}

void setup_kernel_tss_des(unsigned long base, int size)
{
	if (base && size)
		setup_tss_des(KERNEL_TSS >> 3, base, size);
}
