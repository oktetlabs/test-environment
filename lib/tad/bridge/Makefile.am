
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include \
            -I$(DESTDIR)/$(prefix)/../include 

AM_CFLAGS=

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


noinst_HEADERS=tad_bridge_impl.h

noinst_LIBRARIES=libtad_bridge.a

libtad_bridge_a_SOURCES=tad_bridge_stack.c tad_bridge_layer.c tad_bridge_csap.c 


install-exec-hook:
	touch -c -r libtad_bridge.a $(DESTDIR)/$(prefix)/lib/libtad_bridge.a

all: install

