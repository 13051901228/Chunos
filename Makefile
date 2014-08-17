ARCH 		:= arm
CROSS_COMPILE 	:= $(ARCH)-linux-
CC 		:= $(CROSS_COMPILE)gcc
LD 		:= $(CROSS_COMPILE)ld
OBJ_COPY	:= $(CROSS_COMPILE)objcopy
OBJ_DUMP 	:= $(CROSS_COMPILE)objdump

PLATFORM	:= s3c2440
BOARD		:= tq2440

INCLUDE_DIR 	:= include/os/*.h include/asm/*.h include/config/*.h

CCFLAG 		:=-Wall -nostdlib -fno-builtin -g -I$(PWD)/include
LDS 		:= arch/$(ARCH)/kernel/lds/kernel.lds
LDFLAG 		:= -T$(LDS)
LDPATH 		:= -L/opt/FriendlyARM/toolschain/4.4.3/lib/gcc/arm-none-linux-gnueabi/4.4.3 -lgcc

OUT 		:= out
OUT_KERNEL 	= $(OUT)/kernel
OUT_ARCH 	= $(OUT)/$(ARCH)
OUT_PLATFORM 	= $(OUT)/$(PLATFORM)
OUT_BOARD 	= $(OUT)/$(BOARD)
OUT_FS		= $(OUT)/fs
OUT_DRIVER	= $(OUT)/drivers
OUT_RAMDISK	= $(OUT)/ramdisk
OUT_TOOLS	= $(OUT)/tools

boot_elf 	:= $(OUT)/boot.elf
boot_bin 	:= $(OUT)/boot.bin
boot_dump 	:= $(OUT)/boot.s

SRC_ARCH_C	:= $(wildcard arch/$(ARCH)/kernel/*.c)
SRC_ARCH_S	:= $(wildcard arch/$(ARCH)/kernel/*.S)
SRC_KERNEL	:= $(wildcard kernel/*.c init/*.c mm/*.c)
SRC_PLATFORM_C	:= $(wildcard arch/$(ARCH)/platform/$(PLATFORM)/*.c)
SRC_PLATFORM_S	:= $(wildcard arch/$(ARCH)/platform/$(PLATFORM)/*.S)
SRC_BOARD_C	:= $(wildcard arch/$(ARCH)/board/$(BOARD)/*.c)
SRC_BOARD_S	:= $(wildcard arch/$(ARCH)/board/$(BOARD)/*.S)
SRC_FS		:= $(wildcard fs/*.c)
SRC_DRIVER	:= $(wildcard drivers/*.c)

VPATH		:= kernel:mm:init:fs:drivers:arch/$(ARCH)/platform/$(PLATFORM):arch/$(ARCH)/board/$(BOARD):arch/$(ARCH)/kernel

.SUFFIXES:
.SUFFIXES: .S .c

_OBJ_ARCH	+= $(addprefix $(OUT_ARCH)/, $(patsubst %.c,%.o, $(notdir $(SRC_ARCH_C))))
_OBJ_ARCH	+= $(addprefix $(OUT_ARCH)/, $(patsubst %.S,%.o, $(notdir $(SRC_ARCH_S))))
OBJ_ARCH	= $(subst out/$(ARCH)/boot.o,,$(_OBJ_ARCH))
OBJ_KERNEL	+= $(addprefix $(OUT_KERNEL)/, $(patsubst %.c,%.o, $(notdir $(SRC_KERNEL))))
OBJ_PLATFORM	+= $(addprefix $(OUT_PLATFORM)/, $(patsubst %.c,%.o, $(notdir $(SRC_PLATFORM_C))))
OBJ_PLATFORM	+= $(addprefix $(OUT_PLATFORM)/, $(patsubst %.S,%.o, $(notdir $(SRC_PLATFORM_S))))
OBJ_BOARD	+= $(addprefix $(OUT_BOARD)/, $(patsubst %.c,%.o, $(notdir $(SRC_BOARD_C))))
OBJ_BOARD	+= $(addprefix $(OUT_BOARD)/, $(patsubst %.S,%.o, $(notdir $(SRC_BOARD_S))))
OBJ_FS		+= $(addprefix $(OUT_FS)/, $(patsubst %.c,%.o, $(notdir $(SRC_FS))))
OBJ_DRIVER	+= $(addprefix $(OUT_DRIVER)/, $(patsubst %.c,%.o, $(notdir $(SRC_DRIVER))))

OBJECT		= $(OUT_ARCH)/boot.o $(OBJ_ARCH) $(OBJ_PLATFORM) $(OBJ_BOARD) $(OBJ_KERNEL) $(OBJ_FS) $(OBJ_DRIVER)

all: $(OUT) $(OUT_KERNEL) $(OUT_ARCH) $(OUT_PLATFORM) $(OUT_BOARD) $(OUT_FS) $(OUT_DRIVER) $(boot_bin)

$(boot_bin) : $(boot_elf)
	$(OBJ_COPY) -O binary $(boot_elf) $(boot_bin)
	$(OBJ_DUMP) $(boot_elf) -D > $(boot_dump)

$(boot_elf) : $(OBJECT) $(LDS)
	$(LD) $(LDFLAG) -o $(boot_elf) $(OBJECT) $(LDPATH)

$(OUT) $(OUT_KERNEL) $(OUT_ARCH) $(OUT_PLATFORM) $(OUT_BOARD) $(OUT_FS) $(OUT_DRIVER):
	@ mkdir -p $@
	@ mkdir -p $(OUT_TOOLS)
	@ mkdir -p $(OUT_RAMDISK)

out/$(ARCH)/arch_ramdisk.o: arch/$(ARCH)/kernel/arch_ramdisk.S $(INCLUDE_DIR) out/ramdisk.img
	$(CC) $(CCFLAG) -c $< -o $@

$(OUT_ARCH)/%.o: %.c $(INCLUDE_DIR) arch/$(ARCH)/kernel/include/*.h
	$(CC) $(CCFLAG) -c $< -o $@

$(OUT_ARCH)/boot.o: arch/$(ARCH)/kernel/boot.S $(INCLUDE_DIR) arch/$(ARCH)/kernel/include/*.h
	$(CC) $(CCFLAG) -c arch/$(ARCH)/kernel/boot.S -o $@

$(OUT_ARCH)/%.o: %.S $(INCLUDE_DIR) arch/$(ARCH)/kernel/include/*.h
	$(CC) $(CCFLAG) -c $< -o $@

$(OUT_KERNEL)/%.o: %.c $(INCLUDE_DIR)
	$(CC) $(CCFLAG) -c $< -o $@

$(OUT_FS)/%.o: %.c $(INCLUDE_DIR)
	$(CC) $(CCFLAG) -c $< -o $@

$(OUT_DRIVER)/%.o: %.c $(INCLUDE_DIR)
	$(CC) $(CCFLAG) -c $< -o $@

$(OUT_PLATFORM)/%.o : %.c $(INCLUDE_DIR) arch/$(ARCH)/platform/$(PLATFORM)/include/*.h
	$(CC) $(CCFLAG) -c $< -o $@

$(OUT_PLATFORM)/%.o : %.S $(INCLUDE_DIR) arch/$(ARCH)/platform/$(PLATFORM)/include/*.h
	$(CC) $(CCFLAG) -c $< -o $@

$(OUT_BOARD)/%.o : %.c $(INCLUDE_DIR) 
	$(CC) $(CCFLAG) -c $< -o $@

$(OUT_TOOLS)/mklmfs: tools/mklmfs/mklmfs.c tools/mklmfs/lmfs.h tools/mklmfs/types.h
	gcc -o $(OUT_TOOLS)/mklmfs tools/mklmfs/mklmfs.c

$(OUT)/ramdisk.img: $(OUT_RAMDISK) $(OUT_TOOLS)/mklmfs
	@ $(OUT_TOOLS)/mklmfs $(OUT_RAMDISK) out/ramdisk.img

.PHONY: clean run app

clean:
	@ rm -rf out
	@ echo "All build has been cleaned"

run:
	@ cd tools/skyeye && skyeye -c ./skyeye.conf

app:
	cd userspace/bin/ && make
