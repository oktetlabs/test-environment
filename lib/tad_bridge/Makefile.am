
AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include 

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

# VPATH = @srcdir@:@srcdir@/tests

lib_LIBRARIES=libtad_bridge.a

libtad_bridge_a_SOURCES=tad_bridge_stack.c tad_bridge_layer.c tad_bridge_csap.c 

# libtad_ch_la_LIBS=-lasn

install-exec-hook:
	touch -c -r libtad_bridge.a $(DESTDIR)/$(prefix)/lib/libtad_bridge.a

# LDADD=-lasn
# LDFLAGS=-static

all: install

