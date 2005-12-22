
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 


include_HEADERS=tapi_arp.h

lib_LTLIBRARIES=libtapi_arp.la 

libtapi_arp_la_SOURCES=tapi_arp.c


install-exec-hook:
	touch -c -r .libs/libtapi_arp.la $(DESTDIR)/$(libdir)/libtapi_arp.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install

