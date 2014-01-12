	.data

.global __ramdisk_start
__ramdisk_start:	.word ramdisk_start

ramdisk_start:
	.incbin "out/ramdisk.img"
