#-------------------------------------------------------------------------------
.SUFFIXES:
#-------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)

#-------------------------------------------------------------------------------
# APP_NAME sets the long name of the application
# APP_SHORTNAME sets the short name of the application
# APP_AUTHOR sets the author of the application
#-------------------------------------------------------------------------------
APP_NAME		:= Dumpling
APP_SHORTNAME	:= Dumpling
APP_AUTHOR		:= Crementif and emiyl

include $(DEVKITPRO)/wut/share/wut_rules

#-------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
# CONTENT is the path to the bundled folder that will be mounted as /vol/content/
# ICON is the game icon, leave blank to use default rule
# TV_SPLASH is the image displayed during bootup on the TV, leave blank to use default rule
# DRC_SPLASH is the image displayed during bootup on the DRC, leave blank to use default rule
#-------------------------------------------------------------------------------
TARGET		:=	dumpling
BUILD		:=	build
SOURCES		:=	source/app \
				source/app/interfaces \
				source/utils
DATA		:=	data
INCLUDES	:=	include
CONTENT		:=
ICON		:=
TV_SPLASH	:=
DRC_SPLASH	:=

#-------------------------------------------------------------------------------
# options for code generation
#-------------------------------------------------------------------------------
CFLAGS		:=	-g -Wall -Os -ffunction-sections -fdata-sections -Wno-narrowing \
				$(MACHDEP)

ifdef USING_CEMU
CFLAGS		+=	$(INCLUDE) -D__WIIU__ -D__WUT__ -D__wiiu__ -DUSING_CEMU `freetype-config --cflags`
else
CFLAGS		+=	$(INCLUDE) -D__WIIU__ -D__WUT__ -D__wiiu__ -DUSE_LIBFAT -DUSE_LIBMOCHA `freetype-config --cflags`
endif

CXXFLAGS	:=	$(CFLAGS) -std=c++20

ASFLAGS		:=	-g $(ARCH)
LDFLAGS		=	-g $(ARCH) $(RPXSPECS) -Wl,-Map,$(notdir $*.map)

LIBS		:=	-lstdc++ -lwut -lfat -lmocha -liosuhax `freetype-config --libs`

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

CFILES			:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES			:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES		:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

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
export OFILES		:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE		:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
						$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
						-I$(CURDIR)/$(BUILD)

export LIBPATHS		:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifneq (,$(strip $(CONTENT)))
	export APP_CONTENT := $(TOPDIR)/$(CONTENT)
endif

ifneq (,$(strip $(ICON)))
	export APP_ICON := $(TOPDIR)/$(ICON)
else ifneq (,$(wildcard $(TOPDIR)/$(TARGET).png))
	export APP_ICON := $(TOPDIR)/$(TARGET).png
else ifneq (,$(wildcard $(TOPDIR)/icon.png))
	export APP_ICON := $(TOPDIR)/icon.png
endif

ifneq (,$(strip $(TV_SPLASH)))
	export APP_TV_SPLASH := $(TOPDIR)/$(TV_SPLASH)
else ifneq (,$(wildcard $(TOPDIR)/tv-splash.png))
	export APP_TV_SPLASH := $(TOPDIR)/tv-splash.png
else ifneq (,$(wildcard $(TOPDIR)/splash.png))
	export APP_TV_SPLASH := $(TOPDIR)/splash.png
endif

ifneq (,$(strip $(DRC_SPLASH)))
	export APP_DRC_SPLASH := $(TOPDIR)/$(DRC_SPLASH)
else ifneq (,$(wildcard $(TOPDIR)/drc-splash.png))
	export APP_DRC_SPLASH := $(TOPDIR)/drc-splash.png
else ifneq (,$(wildcard $(TOPDIR)/splash.png))
	export APP_DRC_SPLASH := $(TOPDIR)/splash.png
endif

.PHONY: $(BUILD) cemu clean dist all

#-------------------------------------------------------------------------------
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(CURDIR)/source/cfw/ios_usb
	@$(MAKE) --no-print-directory -C $(CURDIR)/source/cfw/ios_mcp
	@$(MAKE) --no-print-directory -C $(CURDIR)/source/cfw/ios_fs
	@$(MAKE) --no-print-directory -C $(CURDIR)/source/cfw/ios_kernel
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#-------------------------------------------------------------------------------
cemu:
	@[ -d $(BUILD) ] || mkdir -p $(BUILD)
	@$(MAKE) USING_CEMU=1 --no-print-directory -f $(CURDIR)/Makefile

run_cemu_wsl: cemu
	@$(shell $(CURDIR)/cemu/Cemu.exe -g "$(shell wslpath -a -w $(OUTPUT).rpx)")

run_cemu_wsl_interpreter: cemu
	@$(shell $(CURDIR)/cemu/Cemu.exe --force-interpreter -g "$(shell wslpath -a -w $(OUTPUT).rpx)")

#-------------------------------------------------------------------------------
clean:
	@echo Clean files from app...
	@rm -fr $(BUILD) $(TARGET).wuhb $(TARGET).rpx $(TARGET).elf
	@$(MAKE) clean --no-print-directory -C $(CURDIR)/source/cfw/ios_kernel
	@$(MAKE) clean --no-print-directory -C $(CURDIR)/source/cfw/ios_fs
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
	@echo Zip up a release zip
	@rm -f dist/dumpling.zip
	@cd dist && zip -q -r ./dumpling.zip ./wiiu && cd ..

#-------------------------------------------------------------------------------

else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

#-------------------------------------------------------------------------------
# main targets
#-------------------------------------------------------------------------------
all: $(OUTPUT).wuhb

$(OUTPUT).wuhb	:	$(OUTPUT).rpx
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
