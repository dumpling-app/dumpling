#-------------------------------------------------------------------------------
.SUFFIXES:
#-------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)

include $(DEVKITPRO)/wut/share/wut_rules

#-------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#-------------------------------------------------------------------------------
TARGET		:=	dumpling
BUILD		:=	build
SOURCES		:=	source/app
DATA		:=	data
INCLUDES	:=	include

#-------------------------------------------------------------------------------
# options for code generation
#-------------------------------------------------------------------------------
CFLAGS	:=	-g -Wall -Os -O2 -ffunction-sections -Wno-narrowing \
			$(MACHDEP)

CFLAGS	+=	$(INCLUDE) -D__WIIU__ -D__WUT__ -D__wiiu__

CXXFLAGS	:= $(CFLAGS) -std=c++20

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-g $(ARCH) $(RPXSPECS) -Wl,-Map,$(notdir $*.map)

LIBS	:=	-lstdc++ -lwut -lfat -liosuhax

#-------------------------------------------------------------------------------
# only include stubs when compiling with cemu target
#-------------------------------------------------------------------------------
ifdef CEMU_STUBS
CFLAGS	+=	-DCEMU_STUBS
SOURCES	+=	source/stub
endif
#-------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level
# containing include and lib
#-------------------------------------------------------------------------------
LIBDIRS	:=	$(PORTLIBS) $(WUT_ROOT) $(WUT_ROOT)/usr


#-------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#-------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#-------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#-------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#-------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#-------------------------------------------------------------------------------
	export LD	:=	$(CC)
#-------------------------------------------------------------------------------
else
#-------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#-------------------------------------------------------------------------------
endif
#-------------------------------------------------------------------------------

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 	:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) cemu clean dist all

#-------------------------------------------------------------------------------
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(CURDIR)/source/cfw/ios_usb
	@$(MAKE) --no-print-directory -C $(CURDIR)/source/cfw/ios_mcp
	@$(MAKE) --no-print-directory -C $(CURDIR)/source/cfw/ios_kernel
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#-------------------------------------------------------------------------------
cemu:
	@[ -d $(BUILD) ] || mkdir -p $(BUILD)
	@$(MAKE) CEMU_STUBS=1 --no-print-directory -f $(CURDIR)/Makefile

#-------------------------------------------------------------------------------
clean:
	@echo Clean files from app...
	@rm -fr $(BUILD) $(TARGET).rpx $(TARGET).elf
	@$(MAKE) clean --no-print-directory -C $(CURDIR)/source/cfw/ios_kernel
	@$(MAKE) clean --no-print-directory -C $(CURDIR)/source/cfw/ios_mcp
	@$(MAKE) clean --no-print-directory -C $(CURDIR)/source/cfw/ios_usb

#-------------------------------------------------------------------------------
dist:
	@echo Making dist folder
	@mkdir -p dist/wiiu/apps/dumpling
	@echo Put latest files into it
	@cp assets/meta.xml dist/wiiu/apps/dumpling/meta.xml
	@cp assets/dumpling-banner.png dist/wiiu/apps/dumpling/icon.png
	@cp $(TARGET).rpx dist/wiiu/apps/dumpling/$(TARGET).rpx

#-------------------------------------------------------------------------------

else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

#-------------------------------------------------------------------------------
# main targets
#-------------------------------------------------------------------------------
all	:	$(OUTPUT).rpx

$(OUTPUT).rpx	:	$(OUTPUT).elf
$(OUTPUT).elf	:	$(OFILES)

$(OFILES_SRC)	: $(HFILES_BIN)

#-------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#-------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#-------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#-------------------------------------------------------------------------------
endif
#-------------------------------------------------------------------------------
