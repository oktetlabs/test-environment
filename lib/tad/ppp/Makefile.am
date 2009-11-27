
AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 

noinst_HEADERS = tad_ppp_impl.h

noinst_LIBRARIES = libtad_ppp.a

libtad_ppp_a_SOURCES = tad_ppp_layer.c tad_ppp_csap.c tad_pppoe_layer.c
