#
# automake file for RGT raw2xml/lib directory
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

AM_CPPFLAGS = @TE_CPPFLAGS@ @EXTRA_CPPFLAGS@
AM_CFLAGS = @TE_CFLAGS@ -I $(srcdir)/../include -Wextra -Werror

lib_luadir = $(libdir)/lua

lib_LTLIBRARIES = librgt-raw2xml.la

librgt_raw2xml_la_SOURCES = \
    cbuf.c                  \
    co.c                    \
    co_strg.c               \
    lua_cbuf.c              \
    lua_msg.c               \
    msg.c                   \
    msg_fmt.c

install-exec-hook:
	@INSTALL@ -d $(lib_luadir)
	@LN_S@ -f ../librgt-raw2xml.so.0.0.0 $(lib_luadir)/rgt.so
