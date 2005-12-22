
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml


AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)


tetest_PROGRAMS = wait_discover

LDADD= -ltapi -ltapi_dhcp -ltapi_tad -lndn -lasn -lconfapi -lrcfapi \
       -lrpcxdr -lconf_oid -llogger_ten -lipc -ltools -lpthread 

MYDEPS = @SELFTAD_TE_DEPS@ @TE_INSTALL@/@host@/lib/libtapi_dhcp.a 

wait_discover_DEPENDENCIES = $(MYDEPS)


