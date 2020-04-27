
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)

AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

AM_LDFLAGS = -L$(DESTDIR)/$(libdir)

include_HEADERS= \
	tapi_dpdk.h \
	tapi_dpdk_stats.h \
	tapi_l2fwd.h \
	tapi_rte_flow.h

lib_LTLIBRARIES = libtapi_dpdk.la

libtapi_dpdk_la_SOURCES = \
	tapi_dpdk.c \
	tapi_dpdk_stats.c \
	tapi_l2fwd.c \
	tapi_rte_flow.c

install-exec-hook:
	for i in ${lib_LTLIBRARIES} ; do \
	    touch -c -r $$i $(DESTDIR)/$(prefix)/lib/$$i ; \
	done

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
