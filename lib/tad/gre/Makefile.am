
AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) \
              $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

noinst_HEADERS = tad_gre_impl.h

noinst_LIBRARIES = libtad_gre.a

libtad_gre_a_SOURCES = \
	tad_gre_layer.c \
	tad_gre_csap.c
