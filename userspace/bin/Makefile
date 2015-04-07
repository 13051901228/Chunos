ARCH 		:= x86
CROSS_COMPILE 	:= i686-linux-android-
CC 		:= $(CROSS_COMPILE)gcc
LD 		:= $(CROSS_COMPILE)ld
OBJCOPY		:= $(CROSS_COMPILE)objcopy
OBJDUMP 	:= $(CROSS_COMPILE)objdump
AR	 	:= $(CROSS_COMPILE)ar
STRIP	 	:= $(CROSS_COMPILE)strip

LIBC_DIR	:= $(shell pwd)/../diet-libc-$(ARCH)
LIBS_DIR	:= $(LIBC_DIR)/lib
INCLUDE_DIR 	:= $(LIBC_DIR)/include

OUT 		:= $(shell pwd)/../../out/userspace/bin
RAMDISK_BIN 	:= ../../out/ramdisk/bin

LDS 		:= $(shell pwd)/app.lds
CCFLAG 		:= -nostdlib -fno-builtin -I$(INCLUDE_DIR)
LDFLAG 		:= -T$(LDS) -nostdlib
LDPATH 		:= -ldietlibc -lgcc -lm -L$(LIBS_DIR)
LDLIBS		:= $(LIBS_DIR)/start.o $(LIBS_DIR)/raise.o $(LIBS_DIR)/__aeabi_unwind_cpp.o

#OBJ_DIRS	: find all the directory which may contains an application
#target_tmp	: tmp target list 
#makefiles	: makefile which may used to build applications
OBJ_DIRS	= $(shell find . -maxdepth 1 -type d)
target_tmp	= $(notdir $(filter-out ., $(OBJ_DIRS)))
makefiles	= $(addsuffix /Makefile, $(target_tmp))

#since the direcotry may not have a makefile so we adjust the 
#target list.
tmp1		= $(shell find $(target_tmp) -name Makefile)
target		= $(filter $(subst /,,$(dir $(tmp1))), $(target_tmp))

source		= $(shell find . -name *.c)
inc		= $(shell find . -name *.h)
object		= $(patsubst %.c, %.o, $(source))

-include $(makefiles)

all: cdir $(target)

cdir:
	@ mkdir -p $(RAMDISK_BIN)
	@ mkdir -p $(OUT)

$(object): %.o : %.c
	@ mkdir -p $(OUT)/$(dir $@)
	$(CC) $(CCFLAG) -c $< -o $(OUT)/$@

$(target): $(object)
	@ cd $(OUT)/$@
	$(LD) $(LDFLAG) -o $(OUT)/$@/$@ $(LDLIBS) $(addprefix $(OUT)/$@/, $(obj-$@)) $(LDPATH)
	@ cp $(OUT)/$@/$@ $(RAMDISK_BIN)
	@ $(STRIP) -s $(RAMDISK_BIN)/$@
	@ $(OBJDUMP) -D $(OUT)/$@/$@ > $(OUT)/$@/$@.s
