
AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include 

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

# VPATH = @srcdir@:@srcdir@/tests

lib_LIBRARIES=libtad_file.a

libtad_file_a_SOURCES=tad_file_stack.c tad_file_layer.c tad_file_csap.c 

# libtad_ch_la_LIBS=-lasn

install-exec-hook:
	touch -c -r libtad_file.a $(DESTDIR)/$(prefix)/lib/libtad_file.a

#TESTS=cdb01 cdb02

LDADD=-lasn
LDFLAGS=-static

#check_PROGRAMS=cdb01 cdb02

all: install

