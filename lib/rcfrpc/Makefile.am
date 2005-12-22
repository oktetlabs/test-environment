
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@ 


include_HEADERS=rcf_rpc.h

lib_LTLIBRARIES=librcfrpc.la

librcfrpc_la_SOURCES=rcf_rpc.c 

install-exec-hook:
	touch -c -r .libs/librcfrpc.la $(DESTDIR)/$(libdir)/librcfrpc.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install

