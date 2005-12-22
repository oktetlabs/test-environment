
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

tetest_DATA = tests-info.xml

tests-info.xml: $(srcdir)/*.c
	@TE_PATH@/te_tests_info.sh $(srcdir) > tests-info.xml

CLEANFILES = tests-info.xml


AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)


tetest_PROGRAMS = example


example_SOURCES = example.c
example_LDADD = -llogger_ten -ltapi_rpc -lrcfapi -lipc -ltapi \
                -ltapi_eth -ltapi_arp \
				-lndn -lasn -ltapi_tad -ltapi_iscsi -ltapi_ipstack \
				-lconfapi \
                -lrcfrpc -lrpcxdr -lconf_types -lconf_oid -lpthread -ltools
                
example_DEPENDENCIES = \
    ${TE_INSTALL}/@host@/lib/librcfapi.a \
    ${TE_INSTALL}/@host@/lib/liblogger_ten.a \
    ${TE_INSTALL}/@host@/lib/libipc.a \
    ${TE_INSTALL}/@host@/lib/libtapi.a \
    ${TE_INSTALL}/@host@/lib/libconfapi.a \
    ${TE_INSTALL}/@host@/lib/libconf_types.a 


