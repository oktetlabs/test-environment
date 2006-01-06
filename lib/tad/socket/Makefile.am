
AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 

noinst_HEADERS = tad_socket_impl.h

noinst_LIBRARIES = libtad_socket.a

libtad_socket_a_SOURCES = tad_socket_csap.c tad_socket_layer.c \
                          tad_socket_stack.c

install-exec-hook:
	touch -c -r libtad_socket.a $(DESTDIR)/$(libdir)/libtad_socket.a

all: install

