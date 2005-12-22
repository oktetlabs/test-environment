
AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) -I$(top_srcdir)
AM_CFLAGS = @TE_CFLAGS@ 

noinst_HEADERS=tad_bridge_impl.h

noinst_LIBRARIES=libtad_bridge.a

libtad_bridge_a_SOURCES=tad_bridge_stack.c tad_bridge_layer.c \
                        tad_bridge_csap.c 


install-exec-hook:
	touch -c -r libtad_bridge.a $(DESTDIR)/$(libdir)/libtad_bridge.a

all: install

