
tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include \
              -I$(DESTDIR)/$(prefix)/../include
AM_CFLAGS =

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


tetest_PROGRAMS = example


example_SOURCES = example.c
example_LDADD = -llogger_ten -lrcfapi -lipc -ltapi -lconfapi -lconf_types -lconf_oid -lpthread
example_DEPENDENCIES = ${TE_INSTALL}/@host@/lib/librcfapi.a \
                       ${TE_INSTALL}/@host@/lib/liblogger_ten.a \
                       ${TE_INSTALL}/@host@/lib/libipc.a

