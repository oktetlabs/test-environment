
AM_CPPFLAGS = -I$(DESTDIR)/$(includedir) -I$(top_srcdir)

noinst_HEADERS = tad_arp_impl.h

noinst_LIBRARIES = libtad_arp.a

libtad_arp_a_SOURCES = tad_arp_stack.c tad_arp_layer.c tad_arp_csap.c


install-exec-hook:
	touch -c -r libtad_arp.a $(DESTDIR)/$(libdir)/libtad_arp.a

all: install
