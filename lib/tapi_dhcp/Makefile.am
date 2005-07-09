
AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include 
AM_LDFLAGS=@TE_LDFLAGS@ 


lib_LTLIBRARIES=libtapi_dhcp.la

include_HEADERS=tapi_dhcp.h
libtapi_dhcp_la_SOURCES=tapi_dhcp.c

install-exec-hook:
	touch -c -r .libs/libtapi_dhcp.la $(DESTDIR)/$(prefix)/lib/libtapi_dhcp.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

