
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include \
            -I$(DESTDIR)/$(prefix)/../include 

AM_CFLAGS=

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


noinst_HEADERS=tad_cli_impl.h

lib_LIBRARIES=libtad_cli.a

libtad_cli_a_SOURCES=tad_cli_stack.c tad_cli_layer.c tad_cli_csap.c 


install-exec-hook:
	touch -c -r libtad_cli.a $(DESTDIR)/$(prefix)/lib/libtad_cli.a

all: install

