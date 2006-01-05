
AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 

noinst_HEADERS = tad_arp_impl.h

noinst_LIBRARIES = libtad_arp.a

libtad_arp_a_SOURCES = tad_arp_csap.c tad_arp_layer.c


install-exec-hook:
	touch -c -r libtad_arp.a $(DESTDIR)/$(libdir)/libtad_arp.a

all: install
