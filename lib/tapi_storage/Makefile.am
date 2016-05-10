
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)

AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

AM_LDFLAGS = @TE_LDFLAGS@


lib_LTLIBRARIES=libtapi_storage.la

include_HEADERS= \
	tapi_local_file.h \
	tapi_local_fs.h \
	tapi_local_storage_device.h \
	tapi_storage_common.h \
	tapi_storage_client.h \
	tapi_storage_server.h \
	tapi_storage_client_ftp.h \
	tapi_storage.h

libtapi_storage_la_SOURCES= \
	tapi_local_file.c \
	tapi_local_fs.c \
	tapi_local_storage_device.c \
	tapi_storage_common.c \
	tapi_storage_client.c \
	tapi_storage_server.c \
	tapi_storage_client_ftp.c \
	tapi_storage.c


install-exec-hook:
	touch -c -r libtapi_storage.la $(DESTDIR)/$(libdir)/libtapi_storage.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
