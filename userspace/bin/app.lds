ENTRY(__start)
SECTIONS
{
	. = 0x00401000;
	.text : 
	{
		*(.text) 
		*(.rodata)
	}


	. = ALIGN(4);

	.data : {*(.data)}

	_bss_start = .;
	.bss : {*(.bss)}
	_bss_end = .;
}
