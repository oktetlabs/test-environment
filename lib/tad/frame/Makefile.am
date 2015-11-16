
AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) \
              $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

noinst_HEADERS = tad_frame_impl.h

noinst_LIBRARIES = libtad_frame.a

libtad_frame_a_SOURCES = tad_frame_csap.c tad_frame_layer.c
