#
# automake file for RGT XML merge directory
# Process with automake
#
# Copyright (C) 2012 Test Environment authors (see file AUTHORS in the
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
# Author: Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
#

AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ @EXTRA_CPPFLAGS@ \
              -D_FILE_OFFSET_BITS=64
AM_CFLAGS = @TE_CFLAGS@ @GLIB_CFLAGS@ @XML_PARSER_CFLAGS@ -Wextra
GEN_LDADD = @TE_LDFLAGS@ @XML_PARSER_LIBS@ @GLIB_LIBS@

bin_PROGRAMS = rgt-xml-merge

rgt_xml_merge_SOURCES = xml-merge.c
rgt_xml_merge_LDADD = $(GEN_LDADD)

