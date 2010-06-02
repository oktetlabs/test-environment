#
# automake file for RGT raw2xml directory
# Process with automake
#
# Copyright (C) 2010 Test Environment authors (see file AUTHORS in the
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
# Author: Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
#

SUBDIRS = lua

AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ @EXTRA_CPPFLAGS@ \
              -D_FILE_OFFSET_BITS=64
AM_CFLAGS = @TE_CFLAGS@ -Wextra -Werror
GEN_LDADD = @TE_LDFLAGS@

bin_PROGRAMS = rgt-raw2xml

rgt_raw2xml_SOURCES = raw2xml.c
rgt_raw2xml_LDADD = $(GEN_LDADD) -llua

