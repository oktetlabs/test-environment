
AM_CFLAGS=-I$(DESTDIR)/$(prefix)/include \
          -I$(DESTDIR)/$(prefix)/../include

AM_CFLAGS=

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


include_HEADERS=tapi_ipstack.h

lib_LTLIBRARIES=libtapi_ipstack.la

libtapi_ipstack_la_SOURCES=tapi_ipstack.c


install-exec-hook:
	touch -c -r .libs/libtapi_ipstack.la \
		$(DESTDIR)/$(prefix)/lib/libtapi_ipstack.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
