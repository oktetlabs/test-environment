
AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


lib_LIBRARIES=libtapi_eth.a 

include_HEADERS=tapi_eth.h
libtapi_eth_a_SOURCES=tapi_eth.c

install-exec-hook:
	touch -c -r libtapi_eth.a $(DESTDIR)/$(prefix)/lib/libtapi_eth.a

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

