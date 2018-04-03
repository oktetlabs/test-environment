#
# automake file for RGT dump directory
# Process with automake
#
# Copyright (C) 2003-2018 OKTET Labs.
#
# Author: Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
#

AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ @EXTRA_CPPFLAGS@ \
              -D_FILE_OFFSET_BITS=64 $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ -Wextra $(TE_CFLAGS_VAR)
GEN_LDADD = @TE_LDFLAGS@

bin_PROGRAMS = rgt-dump

rgt_dump_SOURCES = dump.c
rgt_dump_LDADD = $(GEN_LDADD)

