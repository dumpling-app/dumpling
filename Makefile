#-------------------------------------------------------------------------------
.SUFFIXES:
#-------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>/devkitpro/devkitPPC")
endif

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>/devkitpro/devkitARM")
endif

TOPDIR ?= $(CURDIR)

#-------------------------------------------------------------------------------
# APP_NAME sets the long name of the application
# APP_SHORTNAME sets the short name of the application
# APP_AUTHOR sets the author of the application
#-------------------------------------------------------------------------------
APP_NAME		:=	Dumpling
APP_SHORTNAME	:=	Dumpling
APP_AUTHOR		:=	Crementif and emiyl

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
				source/utils \
				source/utils/fatfs
DATA		:=	data
INCLUDES	:=	include
CONTENT		:=
ICON		:=	assets/dumpling-icon.png
TV_SPLASH	:=	assets/dumpling-tv-boot.png
DRC_SPLASH	:=	assets/dumpling-drc-boot.png

#-------------------------------------------------------------------------------
# options for code generation
#-------------------------------------------------------------------------------
CFLAGS		:=	-g -Wall -Os -ffunction-sections -fdata-sections -Wno-narrowing \
				$(MACHDEP) $(shell $(DEVKITPRO)/portlibs/ppc/bin/freetype-config --cflags)

CFLAGS		+=	$(INCLUDE) -D__WIIU__ -D__WUT__ -D__wiiu__

ifdef USE_DEBUG_STUBS
CFLAGS		+=	-DUSE_DEBUG_STUBS=1
else
CFLAGS		+=	-DUSE_DEBUG_STUBS=0
endif

ifdef USE_RAMDISK
CFLAGS		+=	-DUSE_RAMDISK=1
else
CFLAGS		+=	-DUSE_RAMDISK=0
endif

CXXFLAGS	:=	$(CFLAGS) -std=c++20

ASFLAGS		:=	-g $(ARCH)
LDFLAGS		=	-g $(ARCH) $(RPXSPECS) -Wl,-Map,$(notdir $*.map)

LIBS		:=	-lstdc++ -lwut -lmocha $(shell $(DEVKITPRO)/portlibs/ppc/bin/freetype-config --libs)

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

.PHONY: $(BUILD) debug discimg clean dist all

#-------------------------------------------------------------------------------
all: $(BUILD)

$(BUILD):
	@$(shell [ -d $@ ] || mkdir -p $@)
	@$(MAKE) CC=$(DEVKITARM)/bin/arm-none-eabi-gcc CXX=$(DEVKITARM)/devkitARM/bin/arm-none-eabi-g++ -C $(CURDIR)/source/cfw/ios_usb
	@$(MAKE) CC=$(DEVKITARM)/bin/arm-none-eabi-gcc CXX=$(DEVKITARM)/devkitARM/bin/arm-none-eabi-g++ -C $(CURDIR)/source/cfw/ios_mcp
	@$(MAKE) CC=$(DEVKITARM)/bin/arm-none-eabi-gcc CXX=$(DEVKITARM)/devkitARM/bin/arm-none-eabi-g++ -C $(CURDIR)/source/cfw/ios_fs
	@$(MAKE) CC=$(DEVKITARM)/bin/arm-none-eabi-gcc CXX=$(DEVKITARM)/devkitARM/bin/arm-none-eabi-g++ -C $(CURDIR)/source/cfw/ios_kernel
	@$(MAKE) CC=$(DEVKITPPC)/bin/powerpc-eabi-gcc CXX=$(DEVKITPPC)/bin/powerpc-eabi-g++ -C $(BUILD) -f $(CURDIR)/Makefile

#-------------------------------------------------------------------------------
debug:
	@$(shell [ -d $(BUILD) ] || mkdir -p $(BUILD))
	@$(MAKE) USE_DEBUG_STUBS=1 -f $(CURDIR)/Makefile

discimg:
	@$(shell [ -d $(BUILD) ] || mkdir -p $(BUILD))
	@echo Recreating fatfs disk image...
	@rm -f ./cemu/sdcard/split0.img ./cemu/sdcard/split1.img ./cemu/sdcard/split2.img ./cemu/sdcard/split3.img ./cemu/sdcard/split4.img
	@cd ./cemu/sdcard/ && split -d --suffix-length=1 --bytes=2147483648 --additional-suffix=.img empty.img split || cd ./../../
	@echo Recreated fatfs disk image!
	@$(MAKE) USE_DEBUG_STUBS=1 USE_RAMDISK=1 -f $(CURDIR)/Makefile

#-------------------------------------------------------------------------------
clean:
	@echo Clean files from app...
	@rm -fr $(BUILD) $(TARGET).wuhb $(TARGET).rpx $(TARGET).elf
	@$(MAKE) clean -C $(CURDIR)/source/cfw/ios_kernel
	@$(MAKE) clean -C $(CURDIR)/source/cfw/ios_fs
	@$(MAKE) clean -C $(CURDIR)/source/cfw/ios_mcp
	@$(MAKE) clean -C $(CURDIR)/source/cfw/ios_usb

#-------------------------------------------------------------------------------
dist:
	@echo Making dist folder
	@mkdir -p dist/wiiu/apps/dumpling
	@echo Put latest files into it
	@cp assets/meta.xml dist/wiiu/apps/dumpling/meta.xml
	@cp assets/dumpling-banner.png dist/wiiu/apps/dumpling/icon.png
	@cp $(TARGET).rpx dist/wiiu/apps/dumpling/$(TARGET).rpx
	@cp $(TARGET).wuhb dist/wiiu/apps/dumpling/$(TARGET).wuhb
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
