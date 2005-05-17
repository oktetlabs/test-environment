
AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include
AM_LDFLAGS=@TE_LDFLAGS@ 

lib_LTLIBRARIES=libtapi_bridge.la

include_HEADERS=tapi_stp.h

libtapi_bridge_la_SOURCES=tapi_stp.c


install-exec-hook:
	touch -c -r .libs/libtapi_bridge.la \
		$(DESTDIR)/$(prefix)/lib/libtapi_bridge.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

