
AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 

noinst_HEADERS = tad_frame_impl.h

noinst_LIBRARIES = libtad_frame.a

libtad_frame_a_SOURCES = tad_frame_csap.c tad_frame_layer.c


install-exec-hook:
	touch -c -r libtad_frame.a $(DESTDIR)/$(libdir)/libtad_frame.a

all: install
