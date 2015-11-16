
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) \
              -DTE_LIB_NAME=@with_name@ $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)

lib_LTLIBRARIES=lib@with_name@.la

lib@with_name@_la_SOURCES=rcfunix.c

lib@with_name@_la_LDFLAGS=-lcomm_net_engine 

install-exec-hook:
	touch -c -r lib@with_name@.la $(DESTDIR)/$(libdir)/lib@with_name@.*

all: install

