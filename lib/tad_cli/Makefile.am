
AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include 

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

# VPATH = @srcdir@:@srcdir@/tests

lib_LIBRARIES=libtad_cli.a

libtad_cli_a_SOURCES=tad_cli_stack.c tad_cli_layer.c tad_cli_csap.c 

# libtad_ch_la_LIBS=-lasn

install-exec-hook:
	touch -c -r libtad_cli.a $(DESTDIR)/$(prefix)/lib/libtad_cli.a

# LDADD=-lasn
# LDFLAGS=-static

all: install

