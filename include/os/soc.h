#ifndef _SOC_H_
#define _SOC_H_

#include <os/irq.h>
#include <os/init.h>
#include <os/mmu.h>

#define MEM_MAX_REGION	16

struct memory_region {
	u32 start;
	u32 size;
	int attr;
};

typedef int (*init_call_t)(void);

struct soc_memory_info {
	unsigned long code_start;		/* all kernel data-text start address */
	unsigned long code_end;		/* kernel data-text end address */
	unsigned long init_start;
	unsigned long init_end;
	unsigned long bss_start;
	unsigned long bss_end;
	unsigned long kernel_physical_start;
	unsigned long kernel_virtual_start;
	struct memory_region region[MEM_MAX_REGION];
	int region_nr;
};

/* ids for this system to match something */
struct soc_desc {
	u16 arch_id;
	u16 platform_id;
	u16 board_id;
} __attribute__((packed));

struct soc_platform {
	u16 platform_id;
	u32 (*system_clk_init)(void);
	int (*system_timer_init)(unsigned int hz);
	struct irq_chip *irq_chip;
	struct mmu *mmu;
	void (*parse_memory_info)(void);
	struct mmu_controller *mmu_controller;
	init_call_t *init_action;
};

struct soc_board {
	u16 board_id;
	void (*parse_memory_info)(void);	
};

/*
 * @soc_id	: to find the soc for system, such as s3c2440
 * @board_id	: to find the board for system ,such as tq2440
 * @name	: the name of the platform name.
 * @init_ops	: callbacks to init the soc
 * @board	: board resources information will attach to this member
 */
struct soc {
	u16 soc_id;
	u16 board_id;
	char name[16];
	struct soc_platform platform;
	struct soc_board board;
	struct soc_memory_info memory_info;
};

extern struct soc system_soc;
extern struct soc_memory_info soc_memory_info;

void register_memory_region(address_t start, size_t size, int attr);

static inline struct soc *get_soc(void)
{
	return (&system_soc);
}

static inline struct soc_platform *get_platform(void)
{
	return (&system_soc.platform);
}

static inline struct soc_board *get_board(void)
{
	return (&system_soc.board);
}

static inline struct soc_memory_info *get_soc_memory_info(void)
{
	return (&system_soc.memory_info);
}

static inline struct mmu *get_soc_mmu(void)
{
	return (system_soc.platform.mmu);
}

#define	DEFINE_SOC_PLATFORM(platform)	\
	static __attribute__((section(".__soc_platform"))) \
	struct soc_platform *__soc_platform_##platform  = (&platform)

#define DEFINE_SOC_BOARD(board)		\
	static __attribute__((section(".__soc_board")))	\
	struct soc_board *__soc_board_##board  = (&board)

void soc_clock_init(void);
int timer_tick_init(void);
void console_early_init(void);
int soc_early_init(void);

#endif
