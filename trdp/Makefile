#//
#// $Id$
#//
#// DESCRIPTION    trdp Makefile
#//
#// AUTHOR         Bernd Loehr, NewTec GmbH
#//
#// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
#// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#// Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013-2018. All rights reserved.
#//
#//	SB 2019-08-09: Added new lib target including tti, marshalling, xml parsing etc. and added install option
#//	BL 2019-06-18: V2 changes: dividing trdp_if.c into tlc_if.c, tlp_if.c and tlm_if.c
#//	BL 2019-06-13: Helm's Deep 96Board configuration added
#//	BL 2019-03-21: Prepared for TSN
#//	BL 2019-01-24: Reduce noise
#//	BL 2018-05-08: YOCTO / ARM7 configuration added
#//	BL 2018-02-02: Example renamed: cmdLineSelect -> echoCallback
#//	BL 2017-05-30: 64 bit Linux X86 config added
#//	BL 2017-05-08: 64 bit OSX config added
#//	BL 2016-02-11: Ticket #88 Cleanup makefiles, remove dependencies on external libraries

MAKEFLAGS += --quiet

#// Support for POSIX and VXWORKS, set buildsettings and config first!
 .EXPORT_ALL_VARIABLES:
# Check if configuration is present
ifeq (config/config.mk,$(wildcard config/config.mk)) 
# load target specific configuration
include config/config.mk
endif

include rules.mk

MD = mkdir -p
	
CFLAGS += -D$(TARGET_OS)

# Set paths
INCPATH += -I src/api
VOS_PATH += -I src/vos/$(TARGET_VOS)
VOS_INCPATH += -I src/vos/api -I src/common

vpath %.c src/common src/vos/common test/udpmdcom src/vos/$(TARGET_VOS) test example example/TSN test/diverse test/xml $(ADD_SRC)
vpath %.h src/api src/vos/api src/common src/vos/common $(ADD_INC)

INCLUDES = $(INCPATH) $(VOS_INCPATH) $(VOS_PATH)

#set up your specific paths for the lint *.lnt files etc
#by setting up an environment variable LINT_RULE_DIR like below
#LINT_RULE_DIR = /home/mkoch/svn_trdp/lint/pclint/9.00i/lnt/

#also you may like to point to a spcific path for the lint
#binary - have the environment variable LINT_BINPATH available
#within your build shell
FLINT = $(LINT_BINPATH)flint

# Set Objects
VOS_OBJS += vos_utils.o \
		vos_mem.o \
		vos_sock.o \
		vos_thread.o \
		vos_shared_mem.o

TRDP_OBJS += trdp_pdcom.o \
		trdp_utils.o \
		tlp_if.o \
		tlc_if.o \
		trdp_stats.o \
		$(VOS_OBJS)

# Optional objects for full blown TRDP usage
TRDP_OPT_OBJS += trdp_xml.o \
		tau_xml.o \
		tau_marshall.o \
		tau_dnr.o \
		tau_tti.o \
		tau_ctrl.o


# Set LINT Objects
LINT_OBJECTS = trdp_stats.lob\
		vos_utils.lob \
		vos_sock.lob \
		vos_mem.lob \
		vos_thread.lob \
		vos_shared_mem.lob \
		trdp_pdcom.lob \
		trdp_mdcom.lob \
		trdp_utils.lob \
		tlc_if.lob \
		tlp_if.lob \
		tlm_if.lob \
		trdp_stats.lob

# Set LDFLAGS
LDFLAGS += -L $(OUTDIR)

# Enable / disable MD Support
# by default MD_SUPPORT is always enabled (in current state)
ifeq ($(MD_SUPPORT),0)
	CFLAGS += -DMD_SUPPORT=0
else
	TRDP_OBJS += trdp_mdcom.o
	TRDP_OBJS += tlm_if.o
	CFLAGS += -DMD_SUPPORT=1
endif

ifeq ($(DEBUG), TRUE)
	OUTDIR = bld/output/$(ARCH)-dbg
else
	OUTDIR = bld/output/$(ARCH)-rel
endif

# Enable / Disable realtime scheduling (posix only)
ifeq ($(RT_THREADS), 1)
CFLAGS += -DRT_THREADS
endif

# Set LINT result outdir now after OUTDIR is known
LINT_OUTDIR  = $(OUTDIR)/lint
  
