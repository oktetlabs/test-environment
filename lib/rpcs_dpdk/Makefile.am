
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = -I$(DESTDIR)/$(includedir) \
              -I$(TE_BASE)/agents/@TA@/ \
              -I$(TE_BASE)/agents/@TA@/rpc \
              @TE_CPPFLAGS@ \
              $(TE_CPPFLAGS_VAR) \
              -D'INCLUDE(x)='

AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)


EXTRA_DIST = dpdk.x.m4

noinst_HEADERS = \
	rpcs_dpdk.h

lib_LIBRARIES = librpcs_dpdk.a

librpcs_dpdk_a_SOURCES = \
	mbuf.c \
	ethdev.c \
	ring.c \
	mempool.c \
	rte_mbuf_ndn.c \
	eal.c


install-exec-hook:
	for i in ${lib_LIBRARIES} ; do \
	    touch -c -r $$i $(DESTDIR)/$(prefix)/lib/$$i ; \
	done

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
