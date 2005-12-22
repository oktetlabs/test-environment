
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 


include_HEADERS=tapi_pcap.h

lib_LTLIBRARIES=libtapi_pcap.la 

libtapi_pcap_la_SOURCES=tapi_pcap.c


install-exec-hook:
	touch -c -r .libs/libtapi_pcap.la $(DESTDIR)/$(libdir)/libtapi_pcap.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install

