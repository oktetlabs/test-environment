
AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include
AM_LDFLAGS=@TE_LDFLAGS@ 


include_HEADERS=tapi_radius.h

lib_LTLIBRARIES=libtapi_radius.la 

libtapi_radius_la_SOURCES=tapi_radius.c


install-exec-hook:
	touch -c -r .libs/libtapi_radius.la $(DESTDIR)/$(prefix)/lib/libtapi_radius.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

