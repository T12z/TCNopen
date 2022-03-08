#
# TRDP Spy
#
# Copyright (c) University of Rostock 2020 (this file, see indiviual files for (c))
#
# Authors:
#  Thorsten Schulz <thorsten.schulz@uni-rostock.de>
#
# This work is licensed under the terms of the MPL, version 2.  See
# the LICENSE file in the top-level directory.
# 
# SPDX-License-Identifier: MPL-2.0
#

PKGCNF_EXISTS = $(shell pkg-config wireshark --cflags)
IFLAGS    = $(shell pkg-config wireshark --cflags-only-I)
CFLAGS   += $(shell pkg-config wireshark --cflags-only-other) -fPIC -O2
LDFLAGS  += $(shell pkg-config wireshark --libs-only-other)
LIBS     += $(shell pkg-config wireshark --libs-only-l)
PLUGINDIR = $(shell pkg-config wireshark --variable=plugindir)
INSTALL   = install
WSVER     = $(shell basename $(PLUGINDIR))

CPPFLAGS += -I. $(IFLAGS)
INSTALL  += -D -t

# Just running the ifeq already makes the Make fail, so the error-echo is partially redundant
ifeq ($(PKGCNF_EXISTS),)
	$(error "pkg-config\'s output for wireshark is empty. Have you installed all build-deps?")
endif


HFILES = $(addsuffix /cfile.h,$(subst -I,,$(IFLAGS)))
CFILE_EXISTS = $(strip $(foreach hdr,$(HFILES),$(file <$(hdr))))

CPPFLAGS += -DPLUGIN_VERSION=\"2.2.1\"
CPPFLAGS += -Wall -W -Wextra -Wendif-labels -Wpointer-arith -Warray-bounds -Wformat-security -fwrapv -fno-strict-overflow -fno-delete-null-pointer-checks -Wvla -Waddress -Wattributes -Wdiv-by-zero -Wignored-qualifiers -Wpragmas -Wno-overlength-strings -Wwrite-strings -Wno-long-long -fexcess-precision=fast -Wc++-compat -Wshadow -Wno-pointer-sign -Wold-style-definition -Wstrict-prototypes -Wlogical-op -Wjump-misses-init -fvisibility=hidden
LDFLAGS  += -Wl,--as-needed -pie -Wl,-soname,trdp_spy.so

# There is probably a way to automate checking for that header and going for the fall-back pull

ifeq ($(CFILE_EXISTS),)
packet-trdp_spy.c: info-missing-h cfile.h

endif

all: trdp_spy.so

info-missing-h:
	$(info === )
	$(info If compiling 'packet-trdp_spy.c' fails due to a missing <cfile.h>, which was not packaged in Debian until late 3.2, I will try to pull the file from the web. However, I have to select the right version. Considering, current packages contain the file, I only have to watch old-stable 2.6. If the compilation chokes, check if you have a different version and fix it manually.)
	$(info === )

%.c: %.h

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

trdp_spy.so: trdpDict.o trdp_env.o packet-trdp_spy.o plugin.o
	$(CC) $(CPPFLAGS) $(LDFLAGS) -shared -o "$@" $^ $(LIBS)
	
cfile.h:
	wget https://raw.githubusercontent.com/wireshark/wireshark/master-2.6/cfile.h
	
clean: doc-clean
	$(RM) trdpDict.o trdp_env.o packet-trdp_spy.o plugin.o

distclean: clean
	$(RM) trdp_spy.so cfile.h

install: trdp_spy.so
	$(INSTALL) $(DESTDIR)$(PLUGINDIR)/epan trdp_spy.so

plugin: trdp_spy.so
	$(INSTALL) ../../plugins/$(WSVER)/epan trdp_spy.so

install-html: doc-html
	$(INSTALL) $(DESTDIR)/html-ref doc/html/*.html doc/html/*.png doc/html/*.js doc/html/*.map doc/html/*.css

doc/latex/Makefile doc/html/index.html: *.c *.h
	doxygen Doxyfile
	
doc-pdf: doc/latex/Makefile
	make -C doc/latex
	
doc-html: doc/html/index.html

doc-clean:
	$(RM) -r doc/html doc/latex

	

