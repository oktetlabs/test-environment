
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include \
              -I$(DESTDIR)/$(prefix)/../include
AM_CFLAGS = 

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

include_HEADERS=rcf_api.h 

lib_LIBRARIES=librcfapi.a

librcfapi_a_SOURCES=rcf_api.c


install-exec-hook:
	touch -c -r librcfapi.a $(DESTDIR)/$(prefix)/lib/librcfapi.a

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

