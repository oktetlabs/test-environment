
AM_CPPFLAGS = -I$(DESTDIR)$(prefix)/include \
              -I$(DESTDIR)$(prefix)/../include

# TE_CFLAGS are already included in CFLAGS in configure.ac
AM_CFLAGS = 

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


include_HEADERS = ipc_server.h ipc_client.h
noinst_HEADERS = ipc_internal.h


lib_LTLIBRARIES = libipc.la libipcserver.la

libipcserver_la_SOURCES = server.c portmap_server.c portmap_common.c \
                          ipc_common.c

libipc_la_SOURCES = client.c portmap_common.c ipc_common.c


install-exec-hook:
	touch -c -r .libs/libipc.la $(DESTDIR)/$(prefix)/lib/libipc.*
	touch -c -r .libs/libipcserver.la $(DESTDIR)/$(prefix)/lib/libipcserver.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
