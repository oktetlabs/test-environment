
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include \
            -I$(DESTDIR)/$(prefix)/../include 

AM_CFLAGS=

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


noinst_HEADERS=tad_ipstack_impl.h

noinst_LIBRARIES=libtad_ipstack.a

libtad_ipstack_a_SOURCES=tad_ipstack_csaps.c \
                         tad_ipv4_layer.c \
                         tad_ipv4_stack.c \
                         tad_icmpv4_layer.c \
                         tad_icmpv4_stack.c \
                         tad_udp_layer.c \
                         tad_udp_stack.c 


install-exec-hook:
	touch -c -r libtad_ipstack.a $(DESTDIR)/$(prefix)/lib/libtad_ipstack.a

all: install

