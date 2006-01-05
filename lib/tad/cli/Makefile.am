
AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@


noinst_HEADERS=tad_cli_impl.h

noinst_LIBRARIES=libtad_cli.a

libtad_cli_a_SOURCES=tad_cli_stack.c tad_cli_layer.c tad_cli_csap.c 


install-exec-hook:
	touch -c -r libtad_cli.a $(DESTDIR)/$(libdir)/libtad_cli.a

all: install

