
AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include 
AM_LDFLAGS=@TE_LDFLAGS@ 


include_HEADERS=tapi_cli.h

lib_LTLIBRARIES=libtapi_cli.la 

libtapi_cli_la_SOURCES=tapi_cli.c


install-exec-hook:
	touch -c -r .libs/libtapi_cli.la $(DESTDIR)/$(prefix)/lib/libtapi_cli.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

