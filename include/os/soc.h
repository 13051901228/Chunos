#ifndef _SOC_H_
#define _SOC_H_

#include <os/irq.h>
#include <os/init.h>

#define MEM_MAX_REGION	16

typedef int (*init_call)(void);

struct memory_region {
	u32 start;
	u32 size;
	int attr;
};

struct soc_memory_info {
	u32 code_start;		/* all kernel data-text start address */
	u32 code_end;		/* kernel data-text end address */
	u32 init_start;
	u32 init_end;
	u32 bss_start;
	u32 bss_end;
	u32 kernel_physical_start;
	u32 kernel_virtual_start;
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
	int (*system_timer_init)(u32 hz);
	struct irq_chip *irq_chip;
	init_call *init_action;
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
