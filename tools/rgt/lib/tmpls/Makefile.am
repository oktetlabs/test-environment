#
# automake file for Report Generator template library
# Process it with automake
#
# Copyright (C) 2003-2018 OKTET Labs.
#
# Author: Oleg N. Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
#

SUBDIRS = .

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/@includedir@ @EXTRA_CPPFLAGS@ \
              $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

noinst_LIBRARIES = libtmpls.a
libtmpls_a_SOURCES = rgt_tmpls_lib.c rgt_tmpls_lib.h
