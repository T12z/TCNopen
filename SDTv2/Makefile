#//
#// COPYRIGHT   This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
#//             If a copy of the MPL was not distributed with this file, 
#//             You can obtain one at http://mozilla.org/MPL/2.0/.
#//             Copyright Alstom Transport SA or its subsidiaries and others, 2010-2022. All rights reserved.
#//
#// $Id:  $
#//
#// DESCRIPTION    Top level Makefile
#//
#// AUTHOR         Michael Koch          ...
#//


VER_MAJ = 2
VER_MIN = 4.0
NAME_SO = libsdt.so.$(VER_MAJ)
NAME_A = libsdt.a
NAME_LIM = libsdt.o
NAME_SO_ONLY = libsdt.so


 .EXPORT_ALL_VARIABLES:

# make VERBOSE=1 if you want so all output
#ifdef VERBOSE
#SILENCE_CMD =
#else
#SILENCE_CMD = > /dev/null
#.SILENT:
#endif

ifeq ($(OS), Windows_NT)
MD = mkdir.bat
CP = copy
RM = del /F /S /Q

else
TOPDIR	:= $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)
MD = mkdir -p
CP = cp -f
RM = rm -rf
TAR = tar czf
DLU = makedlu
LN = ln -s
FLINT = flint
endif

ifeq (config/config.mk,$(wildcard config/config.mk)) 
# load ARCH and CPU specific configuration
include config/config.mk

CROSS_COMPILE:=$(TOOLCHAIN)

ifeq ($(DEBUG), TRUE)
	OUTDIR = "output/$(ARCH)-dbg"
else
	OUTDIR = "output/$(ARCH)-rel"
endif
LINT_OUTDIR  = $(OUTDIR)/lint
endif

# load global configuration
include config.mk


##org

vpath %.c src

OBJS = sdt_validator.o \
	   sdt_mutex.o \
	   sdt_mvb.o \
	   sdt_ipt.o \
	   sdt_wtb.o \
	   sdt_uic.o

LOBJS = sdt_validator.c.lob \
	   sdt_mvb.c.lob \
	   sdt_ipt.c.lob \
	   sdt_wtb.c.lob \
	   sdt_uic.c.lob \
	   sdt_mutex.c.lob
#########################################################################
#
# Build rules
#
#########################################################################
all:		outdir lib_obj lib 

test_app: test/test.c       
	$(CC) $(CFLAGS)  test/test.c -o test/test.out -L$(OUTDIR)/$(NAME_SO).$(VER_MIN)

### GW-S build
ifeq ($(TARGET_OS), ARM_TI)
lib:

lib_obj: outdir $(OUTDIR)/$(NAME_A)

endif
			    
### generic LINUX build
ifeq ($(TARGET_OS), LINUX)
lib: outdir $(OUTDIR)/$(NAME_SO).$(VER_MIN)

lib_obj: outdir $(OUTDIR)/$(NAME_A)

endif

### generic vxWorks build (HMI411 SP, CSS3)
ifeq ($(TARGET_OS), VXWORKS)
lib:	

lib_obj: outdir $(OUTDIR)/$(NAME_A)

endif

### CCU-S build
ifeq ($(TARGET_OS), INTEGRITY)
lib:	

lib_obj: outdir $(OUTDIR)/lib_a/$(INT_LIB_A) $(OUTDIR)/lib_b/$(INT_LIB_B)

endif

### CCS1 build for LIM
ifeq ($(TARGET_OS), CSS)
lib:	

lib_obj: outdir $(OUTDIR)/$(NAME_A)

endif


$(OUTDIR)/$(NAME_SO).$(VER_MIN): 	$(addprefix $(OUTDIR)/,$(notdir $(OBJS)))
			@$(ECHO) ' ### Shared Library $(@F) $(ARCH)'
			$(RM) -f $@
			$(CC) $(LDFLAGS) -shared -o $(@) $^ 
			cd $(OUTDIR); md5sum $(NAME_SO).$(VER_MIN) > $(NAME_SO).$(VER_MIN).md5
			$(MD) $(OUTDIR)/dl2/lib; cp $(OUTDIR)/$(NAME_SO).$(VER_MIN) $(OUTDIR)/dl2/lib
			cd $(OUTDIR)/dl2/lib; $(LN) $(NAME_SO).$(VER_MIN) $(NAME_SO);$(LN) $(NAME_SO).$(VER_MIN) $(NAME_SO_ONLY)
			cd $(OUTDIR)/dl2; tar cf sdtv2.tar lib
			cd $(OUTDIR)/dl2; makedlu sdtv2.tar sdtv2 $(VER_MAJ).$(VER_MIN).1 DLU_TYPE_LINUX_TAR TPATH /usr/local/sdtv2.tar OUTFILE sdtv2_$(VER_MAJ).$(VER_MIN).1.dl2
			cd $(OUTDIR)/dl2; md5sum sdtv2_$(VER_MAJ).$(VER_MIN).1.dl2 > sdtv2_$(VER_MAJ).$(VER_MIN).1.dl2.md5
			cp $(OUTDIR)/dl2/sdtv2_$(VER_MAJ).$(VER_MIN).1.dl2* $(OUTDIR)
		

