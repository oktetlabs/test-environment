
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include \
              -I$(srcdir)/../common -I$(srcdir)/../security


noinst_HEADERS = \
	my_login.h \
	iscsi_target.h \
	scsi_target.h \
	iscsi_portal_group.h \
	target_error_rec.h 

noinst_LIBRARIES = libiscsi_target.a 


libiscsi_target_a_SOURCES = \
	my_login.c \
	scsi_target.c \
	iscsi_portal_group.c \
	target_error_rec.c \
	customize.c
