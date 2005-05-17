
AM_CPPFLAGS = @TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include
AM_LDFLAGS = @TE_LDFLAGS@ 

include_HEADERS = tapi_tad.h

lib_LTLIBRARIES = libtapi_tad.la

libtapi_tad_la_SOURCES = tapi_tad.c


install-exec-hook:
	touch -c -r .libs/libtapi_tad.la \
		$(DESTDIR)/$(prefix)/lib/libtapi_tad.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
