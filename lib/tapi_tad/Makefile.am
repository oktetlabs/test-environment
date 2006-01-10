
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@ 

include_HEADERS = tapi_tad.h tapi_ndn.h

lib_LTLIBRARIES = libtapi_tad.la

libtapi_tad_la_SOURCES = tapi_tad.c tapi_ndn.c


install-exec-hook:
	touch -c -r .libs/libtapi_tad.la \
		$(DESTDIR)/$(libdir)/libtapi_tad.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
