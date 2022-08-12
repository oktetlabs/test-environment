#
# automake file for Report Generator Core directory
# Process it with automake
#
# Copyright (C) 2004-2018 OKTET Labs.
#
# Author: Oleg N. Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
#

AUTOMAKE_OPTIONS = foreign
SUBDIRS = .

bin_PROGRAMS = millicom
millicom_SOURCES = millicom.c

millicom_LDADD = -lm -ldl
AM_CPPFLAGS = -I$(top_srcdir) $(TE_CPPFLAGS_VAR)

AM_CFLAGS = -W -Wall -ggdb -O0 $(TE_CFLAGS_VAR)
