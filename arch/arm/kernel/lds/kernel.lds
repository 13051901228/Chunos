ENTRY(_start)
SECTIONS
{
	. = 0x8000a000;

	__code_start = .;
	.text : 
	{
		*(.text) 
		*(.rodata)
	}

	. = ALIGN(4);

	.data : {*(.data)}

	. = ALIGN(4);

	__bss_start = .;
	.bss : {*(.bss)}
	__bss_end = .;

	. = ALIGN(4);

	__init_start = .;

	__soc_platform_start = .;
	.__soc_platform : {
		*(.__soc_platform)
	}
	__soc_platform_end = .;

	__soc_board_start = .;
	.__soc_board : {
		*(.__soc_board)
	}
	__soc_board_end = .;

	. = ALIGN(4);

	__init_text_start = .;
	.__init_text : {
		*(__init_text)
	}
	__init_text_end = .;

	. = ALIGN(4);

	__init_data_start = .;
	.__init_data : {
		*(__init_data)
	}
	__init_data_end = .;

	. = ALIGN(4);

	__syscall_table_start = .;
	.syscall_table : {
		*(.__syscall)
	}
	__syscall_table_end = .;

	. = ALIGN(4);
	
	__init_end = .;
	__code_end = .;
}
