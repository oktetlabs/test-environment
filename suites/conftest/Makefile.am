
tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml


AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include \
              -I$(DESTDIR)/$(prefix)/../include

AM_CFLAGS =

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


tetest_PROGRAMS=conftest localtest


MYLDADD = -lconfapi -lconf_oid -llogger_ten -lipc -lpthread
MYDEPS = ${TE_INSTALL}/@host@/lib/libconfapi.a \
         ${TE_INSTALL}/@host@/lib/libconf_oid.a \
         ${TE_INSTALL}/@host@/lib/liblogger_ten.a \
         ${TE_INSTALL}/@host@/lib/libipc.a


conftest_SOURCES = conftest.c
conftest_LDADD = $(MYLDADD)
conftest_DEPENDENCIES = $(MYDEPS)

localtest_SOURCES = localtest.c
localtest_LDADD = $(MYLDADD)
localtest_DEPENDENCIES = $(MYDEPS)

