PROJ_NAME = EPC_DCBM
CC = arm-none-eabi-gcc
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump

MAP_PATH=../obj/$(PROJ_NAME).map

ARCH_FLAGS = -mcpu=cortex-m4 -mthumb #-mthumb-interwork
ARCH_FLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16

INCFLAGS += -I"../inc" -I"../sys" -I"../sys/cmsis/core/inc"

#CFLAGS += -ggdb
CFLAGS = -O0 -g 
CFLAGS += -fmessage-length=0 -fsigned-char -fno-common
CFLAGS += -fdata-sections -ffunction-sections 
CFLAGS += -Wall -Wno-comment -Wextra -Werror#-Wextra -Wundef -Wstrict-prototypes
CFLAGS += -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-unused-variable
CFLAGS += -Wno-implicit-fallthrough -Wno-type-limits -Wno-missing-field-initializers
CFLAGS += -Wno-sign-compare -Wno-absolute-value -Wno-unused-function
CFLAGS += -DHC32F460
# CFLAGS += -DHC32F460 -DUSE_DDL_DRIVER
CFLAGS += -std=c11 #-g3

ASFLAGS =  -Wa,--warn #-g

LFLAGS = $(ARCH_FLAGS)
LFLAGS += -nostartfiles
LFLAGS += -Wl,--gc-sections
LFLAGS += -Wl,-Map=$(MAP_PATH),--cref
LFLAGS += -Wl,--print-memory-usage
LFLAGS += --specs=nano.specs #--specs=rdimon-v2m.specs #--specs=nano.specs
LFLAGS += -T"../sys/ld/hc32f460xe_flash.ld"

# #config addr to save defalut value
# LFLAGS += -Wl,--section-start=.config_addr=0x0001fe00

obj = ../obj
obj_t = $(patsubst %.c,%.o,$(notdir $(wildcard *.c ../src/*.c ../src/grbl/*.c)))
obj_ts = startup_hc32f460.o

obj_s = $(patsubst %,$(obj)/%,$(obj_t))
obj_s += $(patsubst %,$(obj)/%,$(obj_ts))

SRC = ../src ../driver/src

vpath %.c ../src ../sys ../src/grbl
vpath %.s ../sys
vpath %.o ../obj

$(PROJ_NAME).dis:$(PROJ_NAME).hex
	$(OBJDUMP) -S -h $(obj)/$(PROJ_NAME).elf > $(obj)/$(PROJ_NAME).dis
	compiledb -n -o ../compile_commands.json make

$(PROJ_NAME).hex:$(PROJ_NAME).elf
	$(OBJCOPY) -Oihex $(obj)/$^ $(obj)/$@

$(PROJ_NAME).elf:$(obj_t) $(obj_ts)
	$(CC) $(obj_s) $(LFLAGS) -o $(obj)/$@

startup_hc32f460.o:startup_hc32f460.s
	$(CC) $(ASFLAGS) $^ -c -o $(obj)/$@

$(obj_t) : %.o : %.c
	$(CC) $(ARCH_FLAGS) $(CFLAGS) $(INCFLAGS) $^ -c -o $(obj)/$@

.PHONY:test
test:
	@echo "$(obj_t)"
	@echo "***************************************************************"
	@echo "$(obj_s)"
	@echo "***************************************************************"
	@echo "$(LFLAGS)"
	@echo "***************************************************************"
	@echo "$(ARCH_FLAGS)"
	@echo "***************************************************************"
	@echo "$(CFLAGS)"
	@echo "***************************************************************"
	@echo "$(CC)"
	@echo "***************************************************************"
	@echo "$(INCFLAGS)"

.PHONY:dump
dump:
	$(OBJDUMP) -S -h $(obj)/$(PROJ_NAME).elf > $(obj)/$(PROJ_NAME).dis

.PHONY:clean
clean:
	del /q "..\obj\*"

.PHONY:flash
flash:
	pyocd erase -t hc32f460xe -c
	pyocd flash -t hc32f460xe "../obj/$(PROJ_NAME).hex"

.PHONY:reset
reset:
	pyocd reset -t hc32f460xe

.PHONY:map
map:
	gvim $(MAP_PATH)

t=''

	# type nul>../src/$(t).c
	# type nul>../inc/$(t).h
.PHONY:create
create:
	powershell -c "New-Item -Path '..\src\$(t).c' -ItemType 'File' -Force"
	powershell -c "New-Item -Path '..\inc\$(t).h' -ItemType 'File' -Force"

.PHONY:del
del:
	del "../src/$(t).c"
	del "../inc/$(t).h"

.PHONY:tags
tags:
	ctags -R -f ../tags --fields=+iaS --extras=+q --tag-relative ../

#using python script, if not found, use this to install: pip install compiledb
.PHONY:db
db:
	compiledb -n -o ../compile_commands.json make

.PHONY:cmd
cmd:
	pyocd cmd -t hc32f460xe

