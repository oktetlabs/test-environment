
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 

include_HEADERS=tapi_iscsi.h

lib_LTLIBRARIES=libtapi_iscsi.la

libtapi_iscsi_la_SOURCES=tapi_iscsi.c


install-exec-hook:
	touch -c -r .libs/libtapi_iscsi.la \
		$(DESTDIR)/$(libdir)/libtapi_iscsi.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
