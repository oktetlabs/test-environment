
AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include -I$(top_srcdir)
AM_LDFLAGS=@TE_LDFLAGS@ 

noinst_HEADERS=tad_eth_impl.h

noinst_LIBRARIES=libtad_eth.a

libtad_eth_a_SOURCES=tad_eth_stack.c tad_eth_layer.c tad_eth_csap.c \
                     tad_eth_l3.c

install-exec-hook:
	touch -c -r libtad_eth.a $(DESTDIR)/$(prefix)/lib/libtad_eth.a

all: install

