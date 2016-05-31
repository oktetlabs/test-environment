
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)
AM_LDFLAGS = -L$(DESTDIR)/$(libdir)

noinst_HEADERS = \
	rpcc_dpdk.h

include_HEADERS = \
	tapi_rpc_rte_eal.h \
	tapi_rpc_rte.h

lib_LTLIBRARIES = librpcc_dpdk.la

librpcc_dpdk_la_SOURCES = \
	eal.c


install-exec-hook:
	touch -c -r librpcc_dpdk.la $(DESTDIR)/$(libdir)/librpcc_dpdk.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
