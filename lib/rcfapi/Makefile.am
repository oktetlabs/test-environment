
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@ 

include_HEADERS=rcf_api.h 

lib_LTLIBRARIES=librcfapi.la

librcfapi_la_SOURCES=rcf_api.c


install-exec-hook:
	touch -c -r .libs/librcfapi.la $(DESTDIR)/$(libdir)/librcfapi.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install

