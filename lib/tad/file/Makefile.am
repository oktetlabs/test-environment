
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include \
            -I$(DESTDIR)/$(prefix)/../include

AM_CFLAGS=

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib


noinst_HEADERS=tad_file_impl.h

noinst_LIBRARIES=libtad_file.a

libtad_file_a_SOURCES=tad_file_stack.c tad_file_layer.c tad_file_csap.c 


install-exec-hook:
	touch -c -r libtad_file.a $(DESTDIR)/$(prefix)/lib/libtad_file.a

all: install

