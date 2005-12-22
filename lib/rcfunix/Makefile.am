
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)

lib_LTLIBRARIES=librcfunix.la

librcfunix_la_SOURCES=rcfunix.c

librcfunix_la_LDFLAGS=-lcomm_net_engine 

install-exec-hook:
	touch -c -r .libs/librcfunix.la $(DESTDIR)/$(libdir)/librcfunix.*

all: install

