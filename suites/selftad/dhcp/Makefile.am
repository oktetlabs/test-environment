
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml


AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include 
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


tetest_PROGRAMS = wait_discover

LDADD= -ltapi -ltapi_dhcp -ltapi_tad -lndn -lasn -lconfapi -lrcfapi \
       -lconf_oid -llogger_ten -lipc -lpthread 

MYDEPS = @SELFTAD_TE_DEPS@ @TE_INSTALL@/@host@/lib/libtapi_dhcp.a 

wait_discover_DEPENDENCIES = $(MYDEPS)


