#ifndef _S3C2440_H
#define _S3C2440_H

/******************************************************
 * configure the bus parment for the board
 * ***************************************************/

#define DW8			(0x0)
#define DW16			(0x1)
#define DW32			(0x2)
#define WAIT			(0x1<<2)
#define UBLB			(0x1<<3)


#define B1_BWSCON	 	(DW16)	/* AMD flash(AM29LV160DB), 16-bit,  for nCS1*/
#define B2_BWSCON	 	(DW16)	
#define B3_BWSCON	 	(DW16)	
#define B4_BWSCON	 	(DW32)	
#define B5_BWSCON	 	(DW16)	
#define B6_BWSCON	 	(DW32)	
#define B7_BWSCON	 	(DW32)	

/*BANK0CON*/

#define B0_Tacs			0x3	
#define B0_Tcos			0x3	
#define B0_Tacc			0x7	
#define B0_Tcoh			0x3	
#define B0_Tah			0x3	
#define B0_Tacp			0x1
#define B0_PMC			0x0	

/*BANK1CON*/
#define B1_Tacs			1	
#define B1_Tcos			1	
#define B1_Tacc			6	
#define B1_Tcoh			1	
#define B1_Tah			1	
#define B1_Tacp			0x0
#define B1_PMC			0x0	

/*Bank 2 parameter*/
#define B2_Tacs			1	
#define B2_Tcos			1	
#define B2_Tacc			6	
#define B2_Tcoh			1	
#define B2_Tah			1	
#define B2_Tacp			0x0
#define B2_PMC			0x0	

#define B3_Tacs			0x1	
#define B3_Tcos			0x1	
#define B3_Tacc			0x6	
#define B3_Tcoh			0x1	
#define B3_Tah			0x1	
#define B3_Tacp			0x0
#define B3_PMC			0x0

#define B4_Tacs			0x1	
#define B4_Tcos			0x1	
#define B4_Tacc			0x6	
#define B4_Tcoh			0x1	
#define B4_Tah			0x1	
#define B4_Tacp			0x0
#define B4_PMC			0x0	

#define B5_Tacs			0x1	
#define B5_Tcos			0x1	
#define B5_Tacc			0x6	
#define B5_Tcoh			0x1	
#define B5_Tah			0x1	
#define B5_Tacp			0x0
#define B5_PMC			0x0	

#define TRUE			1

#ifdef	 TRUE				
	#define B6_MT			0x3	
	#define B6_Trcd			0x1	
	#define B6_SCAN			0x1	

	#define B7_MT			0x3	
	#define B7_Trcd			0x1	
	#define B7_SCAN			0x1	

	#define REFEN			0x1	
	#define TREFMD			0x0	
	#define Trp			0x1	
	#define Tsrc			0x1	
	#define Tchr			0x2	
	#define REFCNT			1268	
#else
	#define B6_MT			0x3	
	#define B6_Trcd			0x2	
	#define B6_SCAN			0x1	

	#define B7_MT			0x3	
	#define B7_Trcd			0x2	
	#define B7_SCAN			0x1	

	#define REFEN			0x1	
	#define TREFMD			0x0	
	#define Trp			0x2	
	#define Tsrc			0x2	

	#define Tchr			0x2
	#define REFCNT			1012	
#endif

/***************************************
 * configure the clk for s3c2440 board
 * **************************************/
#define PLL_ON_START
#define ENTRY_BUS_WIDTH		 16
#define BUSWIDTH		 32
#define UCLK			48000000
#define CPU_SEL	SETA		32440001		
#define XTAL_SEL		12000000	
#define FCLK			400000000	
#define FIN			12000000

#define CLKDIV_VAL		5	
#define M_MDIV			92		
#define M_PDIV			1
#define M_SDIV			1		
#define U_MDIV			56		
#define U_PDIV			2
#define U_SDIV			2		

#endif
