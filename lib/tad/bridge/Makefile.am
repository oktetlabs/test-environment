
AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) \
              $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)


noinst_HEADERS = tad_bridge_impl.h

noinst_LIBRARIES = libtad_bridge.a

libtad_bridge_a_SOURCES = tad_bridge_csap.c tad_bridge_layer.c
