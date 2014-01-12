#ifndef _ARM_H
#define _ARM_H

#define MODE_MASK		0X1F
#define USER_MODE		0X10
#define FIQ_MODE		0x11
#define IRQ_MODE		0X12
#define SVC_MODE		0x13
#define ABORT_MODE		0X17
#define UNDEF_MODE		0x1b
#define SYSTEM_MODE		0x1f
#define NO_INT			0xc0

/*
 * define some attribute of arm920t MMU
 */
#define C2_BIT4			(1<<4)
//define some attrib of the mmu (AP bit)\\.

#define AP_SRW_UN		0X01
#define AP_SRW_UR		0X02
#define AP_SRW_URW		0X03

//define the type of the cache (CB bit of C2)
#define WB			0X03			//WRITE BACK CACHE
#define WT			0X02			//WRITE THOURGH CACHE
#define NCNB			0X0
#define NCB			0x1

//define the authorization of the 16 domains.
#define NO_AUTH			0X00
#define CLIENT			0X01
#define SUPERVISOR		0X03
#define DEFAULT			CLIENT
#define D_DEFAULT		0X0
#define D0			CLIENT
#define D1			CLIENT
#define D2			CLIENT
#define D3			CLIENT
#define D4			CLIENT
#define D5			CLIENT
#define D6			CLIENT
#define D7			CLIENT
#define D8			CLIENT
#define D9			CLIENT
#define D10			CLIENT
#define D11			CLIENT
#define D12			CLIENT
#define D13			CLIENT
#define D14			CLIENT
#define D15			CLIENT

#define DOMAIN0			(0<<5)

//define the type of the MMU table ([1:0] bit of C2)
#define MST			0X02		//SECTION 
#define MST_WB_AP_SRW_UN	((MST)|(WB<<2)|(AP_SRW_UN<<10)|C2_BIT4)
#define MST_WB_AP_SRW_UR	((MST)|(WB<<2)|(AP_SRW_UR<<10)|C2_BIT4)
#define MST_WB_AP_SRW_URW	((MST)|(WB<<2)|(AP_SRW_URW<<10)|C2_BIT4)
#define MST_WT_AP_SRW_UN	((MST)|(WT<<2)|(AP_SRW_UN<<10)|C2_BIT4)
#define MST_WT_AP_SRW_UR	((MST)|(WT<<2)|(AP_SRW_UR<<10)|C2_BIT4)
#define MST_WT_AP_SRW_URW	((MST)|(WT<<2)|(AP_SRW_URW<<10)|C2_BIT4)
#define MST_NCNB_AP_SRW_UN	((MST)|(NCNB<<2)|(AP_SRW_UN<<10)|C2_BIT4)
#define MST_NCNB_AP_SRW_UR	((MST)|(NCNB<<2)|(AP_SRW_UR<<10)|C2_BIT4)
#define MST_NCNB_AP_SRW_URW	((MST)|(NCNB<<2)|(AP_SRW_URW<<10)|C2_BIT4)
#define MST_NCB_AP_SRW_UN	((MST)|(NCB<<2)|(AP_SRW_UN<<10)|C2_BIT4)
#define MST_NCB_AP_SRW_UR	((MST)|(NCB<<2)|(AP_SRW_UR<<10)|C2_BIT4)
#define MST_NCB_AP_SRW_URW	((MST)|(NCB<<2)|(AP_SRW_URW<<10)|C2_BIT4)

#define CURDE_PAGE_TABLE	(0x01 | DOMAIN0)

/*
 * define some bit offset of second level page table descriptor
 */
#define PGT_DES_4K		(0X02)
#define PGT_DES			(0)
#define PGT_CB			(2)
#define PGT_AP0			(4)
#define PGT_AP1			(6)
#define PGT_AP2			(8)
#define PGT_AP3			(10)

#define PGT_DES_DEFAULT	\
	(PGT_DES_4K|(AP_SRW_URW << PGT_AP0)|(AP_SRW_URW << PGT_AP1)|(AP_SRW_URW << PGT_AP2)|(AP_SRW_URW << PGT_AP3)|(WB<<PGT_CB))

#endif
