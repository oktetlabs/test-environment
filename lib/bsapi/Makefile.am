
AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include
AM_LDFLAGS=@TE_LDFLAGS@ 


include_HEADERS=te_builder_nuts.h te_builder_ts.h

lib_LTLIBRARIES=libbsapi.la

libbsapi_la_SOURCES=bsapi.c


install-exec-hook:
	touch -c -r libbsapi.la $(DESTDIR)/$(prefix)/lib/libbsapi.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
