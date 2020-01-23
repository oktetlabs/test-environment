
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ $(TIRPC_CFLAGS) -I$(DESTDIR)/$(includedir) \
              $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)
AM_LDFLAGS = -L$(DESTDIR)/$(libdir)

include_HEADERS = \
	rpc_dpdk_defs.h \
	rpc_dpdk_offloads.h

lib_LTLIBRARIES = librpc_dpdk.la

librpc_dpdk_la_SOURCES = \
	rpc_dpdk_offloads.c

librpc_dpdk_la_LIBADD = $(TIRPC_LIBS)

install-exec-hook:
	touch -c -r librpc_dpdk.la $(DESTDIR)/$(libdir)/librpc_dpdk.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
