ARCH 		:= arm
CROSS_COMPILE 	:= arm-linux-
CC 		:= $(CROSS_COMPILE)gcc
LD 		:= $(CROSS_COMPILE)ld
OBJCOPY		:= $(CROSS_COMPILE)objcopy
OBJDUMP 	:= $(CROSS_COMPILE)objdump
AR	 	:= $(CROSS_COMPILE)ar
STRIP	 	:= $(CROSS_COMPILE)strip
INCLUDE_DIR 	:= $(PWD)/include
CCFLAG 		:= -nostdlib -fno-builtin -I$(PWD)/include
OUT 		:= ../../out
RAMDISK 	:= ../../ramdisk

LDS 		:= ./lds/app.lds

LDFLAG 		:= -T$(LDS) -nostdlib
LDPATH 		:= -ldietlibc -lm -lgcc -L./lib

object 		= init test

all: $(RAMDISK) $(object)

$(RAMDISK):
	@mkdir -p $(RAMDISK)
	@mkdir -p $(OUT)

$(object):%:%.c
	@ $(CC) $(CCFLAG) -o $@ ./lib/_start.o ./lib/exit.o ./lib/lib.o $< $(LDFLAG) $(LDPATH)
	@ cp $@ $(RAMDISK)
	@ $(STRIP) -s $(RAMDISK)/$@
	@ $(OBJDUMP) -D $@ > $(OUT)/$@.s

.PHONY: clean
clean:
	@ rm -f $(object)