# Enable / disable Debug
ifeq ($(DEBUG),TRUE)
	CFLAGS += -g3 -O -DDEBUG
	LDFLAGS += -g3
	# Display the strip command and do not execute it
	STRIP = @$(ECHO) "do NOT strip: "
else
	CFLAGS += -Os  -DNO_DEBUG
endif

TARGETS = outdir libtrdp

ifneq ($(TARGET_OS),VXWORKS)
	TARGETS += example test pdtest mdtest xml marshall
else
	TARGETS += vtests
endif

ifeq ($(SOA_SUPPORT),1)
	TRDP_OPT_OBJS   += tau_so_if.o
	CFLAGS += -DSOA_SUPPORT
#	Option: Building with service orientation support!
endif

ifeq ($(TSN_SUPPORT),1)
	TARGETS += tsn
	# Additional sources for TSN support
	VOS_OBJS += vos_sockTSN.o
	CFLAGS += -DTSN_SUPPORT
#	Option: Building with TSN support
endif

ifeq ($(HIGH_PERF_INDEXED),1)
	TARGETS += highperf
	TRDP_OBJS += trdp_pdindex.o
	CFLAGS += -DHIGH_PERF_INDEXED
#	Option: Building high performance stack
endif

# Do a full build
ifeq ($(FULL_BUILD), 1)
	TRDP_OBJS += $(TRDP_OPT_OBJS)
endif

all:	$(TARGETS)

outdir:
	@$(MD) $(OUTDIR)

libtrdp:	outdir $(OUTDIR)/libtrdp.a

libtrdpap: outdir $(OUTDIR)/libtrdpap.a

example:	$(OUTDIR)/echoCallback $(OUTDIR)/receivePolling $(OUTDIR)/sendHello $(OUTDIR)/receiveHello $(OUTDIR)/sendData $(OUTDIR)/sourceFiltering

tsn:		$(OUTDIR)/sendTSN $(OUTDIR)/receiveTSN

test:		outdir $(OUTDIR)/getStats $(OUTDIR)/vostest $(OUTDIR)/MCreceiver $(OUTDIR)/test_mdSingle $(OUTDIR)/inaugTest $(OUTDIR)/localtest $(OUTDIR)/pdPull $(OUTDIR)/localtest2 $(OUTDIR)/localtest3 $(OUTDIR)/localtest4 $(OUTDIR)/pdMcRouting $(OUTDIR)/mdDataLength

pdtest:		outdir $(OUTDIR)/trdp-pd-test $(OUTDIR)/pd_responder $(OUTDIR)/testSub

mdtest:		outdir $(OUTDIR)/trdp-md-test $(OUTDIR)/trdp-md-test-fast $(OUTDIR)/trdp-md-reptestcaller $(OUTDIR)/trdp-md-reptestreplier #$(OUTDIR)/mdTest4

vtests:		outdir $(OUTDIR)/vtest

xml:		outdir $(OUTDIR)/trdp-xmlprint-test $(OUTDIR)/trdp-xmlpd-test

highperf:	outdir $(OUTDIR)/trdp-xmlpd-test-fast $(OUTDIR)/localtest2 $(OUTDIR)/trdp-pd-test-fast

marshall:	$(OUTDIR)/test_marshalling

%_config:
	cp -f config/$@ config/config.mk

$(OUTDIR)/%.o: %.c trdp_if_light.h trdp_types.h vos_types.h vos_sock.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OUTDIR)/libtrdp.a:		$(addprefix $(OUTDIR)/,$(notdir $(TRDP_OBJS)))
			@$(ECHO) ' ### Building the lib $(@F)'
			@$(RM) $@
			$(AR) cq $@ $^
			
$(OUTDIR)/libtrdpap.a:		$(addprefix $(OUTDIR)/,$(notdir $(TRDP_OBJS))) $(addprefix $(OUTDIR)/,$(notdir $(TRDP_OPT_OBJS)))
			@$(ECHO) ' ### Building the lib $(@F)'
			@$(RM) $@
			$(AR) cq $@ $^


###############################################################################
#
# rules for the demos
#
###############################################################################
				  
$(OUTDIR)/echoCallback:  echoCallback.c  $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building application $(@F)'
			$(CC) example/echoCallback.c \
				$(CFLAGS) $(INCLUDES) -o $@\
				-ltrdp \
			    $(LDFLAGS)
			@$(STRIP) $@

