

AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

bin_PROGRAMS=conftest

conftest_SOURCES=conftest.c

conftest_LDADD=-lconfapi -lipc -lpthread

conftest_DEPENDENCIES=${TE_INSTALL}/i686-pc-linux-gnu/lib/libconfapi.a

