
AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include -I$(top_srcdir)
AM_LDFLAGS=@TE_LDFLAGS@ 

noinst_HEADERS=tad_pcap_impl.h

noinst_LIBRARIES=libtad_pcap.a

libtad_eth_a_SOURCES=tad_pcap_stack.c tad_pcap_layer.c tad_pcap_csap.c

install-exec-hook:
	touch -c -r libtad_eth.a $(DESTDIR)/$(prefix)/lib/libtad_pcap.a

all: install

