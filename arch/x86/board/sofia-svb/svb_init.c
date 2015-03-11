#include <os/init.h>
#include <os/mm.h>
#include <os/soc.h>
#include <os/soc_id.h>

void svb_parse_memory_region(void)
{
	register_memory_region(0x0, 256 * 1024 * 1024, NORMAL_MEM);
	register_memory_region(0x10000000, 768 * 1024 *1024, NORMAL_MEM);
}

static struct soc_board sofia_svb = {
	.board_id = 0x01,
	.parse_memory_info = svb_parse_memory_region,
};
DEFINE_SOC_BOARD(sofia_svb);
