#
# automake file for Report Generator Core directory
# Process it with automake
#
# Copyright (C) 2003-2018 OKTET Labs.
#
# Author: Oleg N. Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
#

SUBDIRS = .

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(top_srcdir) @EXTRA_CPPFLAGS@ \
              $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ @GLIB_CFLAGS@ @TCL_CFLAGS@ $(TE_CFLAGS_VAR)

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
                   index_mode.c index_mode.h \
                   junit_mode.c junit_mode.h \
                   rgt_core.c rgt_common.h

rgt_core_LDADD = @GLIB_LIBS@ -lpopt @TCL_LIB_SPEC@ -lm -ldl

bin_SCRIPTS = rgt-conv

EXTRA_DIST = rgt-conv.in

CLEANFILES = rgt-conv
