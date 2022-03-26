ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

ifeq ($(filter $(DEVKITARM)/bin,$(PATH)),)
export PATH:=$(DEVKITARM)/bin:$(PATH)
endif

CC = arm-none-eabi-gcc
LINK = arm-none-eabi-gcc
AS = arm-none-eabi-as
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
BIN2S = $(DEVKITPRO)/tools/bin/bin2s
CFLAGS += -Wall -mbig-endian -std=gnu11 -mcpu=arm926ej-s -msoft-float -mfloat-abi=soft -Os -ffunction-sections
LDFLAGS += -nostartfiles -nodefaultlibs -mbig-endian -Wl,-T,link.ld
LIBS += -lgcc

# Change LDFLAGS to -Wl,-T,link.ld and CFLAGS to -fdata-sections for information on why something's getting discarded

CFILES = $(wildcard source/*.c)
BINFILES = $(wildcard data/*.bin)
OFILES = $(BINFILES:data/%.bin=build/%.bin.o)
OFILES += $(CFILES:source/%.c=build/%.o)
DFILES = $(CFILES:source/%.c=build/%.d)
SFILES = $(wildcard source/*.s)
OFILES += $(SFILES:source/%.s=build/%.o)
PROJECTNAME = ${shell basename "$(CURDIR)"}
CWD = "$(CURDIR)"


#---------------------------------------------------------------------------------
.PHONY: all dirs $(PROJECTNAME) clean

all: dirs $(PROJECTNAME)

$(PROJECTNAME): $(PROJECTNAME).h $(PROJECTNAME)_syms.h

$(PROJECTNAME).h: $(PROJECTNAME).bin
	@echo "Convert payload.bin to includable header..."
	@xxd -i $(PROJECTNAME).bin | sed "s/unsigned/static const unsigned/g" > $(PROJECTNAME).h

$(PROJECTNAME)_syms.h: $(PROJECTNAME).bin
	@echo "#ifndef $(PROJECTNAME)_SYMS_H" > $@
	@echo "#define $(PROJECTNAME)_SYMS_H" >> $@
	@$(OBJDUMP) -EB -t -marm $(PROJECTNAME).elf | grep 'g     F .text' | grep -v '.hidden' | awk '{print "#define " $$6 " 0x" $$1}' >> $@
	@$(OBJDUMP) -EB -t -marm $(PROJECTNAME).elf | grep -e 'g       .text' -e '_bss_' | awk '{print "#define " $$5 " 0x" $$1}' >> $@
	@echo "#endif" >> $@

$(PROJECTNAME).bin: $(PROJECTNAME).elf
	@echo "Turn payload.elf into payload.bin..."
	@$(OBJCOPY) -j .text -j .rodata -j .data -O binary $(PROJECTNAME).elf $(PROJECTNAME).bin

$(PROJECTNAME).elf: $(OFILES)
	@echo "Linking files for $(PROJECTNAME)..."
	@$(LINK) $(LDFLAGS) -o $(PROJECTNAME).elf $(sort $(filter-out build/crt0.o, $(OFILES))) $(LIBDIRS) $(LIBS)

dirs:
	@mkdir -p build

clean:
	@echo "Cleaning up files from $(PROJECTNAME)..."
	@rm -f build/*.o build/*.d
	@rmdir build 2>/dev/null || true
	@rm -f $(PROJECTNAME).elf $(PROJECTNAME).bin $(PROJECTNAME).s $(PROJECTNAME).h $(PROJECTNAME)_syms.h

#---------------------------------------------------------------------------------

-include $(DFILES)

build/%.o: source/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	@$(CC) -MM $< > build/$*.d

build/%.o: source/%.s
	$(CC) $(CFLAGS) -xassembler-with-cpp -c $< -o $@
	@$(CC) -MM $< > build/$*.d

build/%.bin.o: data/%.bin
	@echo $(notdir $<)
	@$(bin2o)