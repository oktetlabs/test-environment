

AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

bin_PROGRAMS=localtest

localtest_SOURCES=localtest.c

localtest_LDADD=-lconfapi -lipc -lpthread

localtest_DEPENDENCIES=${TE_INSTALL}/i686-pc-linux-gnu/lib/libconfapi.a

