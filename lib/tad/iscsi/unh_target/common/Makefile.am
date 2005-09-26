# $Id$

@SET_MAKE@

srcdir=@srcdir@
VPATH=@srcdir@

AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include -I$(top_srcdir) 

AM_LDFLAGS=@TE_LDFLAGS@ 

noinst_HEADERS=list.h iscsi_common.h debug.h range.h \
               crc.h text_param.h target_negotiate.h \
               tcp_utilities.h lun_packing.h scsi_request.h \
               my_memory.h

noinst_LIBRARIES=libiscsi_common.a

libiscsi_common_a_SOURCES = range.c debug.c crc.c tcp_utilities.c \
	                        lun_packing.c text_param.c default_param.c \
	                        target_negotiate.c iscsi_common.c

