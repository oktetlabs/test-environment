
AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

lib_LTLIBRARIES=librcfunix.la

librcfunix_la_SOURCES=rcfunix.c

librcfunix_la_LDFLAGS=-lcomm_net_engine

install-exec-hook:
	touch -c -r .libs/librcfunix.a $(DESTDIR)/$(prefix)/lib/librcfunix.a

all: install

