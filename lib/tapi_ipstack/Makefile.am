
AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include
AM_LDFLAGS=@TE_LDFLAGS@ 

include_HEADERS=tapi_ip.h tapi_tcp.h tapi_udp.h

lib_LTLIBRARIES=libtapi_ipstack.la

libtapi_ipstack_la_SOURCES=tapi_ip.c tapi_tcp.c tapi_udp.c tapi_tcp_conn.c


install-exec-hook:
	touch -c -r .libs/libtapi_ipstack.la \
		$(DESTDIR)/$(prefix)/lib/libtapi_ipstack.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
