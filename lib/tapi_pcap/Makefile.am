
AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include
AM_LDFLAGS=@TE_LDFLAGS@ 


include_HEADERS=tapi_pcap.h

lib_LTLIBRARIES=libtapi_pcap.la 

libtapi_pcap_la_SOURCES=tapi_pcap.c


install-exec-hook:
	touch -c -r .libs/libtapi_pcap.la $(DESTDIR)/$(prefix)/lib/libtapi_pcap.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

