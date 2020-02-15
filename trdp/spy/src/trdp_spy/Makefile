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
CFLAGS   += $(shell pkg-config wireshark --cflags-only-other) -fPIC
LDFLAGS  += $(shell pkg-config wireshark --libs-only-other)
LIBS     += $(shell pkg-config wireshark --libs-only-l)
PLUGINDIR = $(shell pkg-config wireshark --variable=plugindir)
INSTALL   = install

CPPFLAGS += -I. $(IFLAGS)
INSTALL  += -D -t

# Just running the ifeq already makes the Make fail, so the error-echo is partially redundant
ifeq ($(PKGCNF_EXISTS),)
	$(error "pkg-config\'s output for wireshark is empty. Have you installed all build-deps?")
endif


HFILES = $(addsuffix /cfile.h,$(subst -I,,$(IFLAGS)))
CFILE_EXISTS = $(strip $(foreach hdr,$(HFILES),$(file <$(hdr))))

# There is probably a way to automate checking for that header and going for the fall-back pull

ifeq ($(CFILE_EXISTS),)
packet-trdp_spy.c: info-missing-h cfile.h

endif

info-missing-h:
	$(info === )
	$(info If compiling 'packet-trdp_spy.c' fails due to a missing <cfile.h>, which was not packaged in Debian for a while, I will try to pull the file from the web.)
	$(info === )

all: trdp_spy.so

%.c: %.h

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

trdp_spy.so: trdpDict.o trdp_env.o packet-trdp_spy.o
	$(CC) $(CPPFLAGS) $(LDFLAGS) -shared -o "$@" $^ $(LIBS)
	
cfile.h:
	wget https://raw.githubusercontent.com/wireshark/wireshark/master-3.2/cfile.h
	
clean: doc-clean
	$(RM) trdpDict.o trdp_env.o packet-trdp_spy.o

distclean: clean
	$(RM) trdp_spy.so cfile.h

install: trdp_spy.so
	$(INSTALL) $(DESTDIR)$(PLUGINDIR) trdp_spy.so

install-html: doc-html
	$(INSTALL) $(DESTDIR)/html-ref doc/html/*.html doc/html/*.png doc/html/*.js doc/html/*.map doc/html/*.css

doc/latex/Makefile doc/html/index.html: *.c *.h
	doxygen Doxyfile
	
doc-pdf: doc/latex/Makefile
	make -C doc/latex
	
doc-html: doc/html/index.html

doc-clean:
	$(RM) -r doc/html doc/latex

	

