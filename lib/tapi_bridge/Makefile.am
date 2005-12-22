
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 

lib_LTLIBRARIES=libtapi_bridge.la

include_HEADERS=tapi_stp.h

libtapi_bridge_la_SOURCES=tapi_stp.c


install-exec-hook:
	touch -c -r .libs/libtapi_bridge.la \
		$(DESTDIR)/$(libdir)/libtapi_bridge.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install

