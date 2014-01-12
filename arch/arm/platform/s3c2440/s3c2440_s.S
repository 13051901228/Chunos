#include "include/s3c2440.h"

#define WTCON		0X53000000
#define INTMSK		0x4a000008
#define INTSUBMSK	0x4a00000c
#define LOCKTIME	0x4c000000
#define UPLLCON		0x4c000008
#define MPLLCON		0x4c000004
#define BWSCON		0x48000000
#define INTOFFSET	0x4a000014	
#define SRCPND		0x4a000000
#define INTPND		0x4a000010
#define CLKDIVN		0x4c000014

	.text

.global low_level_init
low_level_init:
	ldr r0,=WTCON		@ disable watchdog
	ldr r1,=0
	str r1,[r0]

	ldr r0,=INTMSK		@ disable interrupt
	ldr r1,=0xffffffff
	str r1,[r0]
	ldr r0,=INTSUBMSK
	ldr r1,=0x7FFF
	str r1,[r0]

	ldr r0,=LOCKTIME	@ set pll
	ldr r1,=0xffffffff
	str r1,[r0]
	ldr r0,=CLKDIVN
	ldr r1,=CLKDIV_VAL
	str r1,[r0]

#if	CLDDIV_VAL >1		
	mrc p15,0,r0,c1,c0,0	@ chage the bus mode to asynchronous
	orr r0,r0,#0xc000000
	mcr p15,0,r0,c1,c0,0
#else
	mrc p15,0,r0,c1,c0,0
	bic r0,r0,#0xc000000
	mcr p15,0,r0,c1,c0,0
#endif
	ldr r0,=UPLLCON		@ configure upll,must set upll at first
	ldr r1,upll_data
	str r1,[r0]
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	
	ldr r0,=MPLLCON
	ldr r1,mpll_data
	str r1,[r0]

	mov pc, lr

upll_data: .word ((U_MDIV<<12)+(U_PDIV<<4)+U_SDIV)
mpll_data: .word ((M_MDIV<<12)+(M_PDIV<<4)+M_SDIV)

.global init_mm_bank
init_mm_bank:
	adrl r0,sram_data
	ldr r1,=BWSCON
	add r2,r0,#52

1:
	ldr r3,[r0],#4		
	str r3,[r1],#4
	cmp r2,r0		@loader the data from address [r0],after this add r0=r0+4
	bne 1b

	mov pc,lr

sram_data:
	.word (0+(B1_BWSCON<<4)+(B2_BWSCON<<8)+(B3_BWSCON<<12)+(B4_BWSCON<<16)+(B5_BWSCON<<20)+(B6_BWSCON<<24)+(B7_BWSCON<<28))
	.word ((B0_Tacs<<13)+(B0_Tcos<<11)+(B0_Tacc<<8)+(B0_Tcoh<<6)+(B0_Tah<<4)+(B0_Tacp<<2)+(B0_PMC))   @GCS0
	.word ((B1_Tacs<<13)+(B1_Tcos<<11)+(B1_Tacc<<8)+(B1_Tcoh<<6)+(B1_Tah<<4)+(B1_Tacp<<2)+(B1_PMC))   @GCS1
	.word ((B2_Tacs<<13)+(B2_Tcos<<11)+(B2_Tacc<<8)+(B2_Tcoh<<6)+(B2_Tah<<4)+(B2_Tacp<<2)+(B2_PMC))   @GCS2
	.word ((B3_Tacs<<13)+(B3_Tcos<<11)+(B3_Tacc<<8)+(B3_Tcoh<<6)+(B3_Tah<<4)+(B3_Tacp<<2)+(B3_PMC))   @GCS3
	.word ((B4_Tacs<<13)+(B4_Tcos<<11)+(B4_Tacc<<8)+(B4_Tcoh<<6)+(B4_Tah<<4)+(B4_Tacp<<2)+(B4_PMC))   @GCS4
	.word ((B5_Tacs<<13)+(B5_Tcos<<11)+(B5_Tacc<<8)+(B5_Tcoh<<6)+(B5_Tah<<4)+(B5_Tacp<<2)+(B5_PMC))   @GCS5
	.word ((B6_MT<<15)+(B6_Trcd<<2)+(B6_SCAN))    @GCS6
	.word ((B7_MT<<15)+(B7_Trcd<<2)+(B7_SCAN))    @GCS7
	.word ((REFEN<<23)+(TREFMD<<22)+(Trp<<20)+(Tsrc<<18)+(Tchr<<16)+REFCNT)
	.word 0x32	    @SCLK power saving mode, BANKSIZE 128M/128M
	.word 0x30	    @MRSR6 CL=3clk
	.word 0x30	    @MRSR7 CL=3clk
