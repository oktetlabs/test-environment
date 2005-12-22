
AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) -I$(top_srcdir)
AM_CFLAGS = @TE_CFLAGS@ 

noinst_HEADERS=tad_eth_impl.h

noinst_LIBRARIES=libtad_eth.a

libtad_eth_a_SOURCES=tad_eth_stack.c tad_eth_layer.c tad_eth_csap.c \
                     tad_eth_l3.c

install-exec-hook:
	touch -c -r libtad_eth.a $(DESTDIR)/$(libdir)/libtad_eth.a

all: install

