#
# automake file for RGT XML merge directory
# Process with automake
#
# Copyright (C) 2003-2018 OKTET Labs.
#
# Author: Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
#

AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ @EXTRA_CPPFLAGS@ \
              -D_FILE_OFFSET_BITS=64 $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ @GLIB_CFLAGS@ @XML_PARSER_CFLAGS@ -Wextra \
            $(TE_CFLAGS_VAR)
GEN_LDADD = @TE_LDFLAGS@ @XML_PARSER_LIBS@ @GLIB_LIBS@

bin_PROGRAMS = rgt-xml-merge

rgt_xml_merge_SOURCES = xml-merge.c
rgt_xml_merge_LDADD = $(GEN_LDADD)

