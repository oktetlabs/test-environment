
AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) \
              $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

noinst_HEADERS = tad_rte_mbuf_impl.h

noinst_LIBRARIES = libtad_rte_mbuf.a

libtad_rte_mbuf_a_SOURCES = \
	tad_rte_mbuf_csap.c \
	tad_rte_mbuf_layer.c \
	tad_rte_mbuf_stack.c
