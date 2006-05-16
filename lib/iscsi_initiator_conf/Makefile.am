
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) 
AM_CFLAGS = @TE_CFLAGS@ -pthread
AM_LDFLAGS = @TE_LDFLAGS@


# noinst_PROGRAMS = unh_targetd


# unh_targetd_SOURCES = main.c

# unh_targetd_LDADD = target/libiscsi_target.a security/libiscsi_security.a \
#                    common/libiscsi_common.a


lib_LIBRARIES = libiscsi_initiator_conf.a

noinst_HEADERS = iscsi_initiator.h

libiscsi_initiator_conf_a_SOURCES = conf_iscsi_initiator.c win32.c 

all: install
