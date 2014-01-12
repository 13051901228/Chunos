#ifndef _INIT_H
#define _INIT_H

#define INIT_PATH_SIZE		128

#define CMDLINE_TAG		"xxoo"
#define ARCH_NAME_SIZE		8
#define BOARD_NAME_SIZE		16

#define MEM_MAX_REGION		16

#include <os/mm.h>

struct cmdline {
	unsigned long tag;
	unsigned long head;
	char arg[256];
};

#define __init_data __attribute__((section(".init_data")))
#define __init_text __attribute__((section(".init_text")))

#endif
