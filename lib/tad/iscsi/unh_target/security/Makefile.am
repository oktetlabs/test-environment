# $Id$

@SET_MAKE@

srcdir=@srcdir@
VPATH=@srcdir@

AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include -I$(top_srcdir) \
	         -I$(srcdir)/../common

AM_LDFLAGS=@TE_LDFLAGS@ 

noinst_HEADERS=misc_func.h chap.h \
	           sha1.h md5.h \
               bigint.h srp.h


noinst_LIBRARIES=libiscsi_security.a

libiscsi_security_a_SOURCES = chap.c srp.c misc_func.c	\
                              sha1.c md5.c bigint.c


