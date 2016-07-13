
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) \
			  $(TE_CPPFLAGS_VAR) @EXTRA_CPPFLAGS@
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

include_HEADERS=rpc_transport.h

lib_LTLIBRARIES=librpctransport.la

librpctransport_la_SOURCES=posix.c win32.c

install-exec-hook:
	touch -c -r librpctransport.la $(DESTDIR)/$(libdir)/librpctransport.*


install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install


