
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 

include_HEADERS = \
	tapi_ip.h \
	tapi_icmp4.h \
	tapi_tcp.h \
	tapi_udp.h \
	tapi_socket.h

lib_LTLIBRARIES = libtapi_ipstack.la

libtapi_ipstack_la_SOURCES = \
	tapi_ip4.c \
	tapi_icmp4.c \
	tapi_udp.c \
	tapi_tcp.c \
	tapi_tcp_conn.c \
	tapi_socket.c


install-exec-hook:
	touch -c -r libtapi_ipstack.la $(DESTDIR)/$(libdir)/libtapi_ipstack.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
