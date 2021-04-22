APP=bin/app

TOOLCHAIN=arm-none-eabi
CC=gcc
AS=as
LD=ld
OBJDUMP=objdump
READELF=readelf

CPU=cortex-m0

CCFLAGS=-std=gnu2x -g -Wall -Wextra -pedantic -pedantic-errors -Werror -mcpu=$(CPU) -mthumb -fno-builtin -O3
# Is it a bug from arm-none-eabi-gcc 10.1.0?
#CCFLAGS:=$(CCFLAGS) -fno-auto-inc-dec -fno-reorder-blocks

ASFLAGS=-mcpu=$(CPU) -mthumb

INCLUDES=-Isrc/

CFILES=$(wildcard src/*.c)
SFILES=$(wildcard src/*.s) src/vector.s
OFILES=$(patsubst %.c,%.o,$(CFILES)) $(patsubst %.s,%.o,$(SFILES))

LDSCRIPT=src/samd21j17a_flash.ld

$(APP): $(OFILES)
	$(TOOLCHAIN)-$(LD) -o $@ $^ -T $(LDSCRIPT)
	$(TOOLCHAIN)-$(OBJDUMP) -xhD $@ > $@.lst

program: gdb/initializer $(APP)
	$(TOOLCHAIN)-gdb -q -x $< -ex "program $(APP)"

debug: gdb/initializer $(APP)
	$(TOOLCHAIN)-gdb -q -x $< -ex "program-and-debug $(APP)"

src/vector.s: src/tools/irq_gen.py
	python3 $< > $@

%.o: %.c
	$(TOOLCHAIN)-$(CC) $(CCFLAGS) $(INCLUDES) -o $@ -c $<

%.o: %.s
	$(TOOLCHAIN)-$(AS) $(ASFLAGS) -o $@ -c $<

clean:
	rm -f $(APP) $(APP).lst $(OFILES) src/vector.s

build:
	mkdir -p bin/
