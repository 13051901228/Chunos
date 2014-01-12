#ifndef _ASM_MARCO_H
#define _ASM_MARCO_H

.macro set_mode, mode
	msr cpsr_c, #\mode
.endm

#endif
