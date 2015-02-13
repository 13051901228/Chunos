/*
 * mm/memory_map.h
 * 
 * Created by Le Min at 01/17/2015
 */

#ifndef __MEMORY_MAP_H_
#define __MEMORY_MAP_H_

/*
 * kernel use 1.5G size of the virtual address
 * normal_memory: 1G for kernel included res memory
 * 128M for DMA
 * 256M for IO
 * 64M for vmalloc
 * 48M for mmu translate buffer
 * 16M for arch which used by different arch
 */

#define KERNEL_VIRTUAL_BASE	(0xa0000000)
#define KERNEL_VIRTUAL_SIZE	(1024 * 1024 * 1024)

#define KERNEL_DMA_BASE		(KERNEL_VIRTUAL_BASE + KERNEL_VIRTUAL_SIZE)
#define KERNEL_DMA_SIZE		(128 * 1024 * 1024)

#define KERNEL_IO_BASE		(KERNEL_DMA_BASE + KERNEL_DMA_SIZE)
#define KERNEL_IO_SIZE		(256 * 1024 * 1024)

#define KERNEL_VMALLOC_BASE	(KERNEL_IO_BASE + KERNEL_IO_SIZE)
#define KERNEL_VMALLOC_SIZE	(64 * 1024 * 1024)

#define KERNEL_MMU_BUFFER_BASE	(KERNEL_VMALLOC_BASE + KERNEL_VMALLOC_SIZE)
#define KERNEL_MMU_BUFFER_SIZE	(32 * 1024 * 1024)

#define KERNEL_ELF_BUFFER_BASE	(KERNEL_MMU_BUFFER_BASE + KERNEL_MMU_BUFFER_SIZE)
#define KERNEL_ELF_BUFFER_SIZE	(16 * 1024 * 1024)

#define KERNEL_ARCH_BASE	(KERNEL_ELF_BUFFER_BASE + KERNEL_MMU_BUFFER_SIZE)
#define KERNEL_ARCH_SIZE	(16 * 1024 * 1024)

#endif
