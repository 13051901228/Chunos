	.data

	.align 8

.global __ramdisk_start
__ramdisk_start:	.long ramdisk_start

ramdisk_start:
	.incbin "out/ramdisk.img"
