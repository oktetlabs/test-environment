
AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include
AM_LDFLAGS=@TE_LDFLAGS@ 


include_HEADERS=tapi_arp.h

lib_LTLIBRARIES=libtapi_arp.la 

libtapi_arp_la_SOURCES=tapi_arp.c


install-exec-hook:
	touch -c -r .libs/libtapi_arp.la $(DESTDIR)/$(prefix)/lib/libtapi_arp.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