$(OUTDIR)/sourceFiltering:  sourceFilter/sourceFiltering.c  $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building application $(@F)'
			$(CC) example/sourceFilter/sourceFiltering.c \
			$(CFLAGS) $(INCLUDES) -o $@\
				-ltrdp \
				$(LDFLAGS)
				@$(STRIP) $@

$(OUTDIR)/receivePolling:  echoPolling.c  $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building application $(@F)'
			$(CC) example/echoPolling.c \
				$(CFLAGS) $(INCLUDES) -o $@\
				-ltrdp \
			    $(LDFLAGS)> /dev/null
			@$(STRIP) $@

$(OUTDIR)/receiveHello: receiveHello.c  $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building application $(@F)'
			$(CC) example/receiveHello.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			@$(STRIP) $@

$(OUTDIR)/sendHello:   sendHello.c  $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building application $(@F)'
			$(CC) example/sendHello.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			@$(STRIP) $@

$(OUTDIR)/receiveTSN: receiveTSN.c  $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building application $(@F)'
			$(CC) example/TSN/receiveTSN.c \
			-ltrdp \
			$(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			-o $@
			@$(STRIP) $@

$(OUTDIR)/sendTSN:   sendTSN.c  $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building application $(@F)'
			$(CC) example/TSN/sendTSN.c \
			-ltrdp \
			$(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			-o $@
			@$(STRIP) $@

$(OUTDIR)/sendData:   sendData.c  $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building application $(@F)'
			$(CC) example/sendData.c \
				-ltrdp \
				$(LDFLAGS) $(CFLAGS) $(INCLUDES) \
				-o $@
			@$(STRIP) $@

$(OUTDIR)/getStats:   diverse/getStats.c  $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building statistics commandline tool $(@F)'
			$(CC) test/diverse/getStats.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			@$(STRIP) $@

###############################################################################
#
# rule for the example
#
###############################################################################
$(OUTDIR)/mdManager: mdManager.c  $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building example MD application $(@F)'
			$(CC) example/mdManager.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			@$(STRIP) $@

###############################################################################
#
# rule for the various test binaries
#
###############################################################################
$(OUTDIR)/trdp-xmlprint-test:  trdp-xmlprint-test.c  $(OUTDIR)/libtrdp.a $(addprefix $(OUTDIR)/,$(notdir $(TRDP_OPT_OBJS)))
			@$(ECHO) ' ### Building application $(@F)'
			$(CC) $^ \
			$(CFLAGS) $(INCLUDES) -o $@ \
			-ltrdp \
			$(LDFLAGS)
			@$(STRIP) $@

$(OUTDIR)/trdp-xmlpd-test:  trdp-xmlpd-test.c  $(OUTDIR)/libtrdp.a $(addprefix $(OUTDIR)/,$(notdir $(TRDP_OPT_OBJS)))
			@$(ECHO) ' ### Building application $(@F)'
			$(CC) $^  \
			$(CFLAGS) $(INCLUDES) -o $@\
			-ltrdp \
			$(LDFLAGS)
			@$(STRIP) $@

$(OUTDIR)/trdp-xmlpd-test-fast:  trdp-xmlpd-test-fast.c  $(OUTDIR)/libtrdp.a $(addprefix $(OUTDIR)/,$(notdir $(TRDP_OPT_OBJS)))
			@$(ECHO) ' ### Building application $(@F)'
			$(CC) $^  \
			$(CFLAGS) $(INCLUDES) -o $@\
			-ltrdp \
			$(LDFLAGS)
			@$(STRIP) $@

$(OUTDIR)/mdTest4: mdTest4.c  $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building UDPMDCom test application $(@F)'
			$(CC) test/udpmdcom/mdTest4.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			@$(STRIP) $@

$(OUTDIR)/test_mdSingle: test_mdSingle.c $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building MD single test application $(@F)'
			$(CC) test/diverse/test_mdSingle.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			@$(STRIP) $@

$(OUTDIR)/inaugTest:   diverse/inaugTest.c  $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building republish test $(@F)'
			$(CC) test/diverse/inaugTest.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			@$(STRIP) $@

$(OUTDIR)/trdp-pd-test: $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building PD test application $(@F)'
			$(CC) test/pdpatterns/trdp-pd-test.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			@$(STRIP) $@

$(OUTDIR)/trdp-pd-test-fast: $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building PD test application $(@F)'
			$(CC) test/pdpatterns/trdp-pd-test-fast.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			@$(STRIP) $@

$(OUTDIR)/trdp-md-test: $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building MD test application $(@F)'
			$(CC) test/mdpatterns/trdp-md-test.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			@$(STRIP) $@

$(OUTDIR)/trdp-md-test-fast: $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building MD test application $(@F)'
			$(CC) test/mdpatterns/trdp-md-test-fast.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			@$(STRIP) $@

$(OUTDIR)/trdp-md-reptestcaller: $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building MD test Caller application $(@F)'
			$(CC) test/mdpatterns/rep-testcaller.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			@$(STRIP) $@

$(OUTDIR)/trdp-md-reptestreplier: $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building MD test Replier application $(@F)'
			$(CC) test/mdpatterns/rep-testreplier.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			@$(STRIP) $@

$(OUTDIR)/vtest: $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building vtest application $(@F)'
			$(CC) test/diverse/vtest.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			@$(STRIP) $@

$(OUTDIR)/vostest: $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building VOS test application $(@F)'
			$(CC) test/diverse/LibraryTests.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			@$(STRIP) $@

$(OUTDIR)/pd_responder: $(OUTDIR)/libtrdp.a pd_responder.c
			@$(ECHO) ' ### Building PD test application $(@F)'
			$(CC) test/diverse/pd_responder.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			@$(STRIP) $@

$(OUTDIR)/testSub: $(OUTDIR)/libtrdp.a subTest.c
			@$(ECHO) ' ### Building subscribe PD test application $(@F)'
			$(CC) test/diverse/subTest.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			@$(STRIP) $@

$(OUTDIR)/pdPull: $(OUTDIR)/libtrdp.a pdPull.c
			@$(ECHO) ' ### Building PD pull test application $(@F)'
			$(CC) test/diverse/pdPull.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			@$(STRIP) $@

$(OUTDIR)/localtest:   localtest/api_test.c  $(OUTDIR)/libtrdp.a $(addprefix $(OUTDIR)/,$(notdir $(TRDP_OPT_OBJS)))
			@$(ECHO) ' ### Building local loop test tool $(@F)'
			$(CC) $^  \
				$(CFLAGS)  -Wno-unused-variable $(INCLUDES) -o $@\
				-ltrdp \
				$(LDFLAGS)
			@$(STRIP) $@

$(OUTDIR)/localtest2:   localtest/api_test_2.c  $(OUTDIR)/libtrdp.a $(addprefix $(OUTDIR)/,$(notdir $(TRDP_OPT_OBJS)))
			@$(ECHO) ' ### Building local loop test tool $(@F)'
			$(CC) $^  \
				$(CFLAGS)  -Wno-unused-variable $(INCLUDES) -o $@\
				-ltrdp \
			$(LDFLAGS)
			@$(STRIP) $@

$(OUTDIR)/localtest3:   localtest/api_test_3.c  $(OUTDIR)/libtrdp.a $(addprefix $(OUTDIR)/,$(notdir $(TRDP_OPT_OBJS)))
			@$(ECHO) ' ### Building local loop test tool $(@F)'
			$(CC) $^  \
				$(CFLAGS)  -Wno-unused-variable $(INCLUDES) -o $@\
				-ltrdp \
			$(LDFLAGS)
			@$(STRIP) $@

$(OUTDIR)/localtest4:   localtest/api_test_4.c  $(OUTDIR)/libtrdp.a $(addprefix $(OUTDIR)/,$(notdir $(TRDP_OPT_OBJS)))
			@$(ECHO) ' ### Building local loop test tool $(@F)'
			$(CC) $^  \
				$(CFLAGS)  -Wno-unused-variable $(INCLUDES) -o $@\
				-ltrdp \
			$(LDFLAGS)
			@$(STRIP) $@

$(OUTDIR)/test_marshalling:   marshalling/test_marshalling.c  $(OUTDIR)/libtrdp.a $(addprefix $(OUTDIR)/,$(notdir $(TRDP_OPT_OBJS)))
			@$(ECHO) ' ### Building local loop test tool $(@F)'
			$(CC) $^  \
				$(CFLAGS)  -Wno-unused-variable $(INCLUDES) -o $@\
				-ltrdp \
			$(LDFLAGS)
			@$(STRIP) $@

$(OUTDIR)/MCreceiver: $(OUTDIR)/libtrdp.a
			@$(ECHO) ' ### Building MC joiner application $(@F)'
			$(CC) test/diverse/MCreceiver.c \
				-ltrdp \
				$(LDFLAGS) $(CFLAGS) $(INCLUDES) \
				-o $@
			@$(STRIP) $@

$(OUTDIR)/pdMcRouting: $(OUTDIR)/libtrdp.a pdMcRoutingTest.c
			@$(ECHO) ' ### Building PD MC routing test receiver application $(@F)'
			$(CC) test/diverse/pdMcRoutingTest.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			@$(STRIP) $@

$(OUTDIR)/mdDataLength: $(OUTDIR)/libtrdp.a mdDataLengthTest.c
			@$(ECHO) ' ### Building PD MC routing test receiver application $(@F)'
			$(CC) test/diverse/mdDataLengthTest.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			@$(STRIP) $@		

###############################################################################
#
# wipe out everything section - except the previous target configuration
#
###############################################################################
clean:
	rm -f -r bld/*
	rm -f -r doc/latex/*

unconfig:
	-$(RM) config/config.mk

distclean:	clean unconfig

###############################################################################
#                                                                      	
# Common lint for the whole system
#
############################################################################### 
lint:   loutdir $(LINT_OUTDIR)/final.lint

loutdir:
	@$(MD) $(LINT_OUTDIR)

LINTFLAGS = +v -i$(LINT_RULE_DIR) $(LINT_RULE_DIR)co-gcc.lnt -i ./src/api -i ./src/vos/api -i ./src/common -D$(TARGET_OS) $(LINT_SYSINCLUDE_DIRECTIVES)\
	-DMD_SUPPORT=1 -w3 -e655 -summary -u

# VxWorks will be the single non POSIX OS right now, MS Win uses proprietary build
# framework, so this condition will most likely fit also BSD/Unix targets
ifneq ($(TARGET_OS),VXWORKS)
LINTFLAGS += -DPOSIX
endif


$(LINT_OUTDIR)/final.lint: $(addprefix $(LINT_OUTDIR)/,$(notdir $(LINT_OBJECTS)))
	@$(ECHO) '### Lint Final'
	@$(ECHO) '### Final Lint Stage - Verifying inter module / system wide stuff' > $@
	$(FLINT) $(LINTFLAGS) $(SILENCE_LINT) -zero  $^ 1>>$@ 2>>$@

# Partial lint for single C files
$(LINT_OUTDIR)/%.lob : %.c
	@$(ECHO) ' --- Lint $(^F)'
	@$(ECHO) '### Lint file: $(^F)'  > $(LINT_OUTDIR)/$(^F).lint
	$(FLINT) $(LINTFLAGS) -u -oo\($@\)  -zero $< 1>>$(LINT_OUTDIR)/$(^F).lint 2>>$(LINT_OUTDIR)/$(^F).lint

###############################################################################
#
# create doxygen documentation
#
###############################################################################
doc:		doc/latex/refman.pdf

doc/latex/refman.pdf: Doxyfile trdp_if_light.h trdp_types.h
			@$(ECHO) ' ### Making the Reference Manual PDF'
			$(DOXYPATH)doxygen Doxyfile
			make -C doc/latex
			cp doc/latex/refman.pdf "doc/TCN-TRDP2-D-BOM-033-xx - TRDP Reference Manual.pdf"

###############################################################################
#
# install complete library
#
###############################################################################
ifdef INSTALLDIR
install:
	@$(MD) ../$(INSTALLDIR)
	cp $(OUTDIR)/libtrdpap.a ../$(INSTALLDIR)/libtrdpap.a
endif

###############################################################################
#
# help section for available build options
#
###############################################################################
help:
	@$(ECHO) " " >&2
	@$(ECHO) "BUILD TARGETS FOR TRDP" >&2
	@$(ECHO) "Load one of the configurations below with 'make <configuration>' first:" >&2
	@$(ECHO) "  " >&2
	@$(ECHO) "  * LINUX_config                 - Native build for Linux (uses host gcc regardless of 32/64 bit)" >&2
	@$(ECHO) "  * LINUX_X86_config             - Native build for Linux (Little Endian, uses host gcc 32Bit)" >&2
	@$(ECHO) "  * LINUX_X86_64_config          - Native build for Linux (Little Endian, uses host gcc 64Bit)" >&2
	@$(ECHO) "  * LINUX_X86_64_HP_config       - Native build for Linux as high performance library" >&2
	@$(ECHO) "  * LINUX_X86_64_HP_conform_config - Native build for Linux for Conformance Testing (2.1 API)" >&2
	@$(ECHO) "  * LINUX_PPC_config             - (experimental) Building for Linux on PowerPC using eglibc compiler (603 core)" >&2
	@$(ECHO) "  * LINUX_imx7_config            - Building for Linux ARM7/imx7 using YOCTO toolchain" >&2
	@$(ECHO) "  * LINUX_sama5d27_config        - Native build on NTSecureGateway HelmsDeep96" >&2
	@$(ECHO) "  * LINUX_TSN_config             - (experimental) Native build for RTLinux with basic TSN support" >&2
	@$(ECHO) "  * OSX_64_TSN_config            - Native build for macOS 64Bit with TSN" >&2
	@$(ECHO) "  * OSX_X86_64_config            - Native build for macOS 64Bit" >&2
	@$(ECHO) "  * OSX_X86_64_HP_config         - Native build for macOS 64Bit as high performance library" >&2
	@$(ECHO) "  * OSX_X86_64_SOA_TSN_config    - Native build for macOS 64Bit with SOA and TSN" >&2
	@$(ECHO) "  * POSIX_X86_config             - Native build for POSIX compliant systems" >&2
	@$(ECHO) "  * CENTOS_X86_64_config         - Native build for CentOS7 (Linux, little endian, 64Bit)" >&2
	@$(ECHO) "  * QNX_X86_config               - (experimental) Native (X86) build for QNX" >&2
	@$(ECHO) "  * VXWORKS_KMODE_PPC_config     - (experimental) Building for VXWORKS kernel mode for PowerPC" >&2
	@$(ECHO) " " >&2
	@$(ECHO) "Custom adaptation hints:" >&2
	@$(ECHO) " " >&2
	@$(ECHO) "a) create a buildsettings_%target file containing the paths for compilers etc. prepared to be uesd as source file for your build shell" >&2
	@$(ECHO) "b) adapt the most fitting *_config file to your needs and add it for comprehensibilty to the above list." >&2
	@$(ECHO) " " >&2
	@$(ECHO) "Then call 'make' or 'make all' to build everything. Contents of target all marked below" >&2
	@$(ECHO) "in the 'Other builds:' list with #" >&2
	@$(ECHO) "To build debug binaries, append 'DEBUG=TRUE' to the make command " >&2
	@$(ECHO) "To exclude message data support, append 'MD_SUPPORT=0' to the make command " >&2
	@$(ECHO) "To include realtime scheduling support, append 'RT_THREADS=1' to the make command " >&2
	@$(ECHO) " " >&2
	@$(ECHO) "Other builds:" >&2
	@$(ECHO) "  * make test      # build the test server application" >&2
	@$(ECHO) "  * make pdtest    # build the PDCom test applications" >&2
	@$(ECHO) "  * make mdtest    # build the UDPMDcom test application" >&2
	@$(ECHO) "  * make example   # build the example for MD communication, needs libuuid!" >&2
	@$(ECHO) "  * make libtrdp   # build the static library, only" >&2
	@$(ECHO) "  * make libtrdpap # build the static library including xml parsing, marshalling, dnr and tti" >&2
	@$(ECHO) "  * make xml       # build the xml test applications" >&2
	@$(ECHO) "  * make highperf  # build test applications for high performance (separate PD/MD threads)" >&2
	@$(ECHO) "  * make install   # requires INSTALLDIR to be set and copies the libtrdpap.a lib there" >&2
	@$(ECHO) " " >&2
	@$(ECHO) "Static analysis (currently in prototype state) " >&2
	@$(ECHO) "  * make lint      - build LINT analysis files using the LINT binary under $FLINT" >&2
	@$(ECHO) " " >&2
	@$(ECHO) "Build tree clean up helpers" >&2
	@$(ECHO) "  * make clean     - remove all binaries and objects of the current target" >&2
	@$(ECHO) "  * make unconfig  - remove the configuration file" >&2
	@$(ECHO) "  * make distclean - make clean unconfig" >&2
	@$(ECHO) " " >&2
	@$(ECHO) "Documentation generator" >&2
	@$(ECHO) "  * make doc       - build the documentation (refman.pdf)" >&2
	@$(ECHO) "                   - (needs doxygen installed in executable path)" >&2
	@$(ECHO) " " >&2

