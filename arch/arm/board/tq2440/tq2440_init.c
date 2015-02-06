#include <os/init.h>
#include <os/mm.h>
#include <os/soc.h>
#include <os/soc_id.h>

void tq2440_parse_memory_region(void)
{
	register_memory_region(0x30000000, 64 * 1024 * 1024, NORMAL_MEM);
}

static struct soc_board tq2440 = {
	.board_id = BOARD_TQ2440,
	.parse_memory_info = tq2440_parse_memory_region,
};
DEFINE_SOC_BOARD(tq2440);
