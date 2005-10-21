# $Id$

@SET_MAKE@

srcdir=@srcdir@
VPATH=@srcdir@

AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include -I$(top_srcdir) \
			-I$(srcdir)/../..

AM_LDFLAGS=@TE_LDFLAGS@ 

include_HEADERS=target_negotiate.h list.h \
                iscsi_common.h text_param.h \
                iscsi_custom.h

noinst_HEADERS=debug.h range.h \
               crc.h \
               tcp_utilities.h lun_packing.h scsi_request.h \
               linux-scsi.h my_memory.h scsi_cmnd.h

noinst_LIBRARIES=libiscsi_common.a

libiscsi_common_a_SOURCES = range.c debug.c crc.c tcp_utilities.c \
	                        lun_packing.c text_param.c default_param.c \
	                        target_negotiate.c iscsi_common.c


install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done