#
# Incremental library including TDC. Used as runtime loadable object
# for VxWorks only.
#

$(OUTDIR)/$(NAME_A):	$(addprefix $(OUTDIR)/,$(notdir $(OBJS)))
			@$(ECHO) ' ### Incremental Library $(@F) $(ARCH)'
#			$(RM) -f $@
ifeq ($(TARGET_OS), ARM_TI)
			$(AR) rus $@ $^
else
			$(AR) crus $@ $^
endif					

#$(OUTDIR)/$(NAME_LIM):	$(addprefix $(OUTDIR)/,$(notdir $(OBJS)))
#			@$(ECHO) ' ### Incremental Library $(@F) $(ARCH)'
#			$(RM) -f $@
#			$(LD) $^ -r -o $@
#			$(MD5SUM) $@ > $@.md5 
#			$(SHA1SUM) $@ > $@.sha1			



$(OUTDIR)/lib_a/$(INT_LIB_A):	$(addprefix $(OUTDIR)/lib_a/,$(notdir $(OBJS)))
			@$(ECHO) ' ### Incremental Library $(@F) $(ARCH)'
			$(CC) $^ -archive -o $@ 
			
$(OUTDIR)/lib_b/$(INT_LIB_B):	$(addprefix $(OUTDIR)/lib_b/,$(notdir $(OBJS)))
			@$(ECHO) ' ### Incremental Library $(@F) $(ARCH)'
			$(CC) $^ -archive -o $@
			
outdir:
ifneq ($(TARGET_OS), INTEGRITY)
	@$(MD) $(OUTDIR) 
else
	@$(MD) $(OUTDIR)/lib_a 
	@$(MD) $(OUTDIR)/lib_b
endif  

# Common lint for the whole system
lint:   loutdir $(LINT_OUTDIR)/final.lint

loutdir:
	@$(MD) $(LINT_OUTDIR)

$(LINT_OUTDIR)/final.lint: $(addprefix $(LINT_OUTDIR)/,$(notdir $(LOBJS)))
	@$(ECHO) ' ### Lint Final'
	@$(ECHO) '### Final Lint Stage - Verifying inter module / system wide stuff' > $@
	$(FLINT) $(LINTFLAGS) $(SILENCE_LINT) -zero  $^ 1>>$@ 2>>$@ 
      
clean:
	$(RM) output


#########################################################################
help:
	@echo " " >&2
	@echo "BUILD ENVIRONMENT FOR SDTv2" >&2
	@echo "Load one of the configurations below with 'make <configuration>' first:" >&2 
	@echo "  " >&2
	@echo "  * GBE_HMI_WRL2_config          - Building for WR Linux 2 on HMIs 410-500" >&2
	@echo "  * GBE_HMI_WRL5_config          - Building for WR Linux 5 on HMIs 410-500" >&2
	@echo "  * GBE_VXWORKS_HMI_config       - Building for VXWORKS on HMI411" >&2
	@echo "  * GBE_CCUO_config              - Building for CSS3/VXWORKS on PowerPC" >&2
	@echo "  * GBE_DCU2_config              - Building for CSS3/VXWORKS on PowerPC" >&2    
	@echo "  * GBE_TCNGWS_config            - Building for ARM TI CPU" >&2
	@echo "  * GBE_CCUS_config              - Building for INTEGRITY on CCU-S" >&2
	@echo " " >&2
	@echo "Then call 'make' or 'make all' to build everything." >&2
	@echo "To build debug binaries, append 'DEBUG=TRUE' to the make command " >&2
	@echo "To see the full output, append 'VERBOSE=1' to the make command " >&2
	@echo " " >&2
	@echo "The pre-GBE legacy configurations are retaind within the attic subfolder" >&2
	@echo " " >&2	
	@echo "Other builds:" >&2
	@echo "  * make lib       - build only the libraries" >&2
	@echo "  * make lint      - build LINT analysis files" >&2	
	@echo "  * make clean     - remove all binaries and objects of the current target" >&2
	@echo "  * make unconfig  - remove the configuration file" >&2
	@echo "  * make distclean - make clean unconfig" >&2
	@echo " " >&2

unconfig:
	-$(RM) config/config.mk

%_config: config/%_cfg
ifeq ($(OS), Windows_NT)
	$(CP) config\$(@:config=cfg) config\config.mk
else
	$(CP) config/$(@:config=cfg) config/config.mk
endif

distclean:	clean unconfig

#########################################################################

