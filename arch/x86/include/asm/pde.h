/*
 * include/asm/pde.h
 *
 * Created by Le Min at 2015/03/06
 *
 */

#ifndef _ARCH_PDE_H
#define _ARCH_PDE_H

#define ARCH_PDE_MAP_SIZE		(4 * 1024 * 1024)
#define ARCH_PDE_ENTRY_SIZE		(sizeof(unsigned long))
#define ARCH_PDE_ENTRY_SHIFT		(22)
#define ARCH_PDE_TABLE_SIZE		(4 * 1024)
#define ARCH_PDE_TABLE_ALIGN_SIZE	(4 * 1024)

#define ARCH_PTES_PER_PDE		(4 * 1024 / (sizeof(unsigned long)))
#define ARCH_PTE_ENTRY_SIZE		(sizeof(unsigned long))
#define ARCH_PTE_TABLE_SIZE		(4 * 1024)

#define ARCH_MAP_SIZE_PER_PAGE		(4 * 1024 * 1024)

#endif
