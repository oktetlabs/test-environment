
AM_CPPFLAGS = @TE_CPPFLAGS@ @TAD_CH_DEFS@ -I$(DESTDIR)/$(includedir) \
              -I$(top_srcdir)
AM_CFLAGS = @TE_CFLAGS@ 


noinst_HEADERS=tad_ipstack_impl.h

noinst_LIBRARIES=libtad_ipstack.a

libtad_ipstack_a_SOURCES=tad_ipstack_csaps.c \
                         tad_ipv4_layer.c \
                         tad_ipv4_stack.c \
                         tad_icmpv4_layer.c \
                         tad_icmpv4_stack.c \
                         tad_icmp_methods.c \
                         tad_udp_layer.c \
                         tad_udp_stack.c \
                         tad_tcp_layer.c \
                         tad_tcp_stack.c


install-exec-hook:
	touch -c -r libtad_ipstack.a $(DESTDIR)/$(libdir)/libtad_ipstack.a

all: install

