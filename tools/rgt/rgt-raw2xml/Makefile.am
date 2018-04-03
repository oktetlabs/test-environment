#
# automake file for RGT raw2xml directory
# Process with automake
#
# Copyright (C) 2003-2018 OKTET Labs.
#
# Author: Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
#

SUBDIRS = include lib lua

AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ @EXTRA_CPPFLAGS@ \
              -D_FILE_OFFSET_BITS=64 -DNDEBUG $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ -I $(srcdir)/include -Wextra -Werror \
            $(TE_CFLAGS_VAR)
GEN_LDADD = @TE_LDFLAGS@

bin_PROGRAMS = rgt-raw2xml-sorted

rgt_raw2xml_sorted_SOURCES = raw2xml-sorted.c
rgt_raw2xml_sorted_LDADD = $(GEN_LDADD) -llua

dist_bin_SCRIPTS = rgt-raw2xml
