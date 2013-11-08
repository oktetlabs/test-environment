
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@ 

include_HEADERS = ipc_server.h ipc_client.h
noinst_HEADERS = ipc_internal.h


lib_LTLIBRARIES = libipc.la libipcserver.la

libipcserver_la_SOURCES = server.c portmap_server.c portmap_common.c \
                          ipc_common.c

libipc_la_SOURCES = client.c portmap_common.c ipc_common.c


install-exec-hook:
	for i in $(lib_LTLIBRARIES) ; do \
		j=`echo $${i} | sed -e 's/.la//'`; \
		touch -c -r $$i $(DESTDIR)/$(libdir)/$${j/.la/}.* ; \
	done

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
