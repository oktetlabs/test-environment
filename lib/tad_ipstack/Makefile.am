
AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include 

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

# VPATH = @srcdir@:@srcdir@/tests

lib_LIBRARIES=libtad_ipstack.a

libtad_ipstack_a_SOURCES=   tad_ipstack_csaps.c \
			    tad_ipv4_layer.c \
			    tad_ipv4_stack.c \
			    tad_icmpv4_layer.c \
			    tad_icmpv4_stack.c \
			    tad_udp_layer.c \
			    tad_udp_stack.c 


# libtad_ch_la_LIBS=-lasn

install-exec-hook:
	touch -c -r libtad_ipstack.a $(DESTDIR)/$(prefix)/lib/libtad_ipstack.a

# LDADD=-lasn
# LDFLAGS=-static

all: install

