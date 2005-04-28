# $Id$

AUTOMAKE_OPTIONS = foreign
@SET_MAKE@

srcdir=@srcdir@
VPATH=@srcdir@

AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include 
AM_CFLAGS = 
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

bin_PROGRAMS = log2da

log2da_SOURCES = log2da.c gcov-io.h

EXTRA_DIST = bbhook.c 


