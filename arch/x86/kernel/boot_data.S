	.data
	.align 8

.global	code_start
code_start:			.long __code_start

.global code_end
code_end:			.long __code_end

.global bss_start
bss_start:			.long __bss_start

.global bss_end
bss_end:			.long __bss_end

.global init_start
init_start:			.long __init_start

.global init_end
init_end:			.long __init_end

.global	soc_platform_start
soc_platform_start:		.long __soc_platform_start

.global soc_platform_end
soc_platform_end:		.long __soc_platform_end

.global soc_board_start
soc_board_start:		.long __soc_board_start

.global soc_board_end
soc_board_end:			.long __soc_board_end

.global	init_text_start
init_text_start:		.long __init_text_start

.global init_text_end
init_text_end:			.long __init_text_end

.global init_data_start
init_data_start:		.long __init_data_start

.global init_data_end
init_data_end:			.long __init_data_end

.global syscall_table_start
syscall_table_start:		.long __syscall_table_start

.global syscall_table_end
syscall_table_end:		.long __syscall_table_end

.global init_call_start
init_call_start:		.long __init_func_start

.global init_call_end
init_call_end:			.long __init_func_end
