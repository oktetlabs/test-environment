
AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) \
              $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

noinst_HEADERS = tad_socket_impl.h

noinst_LIBRARIES = libtad_socket.a

libtad_socket_a_SOURCES = \
	tad_socket_csap.c \
	tad_socket_layer.c \
	tad_socket_stack.c
