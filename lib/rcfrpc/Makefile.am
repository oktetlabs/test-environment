
AM_CPPFLAGS = @TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include
AM_LDFLAGS = @TE_LDFLAGS@ 


include_HEADERS=rcf_rpc.h

lib_LTLIBRARIES=librcfrpc.la

librcfrpc_la_SOURCES=rcf_rpc.c 

install-exec-hook:
	touch -c -r .libs/librcfrpc.la $(DESTDIR)/$(prefix)/lib/librcfrpc.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

