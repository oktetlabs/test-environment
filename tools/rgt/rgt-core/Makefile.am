#
# automake file for Report Generator Core directory
# Process it with automake
#
# Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
# root directory of the distribution).
#
# Test Environment is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# Test Environment is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA  02111-1307  USA
#
#
# Author: Oleg N. Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
#

SUBDIRS = .

bin_PROGRAMS = rgt-core

rgt_core_SOURCES = filter.c filter.h \
                   flow_tree.c flow_tree.h \
                   log_msg.c log_msg.h \
                   log_format.c log_format.h \
                   log_format_v1.c \
                   io.c io.h \
                   memory.c memory.h \
                   live_mode.c live_mode.h \
                   postponed_mode.c postponed_mode.h \
                   rgt_core.c rgt_common.h

rgt_core_LDADD = @GLIB_LIBS@ -lpopt @TCL_LIB_SPEC@ -lm -ldl
AM_CPPFLAGS = @GLIB_CFLAGS@ -I$(top_srcdir) @TCL_CFLAGS@

AM_CFLAGS = -W -Wall
bin_SCRIPTS = rgt-filter-prepare rgt-conv

dist_data_DATA = rgt-filter-xml2tcl.xsl

EXTRA_DIST=rgt-conv.in rgt-filter-prepare.in
