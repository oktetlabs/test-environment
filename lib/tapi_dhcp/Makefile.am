
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 


lib_LTLIBRARIES=libtapi_dhcp.la

include_HEADERS=tapi_dhcp.h
libtapi_dhcp_la_SOURCES=tapi_dhcp.c

install-exec-hook:
	touch -c -r .libs/libtapi_dhcp.la $(DESTDIR)/$(libdir)/libtapi_dhcp.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install

