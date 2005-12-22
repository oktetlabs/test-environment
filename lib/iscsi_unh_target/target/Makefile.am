
AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) \
              -I$(srcdir)/../common -I$(srcdir)/../security
AM_CFLAGS = @TE_CFLAGS@ 


noinst_HEADERS = \
	my_login.h \
	iscsi_target.h \
	scsi_target.h \
	iscsi_portal_group.h \
	target_error_rec.h 

include_HEADERS = iscsi_target_api.h

noinst_LIBRARIES = libiscsi_target.a 


libiscsi_target_a_SOURCES = \
	my_login.c \
	scsi_target.c \
	iscsi_portal_group.c \
	target_error_rec.c \
	customize.c
