
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)

AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

AM_LDFLAGS = -L$(DESTDIR)/$(libdir)

lib_LTLIBRARIES=libtapi_nvme.la

noinst_HEADERS= \
    tapi_nvme_internal.h

include_HEADERS=                \
    tapi_nvme.h                 \
    tapi_nvme_kern_target.h     \
    tapi_nvme_onvme_target.h

libtapi_nvme_la_SOURCES=        \
    tapi_nvme.c                 \
    tapi_nvme_internal.c        \
    tapi_nvme_kern_target.c     \
    tapi_nvme_onvme_target.c

install-exec-hook:
	touch -c -r libtapi_nvme.la $(DESTDIR)/$(libdir)/libtapi_nvme.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
