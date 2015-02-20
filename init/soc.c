/*
 * init/soc.c
 *
 * Created by Le Min(lemin9538@163.com)
 */

#include <os/types.h>
#include <os/init.h>
#include <os/mm.h>
#include <os/slab.h>
#include <os/bound.h>
#include <os/mirros.h>
#include <os/init.h>
#include <os/errno.h>
#include <os/string.h>
#include <os/printk.h>
#include <os/panic.h>

/* some static instance of the soc */
struct soc system_soc;
extern struct soc_desc soc_desc;

extern int os_tick_handler(void *arg);

static int __init_text find_system_soc(struct soc *soc)
{
	int nr, i;
	unsigned long start;
	int ret;
	extern unsigned long soc_platform_start, soc_platform_end;
	extern unsigned long soc_board_start, soc_board_end;
	struct soc_platform *platform_tmp;
	struct soc_board *board_tmp = (struct soc_board *)soc_board_start;

	/* find the member board and platform for system */
	start = soc_platform_start;
	nr = (soc_platform_start - soc_platform_end) / sizeof(struct soc_platform *);
	for (i = 0; i < nr; i ++) {
		platform_tmp = *((struct soc_platform **)start);
		if (platform_tmp[i].platform_id == soc_desc.platform_id) {
			memcpy(&soc->platform, platform_tmp, sizeof(struct soc_platform));
			break;
		}
		start += sizeof(unsigned long);
	}

	if (i == nr)
		ret = -EFAULT;

	start = soc_board_start;
	nr = (soc_board_end - soc_board_start) /sizeof(struct soc_board *);
	for (i = 0; i < nr; i ++) {
		board_tmp = *((struct soc_board **)start);
		if (board_tmp[i].board_id == soc_desc.board_id) {
			memcpy(&soc->board, board_tmp, sizeof(struct soc_board));
			break;
		}
		start += sizeof(unsigned long);
	}

	if (i == nr)
		return -EFAULT;

	return 0;
}

void __init_text soc_late_init(void)
{
	struct soc_platform *platform = get_platform();
	int i;

	for (i = 0; ; i++) {
		if (platform->init_action[i])
			(void) platform->init_action[i]();
		else
			break;
	}
}

void __init_text soc_clock_init(void)
{
	struct soc_platform *platform = get_platform();
	
	if (platform->system_clk_init)
		platform->system_clk_init();
}

int __init_text timer_tick_init(void)
{
	int ret = 0;
	struct soc_platform *platform = get_platform();

	if (platform->system_timer_init)
		platform->system_timer_init(HZ);
	else
		kernel_warning("System timer may not init\n");
	
	ret = register_irq(SYSTEM_TIMER_IRQ_NUMBER, os_tick_handler, NULL );
	if (ret)
		kernel_error("Can not register irq %d for system timer\n");
	
	return ret;
}

void register_memory_region(address_t start, size_t size, int attr)
{
	struct soc_memory_info *info = get_soc_memory_info();
	int nr = info->region_nr;
	struct memory_region *tmp = &info->region[nr];
	unsigned long new_start;

	if (nr < MEM_MAX_REGION) {
		new_start = baligin(start, SIZE_1M);
		size = size-(new_start - start);
		size = min_aligin(size, SIZE_1M);
		if (is_aligin(size, SIZE_1M)) {
			if(size >= PAGE_SIZE) {
				tmp->start = new_start;
				tmp->size = size;
				tmp->attr = attr;
				info->region_nr++;
			}
		}
	}
}

static void __init_text soc_get_mem_info(struct soc *soc)
{
	/* these value must set at lds or boot.S */
	extern unsigned long code_start, code_end;
	extern unsigned long bss_start, bss_end;
	extern unsigned long kernel_virtual_start, kernel_phy_start;
	extern unsigned long init_start, init_end;
	struct soc_memory_info *memory_info = get_soc_memory_info();

	kernel_debug("code start 0x%x code_end 0x%x\n", code_start, code_end);
	kernel_debug("bss_start 0x%x bss_end 0x%x\n", bss_start, bss_end);
	kernel_debug("kernel_virtual_start 0x%x kernel_phy_start 0x%x\n",
			kernel_virtual_start, kernel_phy_start);

	memory_info->code_start = code_start;
	memory_info->code_end = code_end;
	memory_info->init_start = init_start;
	memory_info->init_end = init_end;
	memory_info->bss_start = bss_start;
	memory_info->bss_end = bss_end;
	memory_info->kernel_virtual_start = kernel_virtual_start;
	memory_info->kernel_physical_start = kernel_phy_start;

	if (soc->platform.parse_memory_info)
		soc->platform.parse_memory_info();

	if (soc->board.parse_memory_info)
		soc->board.parse_memory_info();
}

void __init_text console_late_init(void)
{
	soc_console_late_init(115200);
}

void __init_text console_early_init(void)
{
	soc_console_early_init(115200);
}

int __init_text soc_early_init(void)
{
	int ret = 0;
	struct soc *soc = get_soc();

	ret = find_system_soc(soc);
	if (ret)
		panic("Unsupport soc and platform\n");

	soc_get_mem_info(soc);

	return 0;
}
