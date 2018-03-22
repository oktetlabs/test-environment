#
# automake file for RGT raw2xml/lib directory
# Process with automake
#
# Copyright (C) 2003-2018 OKTET Labs.
#
# Author: Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
#

AM_CPPFLAGS = @TE_CPPFLAGS@ @EXTRA_CPPFLAGS@ \
              -D_FILE_OFFSET_BITS=64 -DNDEBUG $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ -I $(srcdir)/../include -Wextra -Werror \
            $(TE_CFLAGS_VAR)

lib_luadir = $(libdir)/lua

lib_LTLIBRARIES = librgt-raw2xml.la

librgt_raw2xml_la_SOURCES = \
    lua_rgt_co.c            \
    lua_rgt_co_chunk.c      \
    lua_rgt_co_mngr.c       \
    lua_rgt_msg.c           \
    rgt_cbuf.c              \
    rgt_co.c                \
    rgt_co_strg.c           \
    rgt_msg.c               \
    rgt_msg_fmt.c

install-exec-hook:
	@INSTALL@ -d $(lib_luadir)
	@LN_S@ -f ../librgt-raw2xml.so.0.0.0 $(lib_luadir)/rgt.so
