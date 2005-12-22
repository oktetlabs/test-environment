
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 


include_HEADERS=tapi_eth.h

lib_LTLIBRARIES=libtapi_eth.la 

libtapi_eth_la_SOURCES=tapi_eth.c


install-exec-hook:
	touch -c -r .libs/libtapi_eth.la $(DESTDIR)/$(libdir)/libtapi_eth.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install

