#//
#// $Id:  $
#//
#// DESCRIPTION    Basic Make Rules to create TRDP
#//
#// AUTHOR         Bombardier Transportation GmbH
#//
#// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0 
#// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/
#// Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2014. All rights reserved.
#//

#
# Include the make variables (CC, etc...)
#

ECHO    = echo

ifeq ($(TARGET_OS), VXWORKS)
AS	= $(TCPATH)as$(TCPOSTFIX)
LD	= $(TCPATH)ld$(TCPOSTFIX)
CC	= $(TCPATH)cc$(TCPOSTFIX)
CPP	= $(CC) -E
AR	= $(TCPATH)ar$(TCPOSTFIX)
NM	= $(TCPATH)nm$(TCPOSTFIX)
STRIP	= @echo "No stripping in VxWorks"
OBJCOPY = $(TCPATH)objcopy$(TCPOSTFIX)
OBJDUMP = $(TCPATH)objdump$(TCPOSTFIX)
RANLIB	= $(TCPATH)RANLIB$(TCPOSTFIX)
else
ifeq ($(TARGET_OS), INTEGRITY)
CC	= cc$(TCPOSTFIX)
else
#WINDOWS, LINUX and all other
AS	= $(TCPATH)$(TCPREFIX)as
LD	= $(TCPATH)$(TCPREFIX)ld
CC	= $(TCPATH)$(TCPREFIX)gcc
CPP	= $(TCPATH)$(CC) -E
AR	= $(TCPATH)$(TCPREFIX)ar
NM	= $(TCPATH)$(TCPREFIX)nm
STRIP	= $(TCPATH)$(TCPREFIX)strip
OBJCOPY = $(TCPATH)$(TCPREFIX)objcopy
OBJDUMP = $(TCPATH)$(TCPREFIX)objdump
RANLIB	= $(TCPATH)$(TCPREFIX)RANLIB
LN	= ln -s
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
OPTFLAGS= -O2
ifneq ($(TARGET_OS), ARM_TI)
WARNFLAGS= -Wall
endif
else
OPTFLAGS= -Ospeed -OI
WARNFLAGS= -Wall
endif
endif

       
