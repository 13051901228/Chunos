#ifndef _SOC_ID_H
#define _SOC_ID_H

typedef enum _arch_id {
	ARCH_ARM,
	ARCH_X86,
	ARCH_UNKNOWN
}arch_id;

typedef enum _platform_id {
	PLATFORM_S3C2440,
	PLATFORM_S3C6410,
	PLATFORM_UNKNOWN,
}platform_id;

typedef enum _board_id {
	BOARD_TQ2440,
	BOARD_MINI2440,
	BOARD_UNKNOWN
}board_id;

#endif
