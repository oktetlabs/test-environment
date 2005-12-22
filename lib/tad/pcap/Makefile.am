
AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) -I$(top_srcdir)
AM_CFLAGS = @TE_CFLAGS@ 

noinst_HEADERS=tad_pcap_impl.h

noinst_LIBRARIES=libtad_pcap.a

libtad_pcap_a_SOURCES=tad_pcap_stack.c tad_pcap_layer.c tad_pcap_csap.c

install-exec-hook:
	touch -c -r libtad_pcap.a $(DESTDIR)/$(libdir)/libtad_pcap.a

all: install

