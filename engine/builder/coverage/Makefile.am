# $Id$

AUTOMAKE_OPTIONS = foreign
@SET_MAKE@

srcdir=@srcdir@
VPATH=@srcdir@

AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include 
AM_CFLAGS = 
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

bin_PROGRAMS = tce_dump

tce_dump_SOURCES = tce_dump.c
tce_dump_LDADD = -llogger_ten -lrcfapi -lrpcxdr -lipc @TE_LDADD@ -lpthread -lexpat
tce_dump_DEPENDENCIES=@TE_DEPENDENCIES@ \
                    $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                    $(DESTDIR)/$(prefix)/lib/libipc.a



nobase_dist_data_DATA = bbhook.c modules/Makefile-2.6 modules/bbinit.c



