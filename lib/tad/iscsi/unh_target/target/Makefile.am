# $Id$

AUTOMAKE_OPTIONS = foreign
@SET_MAKE@

srcdir=@srcdir@
VPATH=@srcdir@

AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include -I$(top_srcdir) \
		    -I$(srcdir)/../common \
		    -I$(srcdir)/../security \
		    -I$(srcdir) -I$(srcdir)/../..

noinst_HEADERS=my_login.h iscsi_target.h \
	           scsi_target.h \
	           iscsi_portal_group.h \
			   target_error_rec.h 

noinst_LIBRARIES = libiscsi_target.a 

libiscsi_target_a_SOURCES = my_login.c     \
							scsi_target.c  \
							iscsi_portal_group.c \
							target_error_rec.c \
							customize.c

