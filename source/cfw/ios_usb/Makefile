ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

ifeq ($(filter $(DEVKITARM)/bin,$(PATH)),)
export PATH:=$(DEVKITARM)/bin:$(PATH)
endif

CC = arm-none-eabi-gcc
LINK = arm-none-eabi-ld
AS = arm-none-eabi-as
OBJCOPY = arm-none-eabi-objcopy
BIN2S = $(DEVKITPRO)/tools/bin/bin2s
CFLAGS += -Wall -Wno-unused-variable -Wno-unused-function -mbig-endian -std=gnu99 -march=armv5t -Os
LDFLAGS += -EB -Tlink.ld

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

$(PROJECTNAME): $(OFILES)
	@echo "Linking files for $(PROJECTNAME)..."
	@$(LINK) $(LDFLAGS) -o $(PROJECTNAME).elf $(OFILES)
	@$(OBJCOPY) -j .text -j .rodata -j .data -j .bss -O binary $(PROJECTNAME).elf $(PROJECTNAME).bin
	@xxd -i $(PROJECTNAME).bin | sed "s/unsigned/static const unsigned/g" > $(PROJECTNAME).h

dirs:
	@mkdir -p build

clean:
	@echo "Cleaning up files from $(PROJECTNAME)..."
	@rm -f build/*.o build/*.d
	@rmdir build 2>/dev/null || true
	@rm -f $(PROJECTNAME).elf $(PROJECTNAME).bin $(PROJECTNAME).s $(PROJECTNAME).h

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