
AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) \
              $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

noinst_HEADERS = tad_geneve_impl.h

noinst_LIBRARIES = libtad_geneve.a

libtad_geneve_a_SOURCES = \
	tad_geneve_layer.c \
	tad_geneve_csap.c
