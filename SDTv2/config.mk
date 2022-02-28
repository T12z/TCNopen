#//
#// COPYRIGHT   This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
#//             If a copy of the MPL was not distributed with this file, 
#//             You can obtain one at http://mozilla.org/MPL/2.0/.
#//             Copyright Alstom Transport SA or its subsidiaries and others, 2010-2022. All rights reserved.
#//
#// $Id:  $
#//
#// DESCRIPTION    Top level config.mk
#//
#// AUTHOR         Michael Koch          ...
#//

#
# Include the make variables (CC, etc...)
#

ECHO    = echo

ifeq ($(TARGET_OS), VXWORKS)
AS	= $(TCPATH)as$(TOOLCHAIN)
LD	= $(TCPATH)ld$(TOOLCHAIN)
CC	= $(TCPATH)cc$(TOOLCHAIN)
CPP	= $(CC) -E
AR	= $(TCPATH)ar$(TOOLCHAIN)
NM	= $(TCPATH)nm$(TOOLCHAIN)
STRIP	= $(TCPATH)strip$(TOOLCHAIN)
OBJCOPY = $(TCPATH)objcopy$(TOOLCHAIN)
OBJDUMP = $(TCPATH)objdump$(TOOLCHAIN)
RANLIB	= $(TCPATH)RANLIB$(TOOLCHAIN)
else 
ifeq ($(TARGET_OS), INTEGRITY)
CC	= cc$(TOOLCHAIN)
else
ifeq ($(TARGET_OS), CSS)
CC	= $(TCPATH)cc$(TOOLCHAIN)
AR	= $(TCPATH)ar$(TOOLCHAIN)
LD	= $(TCPATH)ld$(TOOLCHAIN)
else
ifeq ($(TARGET_OS), ARM_TI)
CC	= "D:/buildtools/TMS470_Code_Generation_Tools/5.0.4.0\bin\armcl.exe"
AR	= "D:/buildtools/TMS470_Code_Generation_Tools/5.0.4.0\bin\armar.exe"
else
#WINDOWS, LINUX and all other
AS	= $(TOOLCHAIN)as
LD	= $(TOOLCHAIN)ld
CC	= $(TOOLCHAIN)gcc
CPP	= $(CC) -E
AR	= $(TOOLCHAIN)ar
NM	= $(TOOLCHAIN)nm
STRIP	= $(TOOLCHAIN)strip
OBJCOPY = $(TOOLCHAIN)objcopy
OBJDUMP = $(TOOLCHAIN)objdump
RANLIB	= $(TOOLCHAIN)RANLIB
LN	= ln -s
endif
endif
endif
endif

ifeq ($(DEBUG), TRUE)
ifneq ($(TARGET_OS), INTEGRITY)
DBGFLAGS= -g
else
DBGFLAGS= -G
endif
else
ifneq ($(TARGET_OS), INTEGRITY)
#NON INTEGRITY section
ifneq ($(TARGET_OS), ARM_TI)
WARNFLAGS= -Wall
OPTFLAGS= -O2
else
#ARM section (TI-compiler)
#intentionally left empty
endif
else
#INTEGRITY section
OPTFLAGS= -Ospeed -OI
WARNFLAGS= -Wall
endif
endif

CFLAGS := $(DBGFLAGS) $(OPTFLAGS) $(RELFLAGS) \
	-Iapi \
	-Isrc \
	-D$(TARGET_OS) \
	$(WARNFLAGS) \
        $(PLATFORM_CPPFLAGS)

ifneq ($(TARGET_OS), ARM_TI)
CFLAGS := $(CFLAGS) -ansi
endif

AFLAGS_DEBUG := -Wa,-gstabs
AFLAGS := $(AFLAGS_DEBUG) -D__ASSEMBLY__ $(CPPFLAGS)

LDFLAGS :=  $(PLATFORM_LDFLAGS)

LINTFLAGS = +v -i ./api -D$(TARGET_OS) $(SDTLIBFEATURES) $(LINT_SYSINCLUDE_DIRECTIVES)\
	./ccuslint/integrity-C3.lnt 
	
#########################################################################

export	HPATH CROSS_COMPILE \
	AS LD CC CPP AR NM STRIP OBJCOPY OBJDUMP \
	MAKE
export	TEXT_BASE PLATFORM_CPPFLAGS PLATFORM_RELFLAGS CPPFLAGS CFLAGS AFLAGS

#########################################################################

%.s:	%.S
	$(CPP) $(AFLAGS) -o $@ $(CURDIR)/$<

%.o:	%.S
	$(CC) $(AFLAGS) -c -o $@ $(CURDIR)/$<

#### Code for Linux single process, Windows and VxWorks
ifeq ($(TARGET_OS), ARM_TI)
$(OUTDIR)/%.o: 	%.c
	@$(ECHO) ' --- Compile $(@F)'
	$(CC) $(CFLAGS) -I"D:/buildtools/TMS470_Code_Generation_Tools/5.0.4.0\include" -c $< --obj_directory=$(OUTDIR)
else
ifeq ($(TARGET_OS), INTEGRITY)
$(OUTDIR)/lib_a/%.o: 	%.c
	@$(ECHO) ' --- Compile $(@F)'
	$(CC) $(CFLAGS) $(SDT_LIB_A_CFLAGS) -c $< -o $@
$(OUTDIR)/lib_b/%.o: 	%.c
	@$(ECHO) ' --- Compile $(@F)'
	$(CC) $(CFLAGS) $(SDT_LIB_B_CFLAGS) -c $< -o $@
else
# LINUX and VxWorks Variants here
$(OUTDIR)/%.o: 	%.c
	@$(ECHO) ' --- Compile $(@F)'
	$(CC) $(CFLAGS) -c $< -o $@
endif #INTEGRITY
endif #ARM_TI

# Partial lint for single C files
$(LINT_OUTDIR)/%.c.lob : %.c
	@$(ECHO) ' --- Lint $(^F)'
	@$(ECHO) '### Lint file: $(^F)'  > $(LINT_OUTDIR)/$(^F).lint
	$(FLINT) $(LINTFLAGS) -u -oo\($@\)  -zero $< 1>>$(LINT_OUTDIR)/$(^F).lint 2>>$(LINT_OUTDIR)/$(^F).lint  
