
AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


lib_LIBRARIES=libtapi_bridge.a 

include_HEADERS=tapi_stp.h

libtapi_bridge_a_SOURCES=tapi_stp.c

install-exec-hook:
	touch -c -r libtapi_bridge.a $(DESTDIR)/$(prefix)/lib/libtapi_bridge.a

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

