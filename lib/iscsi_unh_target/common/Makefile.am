
AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) 
AM_CFLAGS = @TE_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@

pkginclude_HEADERS = \
	target_negotiate.h \
	list.h \
	debug.h \
	iscsi_common.h \
	text_param.h \
	iscsi_custom.h

noinst_HEADERS = \
	range.h \
	crc.h \
	tcp_utilities.h \
	lun_packing.h \
	scsi_request.h \
	linux-scsi.h \
	my_memory.h \
	scsi_cmnd.h \
    mutex.h


noinst_LIBRARIES = libiscsi_common.a

libiscsi_common_a_SOURCES = \
	range.c \
	debug.c \
	crc.c \
	tcp_utilities.c \
	lun_packing.c \
	text_param.c \
	default_param.c \
	target_negotiate.c \
	iscsi_common.c \
    shmem.c \
    mutex.c   

install-data-hook:
	for i in $(pkginclude_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(pkgincludedir)/$$i ; \
	done
