# $Id$

@SET_MAKE@

srcdir=@srcdir@
VPATH=@srcdir@

SUBDIRS=common security target

AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include -I$(top_srcdir) \
	    -I$(srcdir)/common -I$(srcdir)/target -I$(srcdir)/..

AM_CFLAGS=@TE_CFLAGS@ -pthread

AM_LDFLAGS=@TE_LDFLAGS@ 

noinst_PROGRAMS = unh_targetd

unh_targetd_SOURCES = main.c

unh_targetd_LDADD = target/libiscsi_target.a security/libiscsi_security.a \
                    common/libiscsi_common.a


