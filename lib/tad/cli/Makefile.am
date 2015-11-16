
AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) \
              $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

noinst_HEADERS = tad_cli_impl.h

noinst_LIBRARIES = libtad_cli.a

libtad_cli_a_SOURCES = tad_cli_stack.c tad_cli_layer.c tad_cli_csap.c 
