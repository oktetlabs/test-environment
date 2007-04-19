

ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 

include_HEADERS = tce_api.h
lib_LTLIBRARIES = libtce.la

libtce_la_SOURCES = tce_generic.c tce_internal.h tce33.c tce34.c

install-exec-hook:
	touch -c -r libtce.la $(DESTDIR)/$(libdir)/libtce.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done


all: install
