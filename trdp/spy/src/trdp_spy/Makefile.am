# Makefile.am
#
# Wireshark - Network traffic analyzer
# By Gerald Combs <gerald@wireshark.org>
# Copyright 1998 Gerald Combs
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

include $(top_srcdir)/Makefile.am.inc
include ../Makefile.am.inc

# the name of the plugin
PLUGIN_NAME = trdp_spy

# Non-generated sources to be scanned for registration routines
NONGENERATED_REGISTER_C_FILES = \
	lookuptype.c \
	packet-trdp_spy.c \
	parsebody.c \
	trdp_env.c

# Non-generated sources
NONGENERATED_C_FILES = \
	$(NONGENERATED_REGISTER_C_FILES)

# Headers.
CLEAN_HEADER_FILES = \
	lookuptype.h \
	packet-trdp_spy.h \
	parsebody.h \
	trdp_env.h

HEADER_FILES = \
	$(CLEAN_HEADER_FILES)

plugin_LTLIBRARIES = trdp_spy.la

plugin.c: $(NONGENERATED_REGISTER_C_FILES) $(top_srcdir)/tools/make-dissector-reg \
    $(top_srcdir)/tools/make-dissector-reg.py
   @if test -n "$(PYTHON)"; then \
     echo Making plugin.c with python ; \
     $(PYTHON) $(top_srcdir)/tools/make-dissector-reg.py $(srcdir) \
         plugin $(NONGENERATED_REGISTER_C_FILES) ; \
   else \
      echo Making plugin.c with shell script ; \
      $(top_srcdir)/tools/make-dissector-reg $(srcdir) \
          $(plugin_src) plugin $(NONGENERATED_REGISTER_C_FILES) ; \
   fi

trdp_spy_la_SOURCES = \
	plugin.c \
	moduleinfo.h \
	$(SRC_FILES)	\
	$(HEADER_FILES)

trdp_spy_la_CPPFLAGS = $(AM_CPPFLAGS) $(PLUGIN_CPPFLAGS)

trdp_spy_la_CFLAGS = $(AM_CFLAGS) $(PLUGIN_CFLAGS)

trdp_spy_la_LDFLAGS = $(PLUGIN_LDFLAGS)

CLEANFILES = \
	trdp_spy \
	*~

MAINTAINERCLEANFILES = \
	Makefile.in	\
	plugin.c

EXTRA_DIST = \
	plugin.rc.in		\
	CMakeLists.txt
