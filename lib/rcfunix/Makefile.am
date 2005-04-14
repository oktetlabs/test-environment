
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include
AM_CFLAGS =
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib

lib_LTLIBRARIES=librcfunix.la

librcfunix_la_SOURCES=rcfunix.c

librcfunix_la_LDFLAGS=-lcomm_net_engine 

install-exec-hook:
	touch -c -r .libs/librcfunix.la $(DESTDIR)/$(prefix)/lib/librcfunix.*

all: install

